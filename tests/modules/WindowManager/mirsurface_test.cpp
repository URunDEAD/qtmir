/*
 * Copyright (C) 2014-2017 Canonical Ltd.
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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

struct MirEvent {}; // otherwise won't compile otherwise due to incomplete type

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QLoggingCategory>
#include <QTest>
#include <QSignalSpy>

// src/common
#include "windowmodelnotifier.h"

// the test subject
#include <QtMir/Application/mirsurface.h>

#include <QtMir/Application/timer.h>
#include <QtMir/Application/mirbuffersgtexture.h>

// mirtest
#include <mir/test/doubles/stub_session.h>
#include <mir/test/doubles/stub_surface.h>

// tests/framework
#include "stub_buffer.h"
#include "stub_windowcontroller.h"
#include "mock_renderable.h"

// tests/modules/common
#include <surfaceobserver.h>

// mir
#include <mir/scene/surface_creation_parameters.h>
#include <mir/version.h>

// miral
#include <miral/window.h>
#include <miral/window_info.h>

using namespace qtmir;

namespace ms = mir::scene;
using StubSurface = mir::test::doubles::StubSurface;
using StubSession = mir::test::doubles::StubSession;

using namespace testing;

struct MockSurface : public StubSurface
{
    MOCK_CONST_METHOD1(buffers_ready_for_compositor, int(void const*));
    MOCK_CONST_METHOD0(visible, bool());
    MOCK_CONST_METHOD0(state, MirWindowState());
    MOCK_CONST_METHOD1(generate_renderables,mir::graphics::RenderableList(mir::compositor::CompositorID id));
};

// A MirBufferSGTexture that keeps the buffer instead of wrapping it in GL state,
// so the texture path can be exercised without a GL context.
struct FakeBufferTexture : MirBufferSGTexture
{
    void setBuffer(const std::shared_ptr<mir::graphics::Buffer>& buffer) override { m_buffer = buffer; }
    void freeBuffer() override { m_buffer.reset(); }
    bool hasBuffer() const override { return m_buffer != nullptr; }

    int textureId() const override { return 1; }
    QSize textureSize() const override { return QSize(1, 1); }
    bool hasAlphaChannel() const override { return false; }
    void bind() override {}

    std::shared_ptr<mir::graphics::Buffer> buffer() const { return m_buffer; }

private:
    std::shared_ptr<mir::graphics::Buffer> m_buffer;
};

QSGTexture *fakeTextureFactory()
{
    return new FakeBufferTexture;
}

namespace {

std::shared_ptr<NiceMock<mir::graphics::MockRenderable>> makeRenderable(
        mir::geometry::Rectangle const& position,
        std::shared_ptr<mir::graphics::Buffer> const& buffer = std::make_shared<mir::graphics::StubBuffer>())
{
    auto renderable = std::make_shared<NiceMock<mir::graphics::MockRenderable>>();
    ON_CALL(*renderable, screen_position()).WillByDefault(Return(position));
    ON_CALL(*renderable, buffer()).WillByDefault(Return(buffer));
    return renderable;
}

} // namespace

class MirSurfaceTest : public ::testing::Test
{
public:
    MirSurfaceTest()
    {
        // We don't want the logging spam cluttering the test results
        QLoggingCategory::setFilterRules(QStringLiteral("qtmir.surfaces=false"));
    }

    const std::shared_ptr<StubSession> stubSession{std::make_shared<StubSession>()};
    const std::shared_ptr<StubSurface> stubSurface{std::make_shared<StubSurface>()};
};

TEST_F(MirSurfaceTest, UpdateTextureBeforeDraw)
{
    int argc = 0;
    char* argv[0];
    QCoreApplication qtApp(argc, argv); // app for deleteLater event

    auto mockSurface = std::make_shared<NiceMock<MockSurface>>();
    miral::Window mockWindow(stubSession, mockSurface);
    ms::SurfaceCreationParameters spec;
    miral::WindowInfo mockWindowInfo(mockWindow, spec);
    auto mockRenderable = std::make_shared<NiceMock<mir::graphics::MockRenderable>>();

    EXPECT_CALL(*mockSurface.get(),buffers_ready_for_compositor(_))
        .WillRepeatedly(Return(1));

    EXPECT_CALL(*mockSurface.get(),generate_renderables(_))
        .WillRepeatedly(Return(mir::graphics::RenderableList{mockRenderable}));

    EXPECT_CALL(*mockRenderable.get(),buffer())
        .WillRepeatedly(Return(std::make_shared<mir::graphics::StubBuffer>()));

    qtmir::MirSurface surface(mockWindowInfo, nullptr);
    surface.setTextureFactory(fakeTextureFactory);
    // Draw once per output, as the frame dropper only cycles outputs it has seen.
    surface.updateTexture((qintptr)123);
    surface.updateTexture((qintptr)124);
#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(0, 30, 0)
    surface.surfaceObserver()->frame_posted(NULL, 1, mir::geometry::Size{1,1});
#else
    surface.surfaceObserver()->frame_posted(1, mir::geometry::Size{1,1});
#endif

    QSignalSpy spyFrameDropped(&surface, SIGNAL(frameDropped()));
    QTest::qWait(300);
    ASSERT_TRUE(spyFrameDropped.count() > 0);
}

/*
 * Two compositors (screens) must not see each other's subsurface state. The third
 * update finds its own gate closed and must still return its own subsurfaces.
 */
