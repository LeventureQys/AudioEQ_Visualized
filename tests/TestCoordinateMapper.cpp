#include <gtest/gtest.h>
#include "CoordinateMapper.h"
#include <cmath>

TEST(CoordinateMapper, FreqToBoundary) {
    CoordinateMapper mapper;
    mapper.setViewport(QRectF(0, 0, 1000, 600));
    mapper.setSampleRate(SampleRate::SR_44100);
    EXPECT_NEAR(mapper.freqToX(20.0), 0.0, 0.01);
    EXPECT_NEAR(mapper.freqToX(22050.0), 1000.0, 0.01);
}

TEST(CoordinateMapper, GainToBoundary) {
    CoordinateMapper mapper;
    mapper.setViewport(QRectF(0, 0, 1000, 600));
    mapper.setGainRange(-48.0, 48.0);
    EXPECT_NEAR(mapper.gainToY(48.0), 0.0, 0.01);
    EXPECT_NEAR(mapper.gainToY(-48.0), 600.0, 0.01);
    EXPECT_NEAR(mapper.gainToY(0.0), 300.0, 0.01);
}

TEST(CoordinateMapper, RoundTrip) {
    CoordinateMapper mapper;
    mapper.setViewport(QRectF(0, 0, 1000, 600));
    mapper.setSampleRate(SampleRate::SR_44100);
    EXPECT_NEAR(mapper.xToFreq(mapper.freqToX(1000.0)), 1000.0, 0.01);
    EXPECT_NEAR(mapper.yToGain(mapper.gainToY(0.0)), 0.0, 0.001);
    EXPECT_NEAR(mapper.yToGain(mapper.gainToY(-12.0)), -12.0, 0.001);
}

TEST(CoordinateMapper, SampleRateUpdate) {
    CoordinateMapper mapper;
    mapper.setViewport(QRectF(0, 0, 1000, 600));
    mapper.setSampleRate(SampleRate::SR_44100);
    EXPECT_NEAR(mapper.freqMax(), 22050.0, 0.01);
    mapper.setSampleRate(SampleRate::SR_48000);
    EXPECT_NEAR(mapper.freqMax(), 24000.0, 0.01);
}

TEST(CoordinateMapper, GainRangeUpdate) {
    CoordinateMapper mapper;
    mapper.setViewport(QRectF(0, 0, 1000, 600));
    mapper.setGainRange(-24.0, 24.0);
    EXPECT_NEAR(mapper.gainToY(24.0), 0.0, 0.01);
    EXPECT_NEAR(mapper.gainToY(-24.0), 600.0, 0.01);
}

TEST(CoordinateMapper, ToPixel) {
    CoordinateMapper mapper;
    mapper.setViewport(QRectF(0, 0, 1000, 600));
    mapper.setSampleRate(SampleRate::SR_44100);
    mapper.setGainRange(-48.0, 48.0);
    QPointF p = mapper.toPixel(1000.0, 0.0);
    EXPECT_NEAR(p.x(), mapper.freqToX(1000.0), 0.01);
    EXPECT_NEAR(p.y(), mapper.gainToY(0.0), 0.01);
}
