/*
 * Copyright 2021 UBports Foundation.
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mirbuffer.h"

#include <mir/graphics/buffer.h>
#include "mir/graphics/texture.h"
#include <mir/renderer/gl/texture_source.h>

#include <stdexcept>

#include <QDebug>

miral::GLBuffer::~GLBuffer() = default;

miral::GLBuffer::GLBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    wrapped(buffer)
{
}

std::shared_ptr<miral::GLBuffer> miral::GLBuffer::from_mir_buffer(std::shared_ptr<mir::graphics::Buffer> const& buffer)
{
    bool usingTextureSource = false;

    // We would like to use gl::Texture, but if we cant, fallback to gl::textureSource
    if (!dynamic_cast<mir::graphics::gl::Texture*>(buffer->native_buffer_base()))
        // As textures will never change once inited, there is no need to do vodo magic
        // on each bind(), so lets create class overrides to save some cpu cycles.
        usingTextureSource = true;

    qDebug() << "Mir buffer is" << (usingTextureSource ? "gl:TextureSource (old)" : "gl:Texture (new)");

    if (usingTextureSource)
        return std::make_shared<miral::GLTextureSourceBuffer>(buffer);
    else
        return std::make_shared<miral::GLTextureBuffer>(buffer);
}

void miral::GLBuffer::reset(std::shared_ptr<mir::graphics::Buffer> const& buffer)
{
    wrapped = buffer;
}

void miral::GLBuffer::reset()
{
    wrapped.reset();
}

bool miral::GLBuffer::empty()
{
    return !wrapped;
}

bool miral::GLBuffer::has_alpha_channel() const
{
    return wrapped &&
        (wrapped->pixel_format() == mir_pixel_format_abgr_8888
        || wrapped->pixel_format() == mir_pixel_format_argb_8888);
}

mir::geometry::Size miral::GLBuffer::size() const
{
    return wrapped->size();
}

miral::GLTextureSourceBuffer::GLTextureSourceBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    GLBuffer(buffer)
{
}

void miral::GLTextureSourceBuffer::bind()
{
    if (!wrapped)
        throw std::logic_error("Bind called without any buffers!");

    if (auto const texsource = dynamic_cast<mir::renderer::gl::TextureSource*>(wrapped->native_buffer_base())) {
        texsource->gl_bind_to_texture();
        texsource->secure_for_render();
    } else {
        throw std::logic_error("Buffer does not support GL rendering");
    }
}

miral::GLTextureBuffer::GLTextureBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    GLBuffer(buffer)
{
}

void miral::GLTextureBuffer::bind()
{
    if (!wrapped)
        throw std::logic_error("Bind called without any buffers!");

    if (auto const texture = dynamic_cast<mir::graphics::gl::Texture*>(wrapped->native_buffer_base())) {
        texture->bind();
    } else {
        throw std::logic_error("Buffer does not support GL rendering");
    }
}
