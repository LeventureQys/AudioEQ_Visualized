#include <QtTest/QtTest>
#include "CoordinateMapper.h"

class TestCoordinateMapper : public QObject
{
	Q_OBJECT

private slots:
	void testBasicMapping();
	void testRoundTripPrecision();
	void testBoundaryClamp();
	void testViewportUpdate();
	void testRangeUpdate();
	void testLogarithmicProperty();
};

void TestCoordinateMapper::testBasicMapping()
{
	CoordinateMapper mapper(QRect(0, 0, 1000, 400), 20.0, 20000.0, -48.0, 48.0);

	double x20 = mapper.freqToX(20.0);
	QVERIFY(qAbs(x20 - 0.0) < 0.5);

	double x20000 = mapper.freqToX(20000.0);
	QVERIFY(qAbs(x20000 - 1000.0) < 0.5);

	double x1000 = mapper.freqToX(1000.0);
	QVERIFY(qAbs(x1000 - 566.0) < 1.0);

	double y0 = mapper.gainToY(0.0);
	QVERIFY(qAbs(y0 - 200.0) < 0.5);

	double yNeg48 = mapper.gainToY(-48.0);
	QVERIFY(qAbs(yNeg48 - 400.0) < 0.5);

	double yPos48 = mapper.gainToY(48.0);
	QVERIFY(qAbs(yPos48 - 0.0) < 0.5);
}

void TestCoordinateMapper::testRoundTripPrecision()
{
	CoordinateMapper mapper(QRect(0, 0, 1000, 400), 20.0, 20000.0, -48.0, 48.0);

	double testFreqs[] = { 20.0, 100.0, 1000.0, 10000.0, 20000.0 };

	for (double freq : testFreqs) {
		double x = mapper.freqToX(freq);
		double roundTrip = mapper.xToFreq(x);
		QVERIFY(qAbs(roundTrip - freq) / freq < 1e-6);
	}

	double testGains[] = { -48.0, -24.0, 0.0, 24.0, 48.0 };

	for (double gain : testGains) {
		double y = mapper.gainToY(gain);
		double roundTrip = mapper.yToGain(y);
		QVERIFY(qAbs(roundTrip - gain) < 1e-6);
	}
}

void TestCoordinateMapper::testBoundaryClamp()
{
	CoordinateMapper mapper(QRect(0, 0, 1000, 400), 20.0, 20000.0, -48.0, 48.0);

	double xBelow = mapper.freqToX(1.0);
	QVERIFY(qAbs(xBelow - 0.0) < 0.5);

	double xAbove = mapper.freqToX(30000.0);
	QVERIFY(qAbs(xAbove - 1000.0) < 0.5);

	double yBelow = mapper.gainToY(-60.0);
	QVERIFY(qAbs(yBelow - 400.0) < 0.5);

	double yAbove = mapper.gainToY(60.0);
	QVERIFY(qAbs(yAbove - 0.0) < 0.5);

	double freqBelow = mapper.xToFreq(-10.0);
	QCOMPARE(freqBelow, 20.0);

	double freqAbove = mapper.xToFreq(1010.0);
	QCOMPARE(freqAbove, 20000.0);

	double gainBelow = mapper.yToGain(500.0);
	QCOMPARE(gainBelow, -48.0);

	double gainAbove = mapper.yToGain(-100.0);
	QCOMPARE(gainAbove, 48.0);
}

void TestCoordinateMapper::testViewportUpdate()
{
	CoordinateMapper mapper(QRect(0, 0, 1000, 400), 20.0, 20000.0, -48.0, 48.0);

	double x1000_orig = mapper.freqToX(1000.0);
	double y0_orig = mapper.gainToY(0.0);

	mapper.setViewport(QRect(0, 0, 500, 200));

	double x1000_new = mapper.freqToX(1000.0);
	double y0_new = mapper.gainToY(0.0);

	QVERIFY(qAbs(x1000_new - x1000_orig / 2.0) < 1.0);
	QVERIFY(qAbs(y0_new - y0_orig / 2.0) < 1.0);

	double x20 = mapper.freqToX(20.0);
	QVERIFY(qAbs(x20 - 0.0) < 0.5);

	double x20000 = mapper.freqToX(20000.0);
	QVERIFY(qAbs(x20000 - 500.0) < 0.5);
}

void TestCoordinateMapper::testRangeUpdate()
{
	CoordinateMapper mapper(QRect(0, 0, 1000, 400), 20.0, 20000.0, -48.0, 48.0);

	mapper.setFreqRange(10.0, 40000.0);

	double x10 = mapper.freqToX(10.0);
	QVERIFY(qAbs(x10 - 0.0) < 0.5);

	double x40000 = mapper.freqToX(40000.0);
	QVERIFY(qAbs(x40000 - 1000.0) < 0.5);

	mapper.setGainRange(-24.0, 24.0);

	double yNeg24 = mapper.gainToY(-24.0);
	QVERIFY(qAbs(yNeg24 - 400.0) < 0.5);

	double yPos24 = mapper.gainToY(24.0);
	QVERIFY(qAbs(yPos24 - 0.0) < 0.5);

	double y0 = mapper.gainToY(0.0);
	QVERIFY(qAbs(y0 - 200.0) < 0.5);
}

void TestCoordinateMapper::testLogarithmicProperty()
{
	CoordinateMapper mapper(QRect(0, 0, 1000, 400), 20.0, 20000.0, -48.0, 48.0);

	double diff1 = mapper.freqToX(200.0) - mapper.freqToX(20.0);
	double diff2 = mapper.freqToX(2000.0) - mapper.freqToX(200.0);
	double diff3 = mapper.freqToX(20000.0) - mapper.freqToX(2000.0);

	QVERIFY(qAbs(diff1 - diff2) < 0.01);
	QVERIFY(qAbs(diff2 - diff3) < 0.01);
}

QTEST_MAIN(TestCoordinateMapper)
#include "TestCoordinateMapper.moc"
