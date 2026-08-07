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

#ifndef MIRSURFACEITEM_H
#define MIRSURFACEITEM_H

#include <memory>
#include <optional>

// Qt
#include <QMutex>
#include <QRectF>
#include <QTimer>

// Lomiri Shell API
#include <lomiri/shell/application/MirSurfaceItemInterface.h>

#include "mirsurfaceinterface.h"
#include "session_interface.h"

namespace qtmir {

class MirSurfaceItem : public lomiri::shell::application::MirSurfaceItemInterface
{
    Q_OBJECT

public:
    explicit MirSurfaceItem(QQuickItem *parent = 0);
    virtual ~MirSurfaceItem();

    ////////
    // MirSurfaceItemInterface

    Mir::Type type() const override;
    QString name() const override;
    bool live() const override;
    Mir::ShellChrome shellChrome() const override;

    Mir::State surfaceState() const override;

    Mir::OrientationAngle orientationAngle() const override;
    void setOrientationAngle(Mir::OrientationAngle angle) override;

    lomiri::shell::application::MirSurfaceInterface* surface() const override;
    void setSurface(lomiri::shell::application::MirSurfaceInterface*) override;

    bool consumesInput() const override;
    void setConsumesInput(bool value) override;

    int surfaceWidth() const override;
    void setSurfaceWidth(int value) override;

    int surfaceHeight() const override;
    void setSurfaceHeight(int value) override;

    FillMode fillMode() const override;
    void setFillMode(FillMode value) override;

    ////////
    // own API

    // to allow easy touch event injection from tests
    bool processTouchEvent(int eventType,
            ulong timestamp,
            Qt::KeyboardModifiers modifiers,
            const QList<QTouchEvent::TouchPoint> &touchPoints,
            Qt::TouchPointStates touchPointStates);

public Q_SLOTS:
    // Called by QQuickWindow from the rendering thread
    void invalidateSceneGraph();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void touchEvent(QTouchEvent *event) override;

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

    void releaseResources() override;

private Q_SLOTS:
    void scheduleMirSurfaceSizeUpdate();
    void updateMirSurfaceSize();

    void updateMirSurfaceActiveFocus();
    void updateMirSurfaceExposure();

    void onActualSurfaceSizeChanged(QSize size);

    void onWindowChanged(QQuickWindow *window);

private:
    bool hasTouchInsideInputRegion(const QList<QTouchEvent::TouchPoint> &touchPoints);

    QString appId() const;
    void endCurrentTouchSequence(ulong timestamp);
    void validateAndDeliverTouchEvent(int eventType,
            ulong timestamp,
            Qt::KeyboardModifiers modifiers,
            const QList<QTouchEvent::TouchPoint> &touchPoints,
            Qt::TouchPointStates touchPointStates);

    void setSurfaceLocked(lomiri::shell::application::MirSurfaceInterface *lomiriShellSurface);

    MirSurfaceInterface* m_surface;
    QQuickWindow* m_window;

    QMutex m_mutex;

    // Guards m_subSurfaceTextures. Deliberately NOT m_mutex: releaseResources() and
    // ~MirSurfaceItem run on the GUI thread and cannot take m_mutex (setSurfaceLocked
    // re-enters QML under it).
    QMutex m_textureMutex;

    QTimer m_updateMirSurfaceSizeTimer;

    class TouchEvent {
    public:
        TouchEvent &operator= (const QTouchEvent &qtEvent) {
            type = qtEvent.type();
            timestamp = qtEvent.timestamp();
            modifiers = qtEvent.modifiers();
            touchPoints = qtEvent.touchPoints();
            touchPointStates = qtEvent.touchPointStates();
            return *this;
        }

        void updateTouchPointStatesAndType();

        int type;
        ulong timestamp;
        Qt::KeyboardModifiers modifiers;
        QList<QTouchEvent::TouchPoint> touchPoints;
        Qt::TouchPointStates touchPointStates;
    } *m_lastTouchEvent;

    std::optional<unsigned int> m_lastFrameNumberRendered;

    int m_surfaceWidth;
    int m_surfaceHeight;
    Mir::OrientationAngle *m_orientationAngle;

    bool m_consumesInput;

    FillMode m_fillMode;

    // The textures drawn by the last updatePaintNode(), entry 0 being the surface's
    // own. Held so they outlive the nodes pointing at them.
    qtmir::MirSurfaceInterface::SubSurfaceTextures m_subSurfaceTextures;
};

} // namespace qtmir

#endif // MIRSURFACEITEM_H
