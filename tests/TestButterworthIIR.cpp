#include <gtest/gtest.h>
#include "filter/ButterworthIIR.h"
#include "filter/FilterAlgorithmFactory.h"
#include "AudioEQTypes.h"
#include <cmath>

TEST(ButterworthIIR, PeakAtCenter) {
    ButterworthIIR iir(FilterType::Peak, 1000.0, 6.0, 1.0);
    EXPECT_NEAR(iir.evaluateAt(1000.0, 44100.0), 6.0, 0.1);
}

TEST(ButterworthIIR, PeakAwayFromCenter) {
    ButterworthIIR iir(FilterType::Peak, 1000.0, 6.0, 1.0);
    EXPECT_LT(std::abs(iir.evaluateAt(4000.0, 44100.0)), 1.0);
    EXPECT_LT(std::abs(iir.evaluateAt(250.0, 44100.0)), 1.0);
}

TEST(ButterworthIIR, PeakZeroGain) {
    ButterworthIIR iir(FilterType::Peak, 1000.0, 0.0, 1.0);
    EXPECT_NEAR(iir.evaluateAt(1000.0, 44100.0), 0.0, 0.001);
    EXPECT_NEAR(iir.evaluateAt(500.0, 44100.0), 0.0, 0.001);
    EXPECT_NEAR(iir.evaluateAt(2000.0, 44100.0), 0.0, 0.001);
}

TEST(ButterworthIIR, LowShelf) {
    ButterworthIIR iir(FilterType::LowShelf, 200.0, 6.0, 0.707);
    EXPECT_NEAR(iir.evaluateAt(10.0, 44100.0), 6.0, 0.5);
    EXPECT_NEAR(iir.evaluateAt(5000.0, 44100.0), 0.0, 0.5);
}

TEST(ButterworthIIR, HighShelf) {
    ButterworthIIR iir(FilterType::HighShelf, 8000.0, 6.0, 0.707);
    EXPECT_NEAR(iir.evaluateAt(20000.0, 44100.0), 6.0, 0.5);
    EXPECT_NEAR(iir.evaluateAt(100.0, 44100.0), 0.0, 0.5);
}

TEST(ButterworthIIR, Factory) {
    auto iir = FilterAlgorithmFactory::create(FilterType::Peak, FilterAlgorithmType::ButterworthIIR, 1000.0, 0.0, 1.0);
    EXPECT_NE(iir, nullptr);
    auto iir2 = FilterAlgorithmFactory::create(FilterType::LowShelf, FilterAlgorithmType::ButterworthIIR, 200.0, 3.0, 0.7);
    EXPECT_NE(iir2, nullptr);
    auto iir3 = FilterAlgorithmFactory::create(FilterType::HighShelf, FilterAlgorithmType::ButterworthIIR, 8000.0, -3.0, 0.7);
    EXPECT_NE(iir3, nullptr);
}

TEST(ButterworthIIR, SampleRateIndependence) {
    ButterworthIIR iir(FilterType::Peak, 1000.0, 6.0, 1.0);
    EXPECT_NEAR(iir.evaluateAt(1000.0, 44100.0), 6.0, 0.2);
    EXPECT_NEAR(iir.evaluateAt(1000.0, 48000.0), 6.0, 0.2);
    EXPECT_NEAR(iir.evaluateAt(1000.0, 96000.0), 6.0, 0.2);
}
