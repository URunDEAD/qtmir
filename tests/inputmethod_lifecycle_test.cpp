/* SPDX-License-Identifier: LGPL-3.0-or-later */
#include "session.h"
#include "untrackedclientcommand.h"
#include "fake_mirsurface.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QSignalSpy>
#include <iostream>
#include <cstdlib>

static void check(bool result, const char *message)
{
    if (!result) { std::cerr << message << '\n'; std::exit(1); }
}

class InputMethodSurface : public qtmir::FakeMirSurface
{
public:
    Mir::Type type() const override { return Mir::InputMethodType; }
};

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    // No Mir session is needed: this exercises only surface registration/list
    // bookkeeping, and no name(), lifecycle IPC or prompt manager operations.
    QLoggingCategory::setFilterRules(QStringLiteral("qtmir.*=false"));
    for (const char *command : { "maliit-server", "/usr/bin/maliit-server",
                                "maliit6-server", "/opt/example/bin/maliit-keyboard",
                                "maliit-keyboard", "/usr/bin/maliit-keyboard",
                                "/usr/lib/qt5/libexec/QtWebProcess" }) {
        check(qtmir::isUntrackedClientCommand(QString::fromUtf8(command)),
              "Keyboard services must bypass ordinary application tracking");
    }
    for (const char *command : { "terminal", "/usr/bin/evil-maliit-keyboard",
                                "/usr/bin/maliit-keyboard-helper",
                                "/usr/lib/qt5/libexec/QtWebProcessHelper" }) {
        check(!qtmir::isUntrackedClientCommand(QString::fromUtf8(command)),
              "Ordinary application command must keep lifecycle tracking");
    }
    qtmir::Session session({}, {});
    auto *inputMethod = new InputMethodSurface;
    QSignalSpy inputReady(inputMethod, &qtmir::MirSurfaceInterface::ready);
    QSignalSpy inputDestroyed(inputMethod, &QObject::destroyed);
    QSignalSpy sessionEmpty(session.surfaceList(), &qtmir::MirSurfaceListModel::emptyChanged);
    session.registerSurface(inputMethod);
    inputMethod->setReady();
    check(inputReady.count() == 1, "Dedicated input-method observers must still receive ready");
    check(session.surfaceList()->isEmpty() && !session.hadSurface(),
          "Input method must not enter the application surface lifecycle");
    check(session.state() == qtmir::SessionInterface::Starting,
          "Input method readiness must not start the ordinary surface lifecycle");
    delete inputMethod;
    check(inputDestroyed.count() == 1 && sessionEmpty.count() == 0 && !session.hadSurface(),
          "Input method hide/destroy must not look like losing the last application surface");

    auto *normal = new qtmir::FakeMirSurface;
    session.registerSurface(normal);
    check(session.surfaceList()->isEmpty(), "Ordinary surfaces still wait for their first frame");
    normal->setReady();
    check(session.surfaceList()->count() == 1 && session.hadSurface()
          && session.state() == qtmir::SessionInterface::Running,
          "Ordinary ready surface must retain normal application lifecycle behavior");
    auto *secondInput = new InputMethodSurface;
    session.registerSurface(secondInput);
    secondInput->setReady();
    delete secondInput;
    check(session.surfaceList()->count() == 1 && session.surfaceList()->first() == normal,
          "Hiding an input method must preserve an ordinary surface in the same session");
    delete normal;
    check(session.surfaceList()->isEmpty() && session.hadSurface(),
          "Last ordinary surface destruction must retain existing close detection");
    std::cout << "Input-method lifecycle regression passed\n";
}
