#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>
#include "CurveEngine.h"
#include "EqualizerModel.h"
#include "AudioEQTypes.h"

class CurveEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!QCoreApplication::instance()) {
            static int argc = 0;
            static char* argv[] = {nullptr};
            m_app = new QCoreApplication(argc, argv);
        }
    }

    void TearDown() override {
        delete m_app;
        m_app = nullptr;
    }

    CurveRequest makeRequest() {
        CurveRequest req;
        EQBand b1, b2, b3;
        b1.freqHz = 250.0;  b1.gainDb = 3.0;  b1.q = 1.0; b1.type = FilterType::Peak;
        b2.freqHz = 1000.0; b2.gainDb = -2.0; b2.q = 2.0; b2.type = FilterType::Peak;
        b3.freqHz = 4000.0; b3.gainDb = 1.0;  b3.q = 0.7; b3.type = FilterType::Peak;
        req.bands = {b1, b2, b3};
        req.sampleRate = SampleRate::SR_44100;
        req.freqMin = 20.0;
        req.freqMax = 22050.0;
        req.pointCount = 500;
        return req;
    }

    QCoreApplication* m_app = nullptr;
};

TEST_F(CurveEngineTest, BasicAsyncRequest) {
    CurveEngine engine;
    QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);

    engine.requestTotalCurve(makeRequest());
    spy.wait(3000);

    ASSERT_EQ(spy.count(), 1);
    auto points = spy[0][0].value<QVector<QPointF>>();
    EXPECT_EQ(points.size(), 500);
    EXPECT_NEAR(points.first().x(), 0.0, 0.01);
    EXPECT_NEAR(points.last().x(), 1.0, 0.01);
}

TEST_F(CurveEngineTest, RapidCancelation) {
    CurveEngine engine;
    QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);

    for (int i = 0; i < 10; ++i) {
        engine.requestTotalCurve(makeRequest());
    }

    spy.wait(5000);

    EXPECT_LE(spy.count(), 2);
}

TEST_F(CurveEngineTest, BandCurveRequest) {
    CurveEngine engine;
    QSignalSpy spy(&engine, &CurveEngine::bandCurveReady);

    engine.requestBandCurve(makeRequest(), 0);
    spy.wait(3000);

    ASSERT_GE(spy.count(), 1);
    auto points = spy[0][1].value<QVector<QPointF>>();
    EXPECT_EQ(points.size(), 500);
}

TEST_F(CurveEngineTest, DestructorSafety) {
    {
        CurveEngine engine;
        engine.requestTotalCurve(makeRequest());
    }
    SUCCEED();
}

TEST_F(CurveEngineTest, PointCount) {
    CurveEngine engine;
    engine.setPointCount(100);
    QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);

    engine.requestTotalCurve(makeRequest());
    spy.wait(3000);

    ASSERT_EQ(spy.count(), 1);
    auto points = spy[0][0].value<QVector<QPointF>>();
    EXPECT_EQ(points.size(), 100);
}
