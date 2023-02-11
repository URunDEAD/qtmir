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

    QtKwinServerDecoration(struct wl_resource* resource, struct wl_resource* surface, qtmir::WindowModelNotifier &windowModel) :
        KwinServerDecoration(resource, Version<1>{}),
        m_windowModel(windowModel),
        surface(surface)
    {
        send_mode_event(Mode::Server);
    };

    void request_mode(uint32_t mode) override {
        qDebug() << "set KDE mode: " << mode;
        auto win = miral::window_for(resource);

        Q_EMIT m_windowModel.windowDecorationChanged(win, mode == Mode::Server);
        send_mode_event(mode);
    };
private:
    qtmir::WindowModelNotifier &m_windowModel;
    struct wl_resource* surface;
};

class QtKwinServerDecorationManager : public KwinServerDecorationManager
{
public:
    using KwinServerDecorationManager::KwinServerDecorationManager;

    QtKwinServerDecorationManager(struct wl_resource*, qtmir::WindowModelNotifier &windowModel);

    void create(struct wl_resource* id, struct wl_resource* surface) override {
        new QtKwinServerDecoration{id, surface, m_windowModel};
    };
private:
    qtmir::WindowModelNotifier &m_windowModel;
};



QtKwinServerDecorationManager::QtKwinServerDecorationManager(
    struct wl_resource* resource, qtmir::WindowModelNotifier &windowModel) :
    KwinServerDecorationManager{resource, Version<1>{}},
    m_windowModel(windowModel)
{
    send_default_mode_event(Mode::Server);
}

class MyGlobal : public KwinServerDecorationManager::Global
{
public:
    explicit MyGlobal(wl_display* display, qtmir::WindowModelNotifier &windowModel);
    
    void bind(wl_resource* new_xdgdecormanager) override;

private:
    qtmir::WindowModelNotifier &m_windowModel;
};

}

MyGlobal::MyGlobal(wl_display* display, qtmir::WindowModelNotifier &windowModel) :
    Global(display, Version<1>{}), m_windowModel(windowModel)
{
}

void MyGlobal::bind(wl_resource* new_kdedecormanager)
{
    new QtKwinServerDecorationManager{new_kdedecormanager, m_windowModel};
}

auto qtmir::qtKwinServerDecorationManagerExtension(qtmir::WindowModelNotifier &windowModel) -> WaylandExtensions::Builder {
    return {
        KwinServerDecorationManager::interface_name,
        [&](WaylandExtensions::Context const* context) {
            return std::make_shared<MyGlobal>(context->display(), windowModel);
        }
    };
}
