#include <QtTest>
#include "filter/FilterAlgorithm.h"
#include "filter/FilterAlgorithmFactory.h"
#include <cmath>

class MockAlgorithm : public FilterAlgorithm {
public:
	FilterAlgorithmType type() const override { return FilterAlgorithmType::ButterworthIIR; }
	double evaluateAt(double freqHz, double sampleRate, const EQBand& band) const override {
		if (sampleRate <= 0) return 0.0;
		return freqHz / sampleRate * band.gain;
	}
	QPair<double, double> qRange(FilterType) const override { return {0.4, 128.0}; }
};

class TestFilterAlgorithm : public QObject {
	Q_OBJECT
private slots:
	void testQRangeDefault()
	{
		MockAlgorithm algo;
		auto range = algo.qRange(FilterType::Peak);
		QCOMPARE(range.first, 0.4);
		QCOMPARE(range.second, 128.0);
	}

	void testFactoryRegister()
	{
		FilterAlgorithmFactory::registerAlgorithm(FilterAlgorithmType::ButterworthIIR,
			[]() -> std::unique_ptr<FilterAlgorithm> {
				return std::make_unique<MockAlgorithm>();
			});

		auto algo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
		QVERIFY(algo != nullptr);
		QCOMPARE(algo->type(), FilterAlgorithmType::ButterworthIIR);

		auto range = algo->qRange(FilterType::Peak);
		QCOMPARE(range.first, 0.4);
		QCOMPARE(range.second, 128.0);
	}

	void testFactoryUnknown()
	{
		auto algo = FilterAlgorithmFactory::create(static_cast<FilterAlgorithmType>(999));
		QVERIFY(algo == nullptr);
	}

	void testEvaluateAt()
	{
		MockAlgorithm algo;
		EQBand band;
		band.gain = 6.0;
		double result = algo.evaluateAt(1000.0, 44100.0, band);
		double expected = 1000.0 / 44100.0 * 6.0;
		QVERIFY(std::abs(result - expected) < 1e-9);
	}
};

QTEST_MAIN(TestFilterAlgorithm)
#include "TestFilterAlgorithm.moc"
