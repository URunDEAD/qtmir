/*
 * Copyright (C) 2024 UBports Foundation
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

#include <gtest/gtest.h>

// the test subject
#include <QtMir/Application/mirsurfaceitem.h>

using namespace qtmir;

namespace {

const qreal tolerance = 1e-6;

void expectRect(const QRectF &actual, qreal x, qreal y, qreal w, qreal h)
{
    EXPECT_NEAR(actual.x(), x, tolerance);
    EXPECT_NEAR(actual.y(), y, tolerance);
    EXPECT_NEAR(actual.width(), w, tolerance);
    EXPECT_NEAR(actual.height(), h, tolerance);
}

} // namespace

// Cases 1-3 pin the formulas MirSurfaceItem::updatePaintNode used before the node tree
// was unified, so the primary's mapping is unchanged for a surface at offset (0,0).

TEST(MirSurfaceItemGeometry, StretchMapsPrimaryOntoWholeItem)
{
    MirSurfaceItem::TexturePlacement placement;
    ASSERT_TRUE(MirSurfaceItem::computeTexturePlacement(QRectF(0, 0, 400, 300),
                                                        QRectF(0, 0, 400, 300),
                                                        QSize(400, 300),
                                                        QRectF(0, 0, 200, 150),
                                                        MirSurfaceItem::Stretch,
                                                        &placement));
    expectRect(placement.targetRect, 0, 0, 200, 150);
    expectRect(placement.subSourceRect, 0, 0, 1, 1);
}

TEST(MirSurfaceItemGeometry, PadOrCropCropsPrimaryToItem)
{
    MirSurfaceItem::TexturePlacement placement;
    ASSERT_TRUE(MirSurfaceItem::computeTexturePlacement(QRectF(0, 0, 400, 300),
                                                        QRectF(0, 0, 400, 300),
                                                        QSize(400, 300),
                                                        QRectF(0, 0, 200, 150),
                                                        MirSurfaceItem::PadOrCrop,
                                                        &placement));
    expectRect(placement.targetRect, 0, 0, 200, 150);
    expectRect(placement.subSourceRect, 0, 0, 0.5, 0.5);
}

TEST(MirSurfaceItemGeometry, PadOrCropDoesNotUpscaleASmallTexture)
{
    MirSurfaceItem::TexturePlacement placement;
    ASSERT_TRUE(MirSurfaceItem::computeTexturePlacement(QRectF(0, 0, 400, 300),
                                                        QRectF(0, 0, 400, 300),
                                                        QSize(100, 100),
                                                        QRectF(0, 0, 200, 150),
                                                        MirSurfaceItem::PadOrCrop,
                                                        &placement));
    expectRect(placement.targetRect, 0, 0, 100, 100);
    expectRect(placement.subSourceRect, 0, 0, 1, 1);
}

TEST(MirSurfaceItemGeometry, SubsurfaceOverflowingBottomRightIsCropped)
{
    MirSurfaceItem::TexturePlacement placement;
    ASSERT_TRUE(MirSurfaceItem::computeTexturePlacement(QRectF(380, 280, 40, 40),
                                                        QRectF(0, 0, 400, 300),
                                                        QSize(400, 300),
                                                        QRectF(0, 0, 400, 300),
                                                        MirSurfaceItem::Stretch,
                                                        &placement));
    expectRect(placement.targetRect, 380, 280, 20, 20);
    expectRect(placement.subSourceRect, 0, 0, 0.5, 0.5);
}

// A CSD shadow at a negative offset. The only case producing a non-zero
// subSourceRect origin.
TEST(MirSurfaceItemGeometry, SubsurfaceOverflowingTopLeftOffsetsTheSource)
{
    MirSurfaceItem::TexturePlacement placement;
    ASSERT_TRUE(MirSurfaceItem::computeTexturePlacement(QRectF(-20, -10, 40, 30),
                                                        QRectF(0, 0, 400, 300),
                                                        QSize(400, 300),
                                                        QRectF(0, 0, 400, 300),
                                                        MirSurfaceItem::Stretch,
                                                        &placement));
    expectRect(placement.targetRect, 0, 0, 20, 20);
    expectRect(placement.subSourceRect, 0.5, 1.0 / 3.0, 0.5, 2.0 / 3.0);
}

TEST(MirSurfaceItemGeometry, SubsurfaceScalesWithThePrimaryInStretch)
{
    MirSurfaceItem::TexturePlacement placement;
    ASSERT_TRUE(MirSurfaceItem::computeTexturePlacement(QRectF(100, 50, 80, 60),
                                                        QRectF(0, 0, 400, 300),
                                                        QSize(400, 300),
                                                        QRectF(0, 0, 200, 150),
                                                        MirSurfaceItem::Stretch,
                                                        &placement));
    expectRect(placement.targetRect, 50, 25, 40, 30);
    expectRect(placement.subSourceRect, 0, 0, 1, 1);
}

TEST(MirSurfaceItemGeometry, FullyOffscreenSubsurfaceGetsNoNode)
{
    MirSurfaceItem::TexturePlacement placement;
    EXPECT_FALSE(MirSurfaceItem::computeTexturePlacement(QRectF(500, 500, 40, 40),
                                                         QRectF(0, 0, 400, 300),
                                                         QSize(400, 300),
                                                         QRectF(0, 0, 400, 300),
                                                         MirSurfaceItem::Stretch,
                                                         &placement));
}

TEST(MirSurfaceItemGeometry, DegenerateInputsAreRejected)
{
    MirSurfaceItem::TexturePlacement placement;

    // Empty extent
    EXPECT_FALSE(MirSurfaceItem::computeTexturePlacement(QRectF(0, 0, 0, 0),
                                                         QRectF(0, 0, 400, 300),
                                                         QSize(400, 300),
                                                         QRectF(0, 0, 400, 300),
                                                         MirSurfaceItem::Stretch,
                                                         &placement));
    // Empty item
    EXPECT_FALSE(MirSurfaceItem::computeTexturePlacement(QRectF(0, 0, 400, 300),
                                                         QRectF(0, 0, 400, 300),
                                                         QSize(400, 300),
                                                         QRectF(0, 0, 0, 0),
                                                         MirSurfaceItem::Stretch,
                                                         &placement));
    // Empty primary extent: would divide by zero
    EXPECT_FALSE(MirSurfaceItem::computeTexturePlacement(QRectF(0, 0, 400, 300),
                                                         QRectF(0, 0, 0, 0),
                                                         QSize(400, 300),
                                                         QRectF(0, 0, 400, 300),
                                                         MirSurfaceItem::Stretch,
                                                         &placement));
    // Empty texture, PadOrCrop: would divide by zero
    EXPECT_FALSE(MirSurfaceItem::computeTexturePlacement(QRectF(0, 0, 400, 300),
                                                         QRectF(0, 0, 400, 300),
                                                         QSize(0, 0),
                                                         QRectF(0, 0, 400, 300),
                                                         MirSurfaceItem::PadOrCrop,
                                                         &placement));
}
