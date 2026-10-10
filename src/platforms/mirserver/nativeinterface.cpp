/*
 * Copyright (C) 2013-2016 Canonical Ltd.
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

#include "nativeinterface.h"

#include "qmirserver.h"
#include "windowcontrollerinterface.h"
#include "platformscreen.h"

#include <QDebug>
#include <QRect>
#include <QScreen>

NativeInterface::NativeInterface(QMirServer *server)
    : m_qMirServer(server)
{
}

void *NativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    return m_qMirServer->nativeResourceForIntegration(resource);
}

void *NativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    if (!screen)
        return nullptr;

    if (resource == QByteArrayLiteral("scale")) {
        auto platformScreen = static_cast<PlatformScreen*>(screen->handle());
        if (platformScreen) {
            return (void*)platformScreen->scaleRef();
        }
    }

    return nullptr;
}

void NativeInterface::setWindowProperty(QPlatformWindow */*window*/, const QString &name, const QVariant &value)
{
    if (name.isNull()) {
        return;
    }

    // TODO remove this hack and expose this properly when QtMir can share WindowController in a C++ header

    // Get WindowController
    auto windowController = static_cast<qtmir::WindowControllerInterface *>(m_qMirServer->nativeResourceForIntegration("WindowController"));

    if (name == QStringLiteral("availableDesktopArea")) {
        QRect rect = value.toRect();
        if (rect.isValid()) {
            windowController->setWindowConfinementRegions({rect});
        } else {
            qWarning().nospace() << "NativeInterface::setWindowProperty("
                << name << "," << value << ") - value is not a QRect";
       }
    } else if (name.endsWith(QStringLiteral("WindowMargins"))) {
        if (value.canConvert(QMetaType::QRect)) {
            QRect rect = value.toRect();
            QMargins margin(rect.x(), rect.y(), rect.width(), rect.height());
            if (name == QStringLiteral("normalWindowMargins")) {
                windowController->setWindowMargins(Mir::NormalType, margin);
            } else if (name == QStringLiteral("dialogWindowMargins")) {
                windowController->setWindowMargins(Mir::DialogType, margin);
            } else {
                qWarning() << "NativeInterface::setWindowProperty missing support for:" << name;
            }
        } else {
            qWarning().nospace() << "NativeInterface::setWindowProperty("
                << name << "," << value << ") - value is not a QRect";
        }
    }
}

#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 0, 0)
std::shared_ptr<qtmir::PromptSessionManager> NativeInterface::thePromptSessionManager() const
{
    return m_qMirServer->thePromptSessionManager();
}
#endif
