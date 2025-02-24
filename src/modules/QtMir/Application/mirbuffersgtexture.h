/*
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

#ifndef MIRBUFFERSGTEXTURE_H
#define MIRBUFFERSGTEXTURE_H


#include <mir/version.h>

#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(2, 20, 0)
namespace mir::graphics
{
class Buffer;
class GLRenderingProvider;
namespace gl
{
class Texture;
}
}
#endif

#include <QSGTexture>

#include <QtGui/qopengl.h>

#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 20, 0)
#include <miroil/mirbuffer.h>

class MirGlBuffer;
#endif

class MirBufferSGTexture : public QSGTexture
{
    Q_OBJECT
public:
    MirBufferSGTexture();
    virtual ~MirBufferSGTexture();

    // virtual so tests can substitute a texture that needs no GL context
#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 20, 0)
    virtual void setBuffer(const std::shared_ptr<mir::graphics::Buffer>& buffer);
#else
    virtual void setBuffer(const std::shared_ptr<mir::graphics::Buffer>& buffer, mir::graphics::GLRenderingProvider& glProvider);
#endif
    virtual void freeBuffer();
    virtual bool hasBuffer() const;

    int textureId() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override { return false; }

    void bind() override;

private:
#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 20, 0)
    std::shared_ptr<MirGlBuffer> m_mirBuffer;
#else
    std::shared_ptr<mir::graphics::Buffer> m_mirBuffer;
    std::shared_ptr<mir::graphics::gl::Texture> m_mirTexture;
#endif
};

#endif // MIRBUFFERSGTEXTURE_H