TEST_F(MirSurfaceTest, SubSurfaceTexturesArePerCompositorId)
{
    int argc = 0;
    char* argv[0];
    QCoreApplication qtApp(argc, argv);

    auto mockSurface = std::make_shared<NiceMock<MockSurface>>();
    miral::Window mockWindow(stubSession, mockSurface);
    ms::SurfaceCreationParameters spec;
    miral::WindowInfo mockWindowInfo(mockWindow, spec);

    mir::graphics::RenderableList const renderables123{
        makeRenderable({{0, 0}, {100, 100}}),
        makeRenderable({{10, 20}, {30, 40}})
    };
    mir::graphics::RenderableList const renderables124{
        makeRenderable({{0, 0}, {100, 100}}),
        makeRenderable({{50, 60}, {70, 80}})
    };

    EXPECT_CALL(*mockSurface.get(), generate_renderables(_))
        .WillRepeatedly(Invoke([&](mir::compositor::CompositorID id) {
            return id == (mir::compositor::CompositorID)123 ? renderables123 : renderables124;
        }));

    // One pending frame per compositor, then none: without per-id state the third
    // update would hand back whatever the second one stored.
    std::map<void const*, int> updateCount;
    EXPECT_CALL(*mockSurface.get(), buffers_ready_for_compositor(_))
        .WillRepeatedly(Invoke([&updateCount](void const* id) {
            return updateCount[id]++ == 0 ? 1 : 0;
        }));

    qtmir::MirSurface surface(mockWindowInfo, nullptr);
    surface.setTextureFactory(fakeTextureFactory);

    ASSERT_TRUE(surface.updateTexture((qintptr)123));
    auto const v1 = surface.textures((qintptr)123);
    ASSERT_TRUE(surface.updateTexture((qintptr)124));
    auto const v2 = surface.textures((qintptr)124);
    ASSERT_TRUE(surface.updateTexture((qintptr)123));
    auto const v3 = surface.textures((qintptr)123);

    ASSERT_EQ(2u, v1.size());
    ASSERT_EQ(2u, v2.size());
    ASSERT_EQ(2u, v3.size());

    EXPECT_EQ(QRectF(10, 20, 30, 40), v1[1].extent);
    EXPECT_EQ(QRectF(50, 60, 70, 80), v2[1].extent);
    EXPECT_EQ(QRectF(10, 20, 30, 40), v3[1].extent);
    EXPECT_NE(v1[1].texture.data(), v2[1].texture.data());
    EXPECT_EQ(v1[1].texture.data(), v3[1].texture.data());
}

