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

#include "kwinserverdecoration.h"
#include "services.h"

#include <wayland-generated/server-decoration_wrapper.h>

#include <QDebug>

using namespace mir::wayland;
using namespace miral;

namespace
{

class QtKwinServerDecoration : public KwinServerDecoration
{
public:
    using KwinServerDecoration::KwinServerDecoration;

    QtKwinServerDecoration(struct wl_resource* resource) : KwinServerDecoration(resource, Version<1>{}) {
        send_mode_event(Mode::Server);
    };

    void release() override {
        qDebug() << "destroy";
    };
    void request_mode(uint32_t mode) override {
        send_mode_event(mode);
        qDebug() << "set mode: " << mode;
    };
};

class QtKwinServerDecorationManager : public KwinServerDecorationManager
{
public:
    using KwinServerDecorationManager::KwinServerDecorationManager;

    QtKwinServerDecorationManager(struct wl_resource*);

    void create(struct wl_resource* id, struct wl_resource* surface) override {
        Q_UNUSED(surface);
        new QtKwinServerDecoration{id};
    };
};



QtKwinServerDecorationManager::QtKwinServerDecorationManager(
    struct wl_resource* resource) :
    KwinServerDecorationManager{resource, Version<1>{}}
{
    send_default_mode_event(Mode::Server);
}

class MyGlobal : public KwinServerDecorationManager::Global
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

void MyGlobal::bind(wl_resource* new_kdedecormanager)
{
    new QtKwinServerDecorationManager{new_kdedecormanager};
}

auto qtmir::qtKwinServerDecorationManagerExtension() -> WaylandExtensions::Builder {
    return {
        KwinServerDecorationManager::interface_name,
        [](WaylandExtensions::Context const* context) {
            return std::make_shared<MyGlobal>(context->display());
        }
    };
}
