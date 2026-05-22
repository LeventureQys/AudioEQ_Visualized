#include <QtTest>
#include <QSignalSpy>
#include "SampleRateManager.h"
#include "EqualizerModel.h"
#include "CurveEngine.h"
#include "CoordinateMapper.h"
#include "filter/ButterworthIIR.h"
#include "filter/FilterAlgorithmFactory.h"

class TestSampleRateManager : public QObject {
	Q_OBJECT
private slots:
	void initTestCase()
	{
		FilterAlgorithmFactory::registerAlgorithm(FilterAlgorithmType::ButterworthIIR,
			[]() -> std::unique_ptr<FilterAlgorithm> { return std::make_unique<ButterworthIIR>(); });
	}

	void testDefaultSampleRate()
	{
		EqualizerModel model;
		CoordinateMapper mapper(QRect(0, 0, 1000, 400), 10.0, 22050.0, -48.0, 48.0);
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());

		SampleRateManager mgr(&model, &engine, &mapper);
		QCOMPARE(mgr.sampleRate(), SampleRate::SR_44100);
	}

	void testSwitchSampleRate()
	{
		EqualizerModel model;
		CoordinateMapper mapper(QRect(0, 0, 1000, 400), 10.0, 22050.0, -48.0, 48.0);
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());

		SampleRateManager mgr(&model, &engine, &mapper);
		mgr.setSampleRate(SampleRate::SR_96000);

		QCOMPARE(mgr.sampleRate(), SampleRate::SR_96000);
		QCOMPARE(model.sampleRate(), SampleRate::SR_96000);
		QCOMPARE(mapper.freqMax(), 48000.0);
	}

	void testNyquistUpdate()
	{
		EqualizerModel model;
		CoordinateMapper mapper(QRect(0, 0, 1000, 400), 10.0, 22050.0, -48.0, 48.0);
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());

		SampleRateManager mgr(&model, &engine, &mapper);
		mgr.setSampleRate(SampleRate::SR_96000);
		QCOMPARE(mgr.nyquistFrequency(), 48000.0);

		mgr.setSampleRate(SampleRate::SR_192000);
		QCOMPARE(mgr.nyquistFrequency(), 96000.0);
	}

	void testSignalEmission()
	{
		EqualizerModel model;
		CoordinateMapper mapper(QRect(0, 0, 1000, 400), 10.0, 22050.0, -48.0, 48.0);
		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		CurveEngine engine(algo.get());

		SampleRateManager mgr(&model, &engine, &mapper);

		QSignalSpy spyRate(&mgr, &SampleRateManager::sampleRateChanged);
		QSignalSpy spyNyquist(&mgr, &SampleRateManager::nyquistChanged);

		mgr.setSampleRate(SampleRate::SR_48000);

		QCOMPARE(spyRate.count(), 1);
		QCOMPARE(spyNyquist.count(), 1);
		QCOMPARE(spyRate.at(0).at(0).value<SampleRate>(), SampleRate::SR_48000);
		QCOMPARE(spyNyquist.at(0).at(0).toDouble(), 24000.0);
	}
};

QTEST_MAIN(TestSampleRateManager)
#include "TestSampleRateManager.moc"
