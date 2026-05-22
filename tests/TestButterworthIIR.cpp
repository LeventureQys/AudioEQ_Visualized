#include <QtTest>
#include "filter/ButterworthIIR.h"
#include <cmath>

class TestButterworthIIR : public QObject {
	Q_OBJECT
private slots:
	void testPeakGain()
	{
		ButterworthIIR algo;
		EQBand band;
		band.type = FilterType::Peak;
		band.frequency = 1000.0;
		band.q = 1.0;
		band.gain = 6.0;

		double gainAtCenter = algo.evaluateAt(1000.0, 44100.0, band);
		QVERIFY(std::abs(gainAtCenter - 6.0) < 0.01);

		double gainFarAway = algo.evaluateAt(100.0, 44100.0, band);
		QVERIFY(std::abs(gainFarAway - 0.0) < 0.5);
	}

	void testLowShelfGain()
	{
		ButterworthIIR algo;
		EQBand band;
		band.type = FilterType::LowShelf;
		band.frequency = 500.0;
		band.q = 0.7;
		band.gain = -3.0;

		double gainLow = algo.evaluateAt(20.0, 44100.0, band);
		QVERIFY(std::abs(gainLow - (-3.0)) < 0.5);

		double gainHigh = algo.evaluateAt(5000.0, 44100.0, band);
		QVERIFY(std::abs(gainHigh - 0.0) < 0.5);
	}

	void testLowPass()
	{
		ButterworthIIR algo;
		EQBand band;
		band.type = FilterType::LowPass;
		band.frequency = 1000.0;

		double gainPassband = algo.evaluateAt(100.0, 44100.0, band);
		QVERIFY(std::abs(gainPassband - 0.0) < 0.1);

		double gainStopband = algo.evaluateAt(4000.0, 44100.0, band);
		QVERIFY(gainStopband < -3.0);
	}

	void testHighPass()
	{
		ButterworthIIR algo;
		EQBand band;
		band.type = FilterType::HighPass;
		band.frequency = 1000.0;

		double gainPassband = algo.evaluateAt(4000.0, 44100.0, band);
		QVERIFY(std::abs(gainPassband - 0.0) < 0.1);

		double gainStopband = algo.evaluateAt(100.0, 44100.0, band);
		QVERIFY(gainStopband < -3.0);
	}

	void testQRange()
	{
		ButterworthIIR algo;

		auto peakRange = algo.qRange(FilterType::Peak);
		QCOMPARE(peakRange.first, 0.4);
		QCOMPARE(peakRange.second, 128.0);

		auto shelfRange = algo.qRange(FilterType::LowShelf);
		QCOMPARE(shelfRange.first, 0.4);
		QCOMPARE(shelfRange.second, 1.6);

		auto hsRange = algo.qRange(FilterType::HighShelf);
		QCOMPARE(hsRange.first, 0.4);
		QCOMPARE(hsRange.second, 1.6);

		auto lpRange = algo.qRange(FilterType::LowPass);
		QCOMPARE(lpRange.first, 0.4);
		QCOMPARE(lpRange.second, 128.0);

		auto hpRange = algo.qRange(FilterType::HighPass);
		QCOMPARE(hpRange.first, 0.4);
		QCOMPARE(hpRange.second, 128.0);

		auto bpRange = algo.qRange(FilterType::BandPass);
		QCOMPARE(bpRange.first, 0.4);
		QCOMPARE(bpRange.second, 128.0);
	}

	void testEvaluateAtPureFunction()
	{
		ButterworthIIR algo;
		EQBand band;
		band.type = FilterType::Peak;
		band.frequency = 500.0;
		band.q = 2.0;
		band.gain = -4.0;

		double r1 = algo.evaluateAt(500.0, 44100.0, band);
		double r2 = algo.evaluateAt(500.0, 44100.0, band);
		double r3 = algo.evaluateAt(500.0, 44100.0, band);

		QCOMPARE(r1, r2);
		QCOMPARE(r2, r3);
	}

	void testBypass()
	{
		ButterworthIIR algo;
		EQBand band;
		band.type = FilterType::Peak;
		band.frequency = 1000.0;
		band.q = 1.0;
		band.gain = 12.0;
		band.bypass = true;

		double result = algo.evaluateAt(1000.0, 44100.0, band);
		QCOMPARE(result, 0.0);

		result = algo.evaluateAt(100.0, 44100.0, band);
		QCOMPARE(result, 0.0);
	}
};

QTEST_MAIN(TestButterworthIIR)
#include "TestButterworthIIR.moc"
