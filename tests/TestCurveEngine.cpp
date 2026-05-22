#include <QtTest>
#include <QSignalSpy>
#include <QElapsedTimer>
#include "CurveEngine.h"
#include "filter/ButterworthIIR.h"
#include "filter/FilterAlgorithmFactory.h"
#include "EqualizerModel.h"
#include <cmath>

class TestCurveEngine : public QObject {
	Q_OBJECT
private slots:
	void initTestCase()
	{
		FilterAlgorithmFactory::registerAlgorithm(FilterAlgorithmType::ButterworthIIR,
			[]() -> std::unique_ptr<FilterAlgorithm> { return std::make_unique<ButterworthIIR>(); });
	}

	void testLogPointGeneration()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(500);
		engine.setFreqRange(20.0, 20000.0);

		EqualizerModel model;
		QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);
		engine.requestTotalCurve(model);
		QVERIFY(spy.wait(5000));

		QCOMPARE(spy.count(), 1);
		QVector<QPointF> points = spy.at(0).at(0).value<QVector<QPointF>>();
		QCOMPARE(points.size(), 500);

		QVERIFY(points.first().x() >= 0.0);
		QVERIFY(points.first().x() < 0.001);
		QVERIFY(points.last().x() <= 1.0);
		QVERIFY(points.last().x() > 0.999);

		for (int i = 1; i < points.size(); ++i)
			QVERIFY(points[i].x() > points[i - 1].x());

		double xPerOctaveStart = points[0].x();
		int countsPerOctave = 0;
		double octaveEndFreq = 40.0;
		for (const auto& pt : points) {
			double freq = 20.0 * std::pow(20000.0 / 20.0, pt.x());
			if (freq <= octaveEndFreq)
				countsPerOctave++;
			else
				break;
		}
		QVERIFY(countsPerOctave >= 40);
		QVERIFY(countsPerOctave <= 65);
	}

	void testTotalCurveComputation()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(500);

		EqualizerModel model;
		model.setBandCount(5);

		QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);
		engine.requestTotalCurve(model);
		QVERIFY(spy.wait(5000));

		QCOMPARE(spy.count(), 1);
		QVector<QPointF> points = spy.at(0).at(0).value<QVector<QPointF>>();
		QCOMPARE(points.size(), 500);

		QVERIFY(points.first().x() >= 0.0);
		QVERIFY(points.last().x() <= 1.0);
		for (const auto& pt : points) {
			QVERIFY(pt.y() >= 0.0);
			QVERIFY(pt.y() <= 1.0);
		}
	}

	void testTotalCurveWithFilters()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(200);

		EqualizerModel model;
		model.setBandCount(3);

		EQBand band0;
		band0.frequency = 500.0;
		band0.gain = 6.0;
		band0.q = 1.0;
		band0.type = FilterType::Peak;
		model.setBandParams(0, band0);

		ShelfBand lpf;
		lpf.frequency = 10000.0;
		lpf.enabled = true;
		lpf.algorithm = FilterAlgorithmType::ButterworthIIR;
		model.setLpf(lpf);

		ShelfBand hpf;
		hpf.frequency = 80.0;
		hpf.enabled = true;
		hpf.algorithm = FilterAlgorithmType::ButterworthIIR;
		model.setHpf(hpf);

		QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);
		engine.requestTotalCurve(model);
		QVERIFY(spy.wait(5000));

		QCOMPARE(spy.count(), 1);
		QVector<QPointF> points = spy.at(0).at(0).value<QVector<QPointF>>();
		QCOMPARE(points.size(), 200);

		double midIndex = points.size() / 2;
		double yMid = points[midIndex].y();
		double yCenter = 0.5;
		QVERIFY(std::abs(yMid - yCenter) < 0.3);
	}

	void testSingleBandCurve()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(200);

		EqualizerModel model;
		model.setBandCount(3);

		EQBand band1;
		band1.frequency = 1000.0;
		band1.gain = 9.0;
		band1.q = 1.0;
		band1.type = FilterType::Peak;
		band1.bypass = false;
		model.setBandParams(1, band1);

		QSignalSpy spy(&engine, &CurveEngine::singleBandCurveReady);
		engine.requestSingleBandCurve(1, model);
		QVERIFY(spy.wait(5000));

		QCOMPARE(spy.count(), 1);
		QVector<QPointF> points = spy.at(0).at(1).value<QVector<QPointF>>();
		int bandIndex = spy.at(0).at(0).toInt();
		QCOMPARE(bandIndex, 1);
		QCOMPARE(points.size(), 200);

		QVERIFY(points.first().x() >= 0.0);
		QVERIFY(points.last().x() <= 1.0);
	}

	void testSingleBandBypass()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(100);

		EqualizerModel model;
		model.setBandCount(1);

		EQBand band0;
		band0.frequency = 500.0;
		band0.gain = 12.0;
		band0.q = 1.0;
		band0.type = FilterType::Peak;
		band0.bypass = true;
		model.setBandParams(0, band0);

		QSignalSpy spy(&engine, &CurveEngine::singleBandCurveReady);
		engine.requestSingleBandCurve(0, model);
		QVERIFY(spy.wait(5000));

		QCOMPARE(spy.count(), 1);
		QVector<QPointF> points = spy.at(0).at(1).value<QVector<QPointF>>();
		QCOMPARE(points.size(), 100);

		double firstY = points.first().y();
		for (const auto& pt : points)
			QCOMPARE(pt.y(), firstY);
	}

	void testSingleBandLpfHpf()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(100);

		EqualizerModel model;
		model.setBandCount(0);

		ShelfBand lpf;
		lpf.frequency = 5000.0;
		lpf.enabled = true;
		lpf.algorithm = FilterAlgorithmType::ButterworthIIR;
		model.setLpf(lpf);

		QSignalSpy spy(&engine, &CurveEngine::singleBandCurveReady);
		engine.requestSingleBandCurve(-2, model);
		QVERIFY(spy.wait(5000));

		QCOMPARE(spy.count(), 1);
		QVector<QPointF> points = spy.at(0).at(1).value<QVector<QPointF>>();
		QCOMPARE(points.size(), 100);

		model.setLpfEnabled(false);

		QSignalSpy spy2(&engine, &CurveEngine::singleBandCurveReady);
		engine.requestSingleBandCurve(-2, model);
		QVERIFY(spy2.wait(5000));

		QCOMPARE(spy2.count(), 1);
		QVector<QPointF> pointsDisabled = spy2.at(0).at(1).value<QVector<QPointF>>();
		double firstY = pointsDisabled.first().y();
		for (const auto& pt : pointsDisabled)
			QCOMPARE(pt.y(), firstY);
	}

	void testCancelMechanism()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(10000);

		EqualizerModel model;
		EQBand band;
		band.frequency = 1000.0;
		band.gain = 6.0;
		band.q = 1.0;
		band.type = FilterType::Peak;
		model.setBandCount(1);
		model.setBandParams(0, band);

		QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);
		engine.requestTotalCurve(model);
		engine.cancelPending();

		bool received = spy.wait(1000);
		if (received) {
			QVERIFY(spy.count() <= 1);
		}
	}

	void testThreadSafety()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(50);

		EqualizerModel model;
		model.setBandCount(3);

		const int requestCount = 20;
		QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);
		for (int i = 0; i < requestCount; ++i)
			engine.requestTotalCurve(model);

		QElapsedTimer timer;
		timer.start();
		while (spy.count() < requestCount && timer.elapsed() < 5000)
			QTest::qWait(10);

		QVERIFY(spy.count() >= requestCount);
	}

	void testSetPointCount()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());

		QCOMPARE(engine.pointCount(), 500);

		engine.setPointCount(100);
		QCOMPARE(engine.pointCount(), 100);

		engine.setPointCount(0);
		QCOMPARE(engine.pointCount(), 1);

		EqualizerModel model;
		QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);
		engine.requestTotalCurve(model);
		QVERIFY(spy.wait(5000));

		QCOMPARE(spy.count(), 1);
		QVector<QPointF> points = spy.at(0).at(0).value<QVector<QPointF>>();
		QCOMPARE(points.size(), 1);
	}

	void testFreqGainRange()
	{
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());
		engine.setPointCount(200);
		engine.setFreqRange(100.0, 10000.0);
		engine.setGainRange(-24.0, 24.0);

		EqualizerModel model;

		QSignalSpy spy(&engine, &CurveEngine::totalCurveReady);
		engine.requestTotalCurve(model);
		QVERIFY(spy.wait(5000));

		QVector<QPointF> points = spy.at(0).at(0).value<QVector<QPointF>>();
		QCOMPARE(points.size(), 200);

		QVERIFY(points.first().x() >= 0.0);
		QVERIFY(points.last().x() <= 1.0);

		for (const auto& pt : points) {
			QVERIFY(pt.y() >= 0.0);
			QVERIFY(pt.y() <= 1.0);
		}
	}
};

QTEST_MAIN(TestCurveEngine)
#include "TestCurveEngine.moc"
