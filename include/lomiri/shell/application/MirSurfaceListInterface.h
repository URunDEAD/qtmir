/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOMIRI_SHELL_APPLICATION_MIRSURFACELISTINTERFACE_H
#define LOMIRI_SHELL_APPLICATION_MIRSURFACELISTINTERFACE_H

#include <QAbstractListModel>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
Q_MOC_INCLUDE("MirSurfaceInterface.h")
#endif

namespace lomiri {
namespace shell {
namespace application {

class MirSurfaceInterface;

/**
 * @brief Interface for a list model of MirSurfaces
 */
class MirSurfaceListInterface : public QAbstractListModel
{

    Q_OBJECT

    /**
     * @brief Number of surfaces in this model
     *
     * This is the same as rowCount, added in order to keep compatibility with QML ListModels.
     */
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /**
     * @brief The first (index 0) surface in this model
     *
     * Will always match the result of get(0). But being a property, it's more appropriate
     * for use in qml bindinds (JS expression gets reevaluated when it changes)
     */
    Q_PROPERTY(lomiri::shell::application::MirSurfaceInterface* first READ first NOTIFY firstChanged)
public:
    /**
     * @brief The Roles supported by the model
     *
     * SurfaceRole - A MirSurfaceInterface.
     */
    enum Roles {
        SurfaceRole = Qt::UserRole,
    };

    /// @cond
    MirSurfaceListInterface(QObject *parent = 0) : QAbstractListModel(parent) {}
    virtual ~MirSurfaceListInterface() {}
    /// @endcond

    /**
     * @brief Returns the surface at the specified index
     *
     */
    Q_INVOKABLE virtual MirSurfaceInterface *get(int index) = 0;

    /// @cond
    // QAbstractItemModel methods
    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roleNames;
        roleNames.insert(SurfaceRole, "surface");
        return roleNames;
    }

    int count() const { return rowCount(); }

    MirSurfaceInterface *first() {
        if (rowCount() > 0) {
            return get(0);
        } else {
            return nullptr;
        }
    }
    /// @endcond

Q_SIGNALS:
    /// @cond
    void countChanged(int count);
    void firstChanged();
    /// @endcond
};

} // namespace application
} // namespace shell
} // namespace lomiri

Q_DECLARE_METATYPE(lomiri::shell::application::MirSurfaceListInterface*)

#endif // LOMIRI_SHELL_APPLICATION_MIRSURFACELISTINTERFACE_H
