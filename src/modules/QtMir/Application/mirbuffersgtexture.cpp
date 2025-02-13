/*
 * Copyright 2021 UBports Foundation
 * Copyright (C) 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mirbuffersgtexture.h"

#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 20, 0)

// Mir
#include <mir/geometry/size.h>

// Qt
#include <QDebug>
#include <QtGui/QOpenGLFunctions>
#include <QMutexLocker>

namespace mg = mir::geometry;

class MirGlBuffer
{
public:
    MirGlBuffer(const std::shared_ptr<miral::GLBuffer>& buffer);
    virtual ~MirGlBuffer();

    static std::shared_ptr<MirGlBuffer> from_mir_buffer(const std::shared_ptr<mir::graphics::Buffer>& buffer);

    void setBuffer(const std::shared_ptr<mir::graphics::Buffer>& buffer);
    void freeBuffer();
    bool hasBuffer() const;

    int textureId();
    QSize textureSize() const;
    bool hasAlphaChannel() const;

    void bind();

    virtual void updateTextureId() = 0;

protected:
    virtual void bindTexture() = 0;

    std::shared_ptr<miral::GLBuffer> m_mirBuffer;
    GLuint m_textureId;

private:
    bool m_needsUpdate;
    QMutex m_mutex;
    int m_width;
    int m_height;
};

class MirGlBufferTexture : public MirGlBuffer
{
public:
    MirGlBufferTexture(const std::shared_ptr<miral::GLBuffer>& buffer)
        : MirGlBuffer(buffer)
          // Strong reference held by base class.
        , m_texBuf(static_cast<miral::GLTextureBuffer *>(buffer.get()))
        {}

    void updateTextureId() override
    {
        auto cc = QOpenGLContext::currentContext();
        if (!cc) {
            qWarning() << "WARNING: MirGlBufferTexture::updateTextureId: no context, no texture. This should not happen!";
            return; // No context, no texture
        }

        auto f = cc->functions();

        GLint current_binding;
        f->glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_binding);
        m_texBuf->bind();
        f->glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*) &m_textureId);
        f->glBindTexture(GL_TEXTURE_2D, (GLint) current_binding);
    }

    void bindTexture() override {
        m_texBuf->bind();
    }

private:
    miral::GLTextureBuffer *m_texBuf;
};

class MirGlBufferTexturesource : public MirGlBuffer
{
public:
    MirGlBufferTexturesource(const std::shared_ptr<miral::GLBuffer>& buffer)
        : MirGlBuffer(buffer)
          // Strong reference held by base class.
        , m_texSourceBuf(static_cast<miral::GLTextureSourceBuffer *>(buffer.get()))
        {}

    void updateTextureId() override
    {
        auto cc = QOpenGLContext::currentContext();
        if (!cc) {
            qWarning() << "WARNING: MirGlBufferTexturesource::updateTextureId: no context, no texture. This should not happen!";
            return; // No context, no texture
        }

        auto f = cc->functions();

        if (!m_textureId) {
            f->glGenTextures(1, &m_textureId);
        }
    }

    void bindTexture() override
    {
        auto cc = QOpenGLContext::currentContext();
        if (!cc) {
            qWarning() << "WARNING: MirGlBufferTexturesource::bindTexture: no context, no texture. This should not happen!";
            return; // No context, no texture
        }

        auto f = cc->functions();

        f->glBindTexture(GL_TEXTURE_2D, m_textureId);

        m_texSourceBuf->bind();

        // Fix for lp:1583088 - For non-GL clients, Mir uploads the client pixel buffer to a GL texture.
        // But as it does so, it changes some GL state and neglects to restore it, which breaks Qt's rendering.
        f->glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // 4 is the default which Qt uses
    }

private:
    miral::GLTextureSourceBuffer * m_texSourceBuf;
};

MirGlBuffer::MirGlBuffer(const std::shared_ptr<miral::GLBuffer>& buffer) :
    m_mirBuffer(buffer)
    , m_textureId(0)
    , m_needsUpdate(true)
{
    mg::Size size = m_mirBuffer->size();
    m_height = size.height.as_int();
    m_width = size.width.as_int();
}

MirGlBuffer::~MirGlBuffer()
{
    QMutexLocker locker(&m_mutex);

    auto cc = QOpenGLContext::currentContext();
    if (!cc) {
        qWarning() << "WARNING: MirGlBuffer::~MirGlBuffer: no context. This should not happen!";
    }
    if (m_textureId && cc) {
        auto f = cc->functions();
        if (f) f->glDeleteTextures(1, &m_textureId);
    }
}

std::shared_ptr<MirGlBuffer> MirGlBuffer::from_mir_buffer(const std::shared_ptr<mir::graphics::Buffer>& buffer) {
    auto glBuffer = miral::GLBuffer::from_mir_buffer(buffer);
    if (glBuffer->type() == miral::GLBuffer::Type::GLTextureSource)
        return std::make_shared<MirGlBufferTexturesource>(glBuffer);
    else
        return std::make_shared<MirGlBufferTexture>(glBuffer);
}

void MirGlBuffer::freeBuffer()
{
    QMutexLocker locker(&m_mutex);

    if (!m_mirBuffer)
        return;

    m_mirBuffer->reset();
    m_width = 0;
    m_height = 0;
}

void MirGlBuffer::setBuffer(const std::shared_ptr<mir::graphics::Buffer>& buffer)
{
    QMutexLocker locker(&m_mutex);

    m_mirBuffer->reset(buffer);

    mg::Size size = m_mirBuffer->size();
    m_height = size.height.as_int();
    m_width = size.width.as_int();
    m_needsUpdate = true;
}

bool MirGlBuffer::hasBuffer() const
{
    if (!m_mirBuffer)
        return false;

    return !m_mirBuffer->empty();
}

int MirGlBuffer::textureId()
{
    QMutexLocker locker(&m_mutex);

    if (m_needsUpdate) {
        updateTextureId();
        m_needsUpdate = false;
    }

    return m_textureId;
}

QSize MirGlBuffer::textureSize() const
{
    return QSize(m_width, m_height);
}

bool MirGlBuffer::hasAlphaChannel() const
{
    return m_mirBuffer->has_alpha_channel();
}

void MirGlBuffer::bind() {
    QMutexLocker locker(&m_mutex);

    Q_ASSERT(hasBuffer());

    bindTexture();
}

MirBufferSGTexture::MirBufferSGTexture()
    : QSGTexture()
{
    setFiltering(QSGTexture::Linear);
    setHorizontalWrapMode(QSGTexture::ClampToEdge);
    setVerticalWrapMode(QSGTexture::ClampToEdge);
}

MirBufferSGTexture::~MirBufferSGTexture()
{
    m_mirBuffer.reset();
}

void MirBufferSGTexture::freeBuffer()
{
    if (!m_mirBuffer)
        return;

    m_mirBuffer->freeBuffer();
}

void MirBufferSGTexture::setBuffer(const std::shared_ptr<mir::graphics::Buffer>& buffer)
{
    // For performance reasons, lets not recreate
    // the glbuffer class
    if (m_mirBuffer) {
        try {
            m_mirBuffer->setBuffer(buffer);
            return;
        } catch (const std::runtime_error &) {
            // Likely incompatible buffer. Try re-creating the wrapper.
        }
    }

    m_mirBuffer = MirGlBuffer::from_mir_buffer(buffer);
}

bool MirBufferSGTexture::hasBuffer() const
{
    if (!m_mirBuffer)
        return false;

    return m_mirBuffer->hasBuffer();
}

int MirBufferSGTexture::textureId() const
{
    return m_mirBuffer->textureId();
}

QSize MirBufferSGTexture::textureSize() const
{
    return m_mirBuffer->textureSize();
}

bool MirBufferSGTexture::hasAlphaChannel() const
{
    return m_mirBuffer->hasAlphaChannel();
}

void MirBufferSGTexture::bind()
{
    m_mirBuffer->bind();
    updateBindOptions(true/* force */);
}

