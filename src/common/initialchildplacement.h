/* Initial automatic placement of a child before its first real buffer. */
#ifndef QTMIR_INITIALCHILDPLACEMENT_H
#define QTMIR_INITIALCHILDPLACEMENT_H

#include <QPoint>
#include <QSize>

namespace qtmir {
class InitialChildPlacement
{
public:
    static QPoint centeredOffset(QSize parent, QSize child)
    {
        // Match MirAL's optical centre, including its integer rounding.
        const int dx = parent.width() - child.width();
        const int dy = parent.height() - child.height();
        return QPoint(dx / 2, dy / 2 - dy / 6);
    }

    void remember(bool eligible, QPoint childPosition, QSize childSize,
                  QPoint parentPosition, QSize parentSize)
    {
        initialSize = childSize;
        initialOffset = childPosition - parentPosition;
        pending = eligible && positive(childSize) && positive(parentSize)
            && initialOffset == centeredOffset(parentSize, childSize);
    }

    void cancel() { pending = false; }

    bool takePosition(QPoint childPosition, QSize childSize,
                      QPoint parentPosition, QSize parentSize, QPoint &position)
    {
        const bool wasPending = pending;
        pending = false; // Only the first ready notification may adjust placement.
        if (!wasPending || childSize == initialSize || !positive(childSize)
            || !positive(parentSize) || childPosition - parentPosition != initialOffset)
            return false;
        position = parentPosition + centeredOffset(parentSize, childSize);
        return true;
    }

private:
    static bool positive(QSize size) { return size.width() > 0 && size.height() > 0; }
    bool pending{false};
    QSize initialSize;
    QPoint initialOffset;
};
}
#endif