/*
 * updateTexture() creates this output's textures and must leave them alive for the
 * separate textures() call that follows. Nothing outside holds a reference before
 * the first frame.
 */
TEST_F(MirSurfaceTest, FirstUpdateSurvivesUntilTexturesIsCalled)
{
    int argc = 0;
    char* argv[0];
    QCoreApplication qtApp(argc, argv);

    auto mockSurface = std::make_shared<NiceMock<MockSurface>>();
    miral::Window mockWindow(stubSession, mockSurface);
    ms::SurfaceCreationParameters spec;
    miral::WindowInfo mockWindowInfo(mockWindow, spec);

    EXPECT_CALL(*mockSurface.get(), generate_renderables(_))
        .WillRepeatedly(Return(mir::graphics::RenderableList{
            makeRenderable({{0, 0}, {100, 100}}),
            makeRenderable({{10, 20}, {30, 40}})
        }));
    EXPECT_CALL(*mockSurface.get(), buffers_ready_for_compositor(_))
        .WillRepeatedly(Return(1));

    qtmir::MirSurface surface(mockWindowInfo, nullptr);
    surface.setTextureFactory(fakeTextureFactory);

    ASSERT_TRUE(surface.updateTexture((qintptr)123));

    auto const v = surface.textures((qintptr)123);
    ASSERT_EQ(2u, v.size());
    EXPECT_EQ(QRectF(10, 20, 30, 40), v[1].extent);
}

/*
 * A subsurface slot keeps its texture between frames and just swaps the buffer.
 */
TEST_F(MirSurfaceTest, SubSurfaceTexturesAreReusedAcrossFrames)
{
    int argc = 0;
    char* argv[0];
    QCoreApplication qtApp(argc, argv);

    auto mockSurface = std::make_shared<NiceMock<MockSurface>>();
    miral::Window mockWindow(stubSession, mockSurface);
    ms::SurfaceCreationParameters spec;
    miral::WindowInfo mockWindowInfo(mockWindow, spec);

    auto const bufferA = std::make_shared<mir::graphics::StubBuffer>();
    auto const bufferB = std::make_shared<mir::graphics::StubBuffer>();

    auto primaryRenderable = makeRenderable({{0, 0}, {100, 100}});
    auto subRenderable = makeRenderable({{10, 20}, {30, 40}});
    EXPECT_CALL(*subRenderable, buffer())
        .WillOnce(Return(bufferA))
        .WillRepeatedly(Return(bufferB));

    EXPECT_CALL(*mockSurface.get(), generate_renderables(_))
        .WillRepeatedly(Return(mir::graphics::RenderableList{primaryRenderable, subRenderable}));

    EXPECT_CALL(*mockSurface.get(), buffers_ready_for_compositor(_))
        .WillRepeatedly(Return(1));

    qtmir::MirSurface surface(mockWindowInfo, nullptr);
    surface.setTextureFactory(fakeTextureFactory);

    ASSERT_TRUE(surface.updateTexture((qintptr)123));
    auto const v1 = surface.textures((qintptr)123);
    ASSERT_EQ(2u, v1.size());
    auto *sub = dynamic_cast<FakeBufferTexture*>(v1[1].texture.data());
    ASSERT_NE(nullptr, sub);
    EXPECT_EQ(bufferA, sub->buffer());

    ASSERT_TRUE(surface.updateTexture((qintptr)123));
    auto const v2 = surface.textures((qintptr)123);
    ASSERT_EQ(2u, v2.size());
    ASSERT_EQ(v1[1].texture.data(), v2[1].texture.data());
    EXPECT_EQ(bufferB, sub->buffer());
}

/*
 * A vanished subsurface drops its slot, even though no new primary buffer arrived.
 */
