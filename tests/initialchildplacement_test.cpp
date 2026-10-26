#include "initialchildplacement.h"
#include <iostream>
#include <cstdlib>

static void check(bool result, const char *message)
{
    if (!result) { std::cerr << message << '\n'; std::exit(1); }
}

int main()
{
    using qtmir::InitialChildPlacement;
    const QSize parentSize(372, 804), placeholder(640, 480), actual(200, 120);
    const QPoint parent(-134, 116), initialOffset(-134, 108), actualOffset(86, 228);
    QPoint target;
    InitialChildPlacement placement;
    auto remember = [&](bool eligible = true) {
        placement.remember(eligible, parent + initialOffset, placeholder, parent, parentSize);
    };
    remember();
    check(placement.takePosition(parent + initialOffset, actual, parent, parentSize, target),
          "changed automatic child size must correct first placement");
    check(target - parent == actualOffset, "640x480 -> 200x120 must yield relative(86,228)");
    check(!placement.takePosition(parent + initialOffset, actual, parent, parentSize, target),
          "placement must only be consumed once");
    remember();
    check(!placement.takePosition(parent + initialOffset, placeholder, parent, parentSize, target),
          "unchanged size must keep position");
    remember();
    check(!placement.takePosition(parent + initialOffset + QPoint(1, 2), actual, parent, parentSize, target),
          "moved child must keep manual position");
    remember();
    placement.cancel();
    check(!placement.takePosition(parent + initialOffset, actual, parent, parentSize, target),
          "explicit move cancels even if moved back to initial position");
    remember(false);
    check(!placement.takePosition(parent + initialOffset, actual, parent, parentSize, target),
          "excluded popup or positioning-hint path must keep placement");
    placement.remember(true, parent + QPoint(12, 34), placeholder, parent, parentSize);
    check(!placement.takePosition(parent + QPoint(12, 34), actual, parent, parentSize, target),
          "nonautomatic initial placement must be preserved");
    remember();
    const QPoint movedParent(70, -90);
    check(placement.takePosition(movedParent + initialOffset, actual, movedParent, parentSize, target),
          "inherited parent translation must not cancel automatic placement");
    check(target == movedParent + actualOffset, "current parent global offset must be added once");
    remember();
    check(!placement.takePosition(parent + initialOffset, QSize(0, 120), parent, parentSize, target),
          "zero child dimensions must be rejected");
    std::cout << "Initial child placement regression passed\n";
}