#else

/*
 * Copyright 2021 UBports Foundation.
 * Copyright (C) 2013-2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mirbuffersgtexture.h"

// Mir
#include <mir/geometry/size.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/texture.h>
#include <mir/graphics/drm_formats.h>

MirBufferSGTexture::MirBufferSGTexture()
    : QSGTexture()
{
    setFiltering(QSGTexture::Linear);
    setHorizontalWrapMode(QSGTexture::ClampToEdge);
    setVerticalWrapMode(QSGTexture::ClampToEdge);
}

MirBufferSGTexture::~MirBufferSGTexture() = default;

void MirBufferSGTexture::freeBuffer()
{
    m_mirBuffer.reset();
    m_mirTexture.reset();
}

void MirBufferSGTexture::setBuffer(
    const std::shared_ptr<mir::graphics::Buffer>& buffer,
    mir::graphics::GLRenderingProvider& glProvider)
{
    m_mirBuffer = buffer;
    m_mirTexture = glProvider.as_texture(buffer);
}

bool MirBufferSGTexture::hasBuffer() const
{
    return m_mirBuffer && m_mirTexture;
}

std::shared_ptr<mir::graphics::Buffer> MirBufferSGTexture::buffer() const
{
    return m_mirBuffer;
}

int MirBufferSGTexture::textureId() const
{
    return m_mirTexture->tex_id();
}

QSize MirBufferSGTexture::textureSize() const
{
    auto const size = m_mirBuffer->size();
    return QSize{size.width.as_value(), size.height.as_value()};
}

bool MirBufferSGTexture::hasAlphaChannel() const
{
    /* Mir will probably move towards DRMFormat as the
     * common format information; it already covers many more
     * formats than MirPixelFormat, and has more information
     * easily available.
     */
    auto format = mir::graphics::DRMFormat::from_mir_format(m_mirBuffer->pixel_format());
    if (format.info())
    {
        return format.info().value().has_alpha();
    }
    return false;
}

void MirBufferSGTexture::bind()
{
    m_mirTexture->bind();
    updateBindOptions(true/* force */);
}

#endif
