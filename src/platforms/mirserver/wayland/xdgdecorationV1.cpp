/*
 * Copyright (C) 2020 UBports Foundation
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

#include "xdgdecorationV1.h"
#include "services.h"

#include <wayland-generated/xdg-decoration-unstable-v1_wrapper.h>

#include <QDebug>

using namespace mir::wayland;
using namespace miral;

namespace
{

class QtXdgToplevelDecorationV1 : public XdgToplevelDecorationV1
{
public:
    using XdgToplevelDecorationV1::XdgToplevelDecorationV1;

    QtXdgToplevelDecorationV1(struct wl_resource* resource) : XdgToplevelDecorationV1(resource, Version<1>{}) {
        send_configure_event(Mode::server_side);
    };

    void set_mode(uint32_t mode) override {
        send_configure_event(Mode::server_side);
        qDebug() << "set XDG mode: " << mode;
    };
    void unset_mode() override {
        qDebug("unset_mode");
    }
};

class QtXdgDecorationManagerV1 : public XdgDecorationManagerV1
{
public:
    using XdgDecorationManagerV1::XdgDecorationManagerV1;

    QtXdgDecorationManagerV1(struct wl_resource*);

    void get_toplevel_decoration(struct wl_resource* id, struct wl_resource* toplevel) override {
        Q_UNUSED(toplevel);
        new QtXdgToplevelDecorationV1{id};
    };
};



QtXdgDecorationManagerV1::QtXdgDecorationManagerV1(
    struct wl_resource* resource) :
    XdgDecorationManagerV1{resource, Version<1>{}}
{
}

class MyGlobal : public QtXdgDecorationManagerV1::Global
{
public:
    explicit MyGlobal(wl_display* display);
    
    void bind(wl_resource* new_xdgdecormanager) override;
};

}

MyGlobal::MyGlobal(wl_display* display) :
    Global(display, Version<1>{})
{
}

void MyGlobal::bind(wl_resource* new_xdgdecormanager)
{
    new QtXdgDecorationManagerV1{new_xdgdecormanager};
}

auto qtmir::qtXdgDecorationManagerV1Extension() -> WaylandExtensions::Builder {
    return {
        XdgDecorationManagerV1::interface_name,
        [](WaylandExtensions::Context const* context) {
            return std::make_shared<MyGlobal>(context->display());
        }
    };
}
