#ifndef QTMIR_UNTRACKEDCLIENTCOMMAND_H
#define QTMIR_UNTRACKEDCLIENTCOMMAND_H
#include <QRegularExpression>
#include <QString>
namespace qtmir {
inline bool isUntrackedClientCommand(const QString &command)
{
    // Keyboard services and WebKit helpers are not independently managed apps.
    static const QRegularExpression pattern(QStringLiteral(
        "((^|/)(maliit6?-server|maliit-keyboard)|qt5/libexec/QtWebProcess)$"));
    return pattern.match(command).hasMatch();
}
}
#endif
