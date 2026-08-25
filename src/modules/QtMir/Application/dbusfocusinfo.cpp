/*
 * Copyright (C) 2016 Canonical Ltd.
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

#include "dbusfocusinfo.h"

// local
#include "mirsurfacelistmodel.h"
#include "mirsurfaceinterface.h"
#include "session_interface.h"

// QPA mirserver
#include <logging.h>
#include <shelluuid.h>

#include <QDBusConnection>

using namespace qtmir;

DBusFocusInfo::DBusFocusInfo(const QList<Application*> &applications)
    : m_applications(applications)
{
    QDBusConnection::sessionBus().registerService("com.lomiri.Shell.FocusInfo");
    QDBusConnection::sessionBus().registerObject("/com/lomiri/Shell/FocusInfo", this, QDBusConnection::ExportScriptableSlots);
}

bool DBusFocusInfo::isPidFocused(unsigned int pid)
{
    if (QCoreApplication::applicationPid() == (qint64)pid) {
        // Shell itself.
        // Don't bother checking if it has a QML with activeFocus() which is not a MirSurfaceItem.
        return true;
    } else {
        auto pidSet = fetchAssociatedPids((pid_t)pid);
        SessionInterface *session = findSessionWithPid(pidSet);
        return session ? session->activeFocus() : false;
    }
}

namespace {

struct CGroupControllerPath {
    QByteArray path;
    bool is_v1;
};

struct CGroupControllerPath getCGroupControllerPath(pid_t pid)
{
    QFile procCGroup(QStringLiteral("/proc/%1/cgroup").arg(pid));
    if (!procCGroup.open(QIODevice::ReadOnly))
        return { .path = QByteArray(), .is_v1 = false };

    QByteArray line;
    while (line = procCGroup.readLine(), !line.isEmpty()) {
        QList<QByteArray> parts = line.split(':');

        // XXX: if Systemd is in hybrid mode, both CGroup v1 and v2 hierarchy
        // will be available and there's no guarantee what we'll get first. But
        // I believe they would just give the same answer anyway. So, :shrug:.

        if (parts[0] == "0") {
            // CGroup v2
            return { .path = parts[2].trimmed(), .is_v1 = false };
        } else if (parts[1] == "name=systemd") {
            // CGroup v1 named hierarchy
            return { .path = parts[2].trimmed(), .is_v1 = true };
        }
    }

    // CGroup not available?
    return { .path = QByteArray(), .is_v1 = false };
}

QSet<pid_t> getPidsInCGroup(const struct CGroupControllerPath & cgPath)
{
    // XXX: this relies on specifics of Systemd setup. To be fool-proof, one
    // would want to parse /proc/cgroups and/or /proc/self/mounts, but since
    // we're tied to Systemd anyway, :shrug:.

    QString cgMnt;
    if (cgPath.is_v1) {
        cgMnt = QStringLiteral("/sys/fs/cgroup/systemd");
    } else if (QDir(QStringLiteral("/sys/fs/cgroup/unified")).exists()) {
        cgMnt = QStringLiteral("/sys/fs/cgroup/unified");
    } else {
        cgMnt = QStringLiteral("/sys/fs/cgroup");
    }

    QFile cgProcs(
        QStringLiteral("%1/%2/cgroup.procs")
            .arg(cgMnt)
            .arg(QString::fromLocal8Bit(cgPath.path)));

    if (!cgProcs.open(QIODevice::ReadOnly)) {
        return QSet<pid_t>();
    }

    QSet<pid_t> pids;
    QByteArray line;
    while (line = cgProcs.readLine(), !line.isEmpty()) {
        bool ok;
        pid_t pid = line.toInt(&ok, /* base */ 10);

        if (ok) {
            pids.insert(pid);
        } else {
            qCWarning(QTMIR_DBUS) << "DBusFocusInfo: error while parsing PIDs from" << cgProcs.fileName()
                                  << "- bailing out.";
            return QSet<pid_t>();
        }
    }

    return pids;
}
}

QSet<pid_t> DBusFocusInfo::fetchAssociatedPids(pid_t pid)
{
    QSet<pid_t> pids({pid});

    auto cgPath = getCGroupControllerPath(pid);

    if (cgPath.path.isEmpty()) {
        qCDebug(QTMIR_DBUS) << "DBusFocusInfo: pid" << pid << "unable to determine cgroup, assuming is not app-specific.";
        return pids;
    } else if (!cgPath.path.endsWith(".service")) {
        qCDebug(QTMIR_DBUS)
            << "DBusFocusInfo: pid" << pid << "is not in a .service cgroup, "
                "assuming is not app-specific (got" << cgPath.path << ").";
        return pids;
    }

    pids.unite(getPidsInCGroup(cgPath));
    return pids;
}

SessionInterface* DBusFocusInfo::findSessionWithPid(const QSet<pid_t> &pidSet)
{
    Q_FOREACH (Application* application, m_applications) {
        QVector<SessionInterface*> sessions = application->sessions();
        for (auto session : sessions) {
            if (pidSet.contains(session->pid())) {
                return session;
            }
            SessionInterface *chosenChildSession = nullptr;
            session->foreachChildSession([&](SessionInterface* childSession) {
                if (pidSet.contains(childSession->pid())) {
                    chosenChildSession = childSession;
                }
            });
            if (chosenChildSession) {
                return chosenChildSession;
            }
        }
    }
    return nullptr;
}

bool DBusFocusInfo::isSurfaceFocused(const QString &serializedId)
{
    // TODO: Implement a penalty for negative queries, such as stalling for some time before answering
    //       further queries. That's in order to avoid brute-force approaches to find a valid surface id.
    //       That's particularly important for shell's own surface id as it's always valid.
    bool result = false;
    if (serializedId == ShellUuId::toString()) {
        result = true;
    } else {
        MirSurfaceInterface *qmlSurface = findQmlSurface(serializedId);
        result = qmlSurface ? qmlSurface->activeFocus() : false;
    }
    qCDebug(QTMIR_DBUS).nospace() << "DBusFocusInfo: isSurfaceFocused("<<serializedId<<") -> " << result;
    return result;
}

MirSurfaceInterface *DBusFocusInfo::findQmlSurface(const QString &serializedId)
{
    for (Application* application : m_applications) {
        for (SessionInterface *session : application->sessions()) {
            if (session) {
                auto surfaceList = static_cast<MirSurfaceListModel*>(session->surfaceList());
                for (int i = 0; i < surfaceList->count(); ++i) {
                    auto qmlSurface = static_cast<MirSurfaceInterface*>(surfaceList->get(i));
                    if (qmlSurface->persistentId() == serializedId) {
                        return qmlSurface;
                    }
                }

                surfaceList = static_cast<MirSurfaceListModel*>(session->promptSurfaceList());
                for (int i = 0; i < surfaceList->count(); ++i) {
                    auto qmlSurface = static_cast<MirSurfaceInterface*>(surfaceList->get(i));
                    if (qmlSurface->persistentId() == serializedId) {
                        return qmlSurface;
                    }
                }
            }
        }
    }
    return nullptr;
}