TEST_F(MirSurfaceTest, SubSurfaceSlotsShrinkWithRenderables)
{
    int argc = 0;
    char* argv[0];
    QCoreApplication qtApp(argc, argv);

    auto mockSurface = std::make_shared<NiceMock<MockSurface>>();
    miral::Window mockWindow(stubSession, mockSurface);
    ms::SurfaceCreationParameters spec;
    miral::WindowInfo mockWindowInfo(mockWindow, spec);

    mir::graphics::RenderableList renderables{
        makeRenderable({{0, 0}, {100, 100}}),
        makeRenderable({{10, 20}, {30, 40}}),
        makeRenderable({{50, 60}, {70, 80}})
    };

    EXPECT_CALL(*mockSurface.get(), generate_renderables(_))
        .WillRepeatedly(Invoke([&renderables](mir::compositor::CompositorID) { return renderables; }));

    std::map<void const*, int> updateCount;
    EXPECT_CALL(*mockSurface.get(), buffers_ready_for_compositor(_))
        .WillRepeatedly(Invoke([&updateCount](void const* id) {
            return updateCount[id]++ == 0 ? 1 : 0;
        }));

    qtmir::MirSurface surface(mockWindowInfo, nullptr);
    surface.setTextureFactory(fakeTextureFactory);

    ASSERT_TRUE(surface.updateTexture((qintptr)123));
    ASSERT_EQ(3u, surface.textures((qintptr)123).size());

    renderables.resize(1);
    ASSERT_TRUE(surface.updateTexture((qintptr)123));
    auto const v2 = surface.textures((qintptr)123);

    EXPECT_EQ(1u, v2.size());
}

/*
 * Test that MirSurface.visible is recalculated after the client swaps the first frame.
 * A surface is not considered visible unless it has a non-hidden & non-minimized state, and
 * it has drawn at least one frame.
 */
TEST_F(MirSurfaceTest, EnsureVisiblePropertyRecalculatedAfterFrameSwap)
{
    auto mockSurface = std::make_shared<NiceMock<MockSurface>>();
    miral::Window mockWindow(stubSession, mockSurface);
    ms::SurfaceCreationParameters spec;
    miral::WindowInfo mockWindowInfo(mockWindow, spec);

    EXPECT_CALL(*mockSurface.get(),state())
        .WillRepeatedly(Return(mir_window_state_maximized));
    EXPECT_CALL(*mockSurface.get(),visible())
        .WillOnce(Return(false));

    qtmir::MirSurface surface(mockWindowInfo, nullptr);

    EXPECT_FALSE(surface.visible());

    EXPECT_CALL(*mockSurface.get(),visible())
        .WillOnce(Return(true));

    surface.setReady();
    EXPECT_TRUE(surface.visible());
}

/*
 * Test that a surface whose client fails to comply with a close request will eventually comply.
 */
struct MockWindowModelController : public StubWindowModelController
{
    MOCK_METHOD1(requestClose, void(const miral::Window &));
};

TEST_F(MirSurfaceTest, failedSurfaceCloseEventuallyDestroysSurface)
{
    int argc = 0;
    char* argv[0];
    QCoreApplication qtApp(argc, argv); // app for deleteLater event

    miral::Window mockWindow(stubSession, stubSurface);
    ms::SurfaceCreationParameters spec;
    miral::WindowInfo mockWindowInfo(mockWindow, spec);
    MockWindowModelController controller;

    qtmir::MirSurface surface(mockWindowInfo, &controller);

    bool surfaceDeleted = false;
    QObject::connect(&surface, &QObject::destroyed, &surface, [&surfaceDeleted](){ surfaceDeleted = true; });

    QSharedPointer<FakeTimeSource> fakeTimeSource(new FakeTimeSource);
    QPointer<FakeTimer> fakeTimer(new FakeTimer(fakeTimeSource));
    surface.setCloseTimer(fakeTimer.data()); // surface takes ownership of the timer

    qintptr view = (qintptr)1;
    surface.registerView(view);

    EXPECT_CALL(controller, requestClose(_))
        .Times(1);

    surface.close();

    if (fakeTimer->isRunning()) {
        // Simulate that closeTimer has timed out.
        fakeTimeSource->m_msecsSinceReference = fakeTimer->nextTimeoutTime() + 1;
        fakeTimer->update();
    }

    // clean up
    surface.setLive(false);
    surface.unregisterView(view);
}
