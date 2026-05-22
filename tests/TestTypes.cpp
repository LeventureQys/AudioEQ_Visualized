#include <QtTest>
#include "../src/AudioEQTypes.h"

class TestTypes : public QObject
{
    Q_OBJECT

private slots:
    void testEQBandDefaults()
    {
        EQBand band;
        QCOMPARE(band.index, -1);
        QCOMPARE(band.frequency, 1000.0);
        QCOMPARE(band.gain, 0.0);
        QCOMPARE(band.q, 1.0);
        QCOMPARE(band.type, FilterType::Peak);
        QCOMPARE(band.algorithm, FilterAlgorithmType::ButterworthIIR);
        QCOMPARE(band.bypass, false);
    }

    void testShelfBandDefaults()
    {
        ShelfBand shelf;
        QCOMPARE(shelf.frequency, 20000.0);
        QCOMPARE(shelf.enabled, false);
        QCOMPARE(shelf.algorithm, FilterAlgorithmType::ButterworthIIR);
    }

    void testSampleRateValues()
    {
        QCOMPARE(static_cast<int>(SampleRate::SR_44100),  44100);
        QCOMPARE(static_cast<int>(SampleRate::SR_48000),  48000);
        QCOMPARE(static_cast<int>(SampleRate::SR_96000),  96000);
        QCOMPARE(static_cast<int>(SampleRate::SR_192000), 192000);
    }

    void testResultCodeValues()
    {
        QCOMPARE(static_cast<int>(ResultCode::OK), 0);
        QVERIFY(static_cast<int>(ResultCode::Failed) != 0);
        QVERIFY(static_cast<int>(ResultCode::IndexOutOfRange) != 0);
        QVERIFY(static_cast<int>(ResultCode::InvalidParameter) != 0);
        QVERIFY(static_cast<int>(ResultCode::VulkanNotAvailable) != 0);
    }

    void testEQBandAssignAndCompare()
    {
        EQBand a;
        a.index = 3;
        a.frequency = 2000.0;
        a.gain = 6.0;
        a.q = 2.5;
        a.type = FilterType::HighShelf;
        a.algorithm = FilterAlgorithmType::ButterworthIIR;
        a.bypass = true;

        QCOMPARE(a.index, 3);
        QCOMPARE(a.frequency, 2000.0);
        QCOMPARE(a.gain, 6.0);
        QCOMPARE(a.q, 2.5);
        QCOMPARE(a.type, FilterType::HighShelf);
        QCOMPARE(a.algorithm, FilterAlgorithmType::ButterworthIIR);
        QCOMPARE(a.bypass, true);

        EQBand b = a;
        QCOMPARE(b.index, a.index);
        QCOMPARE(b.frequency, a.frequency);
        QCOMPARE(b.gain, a.gain);
        QCOMPARE(b.q, a.q);
        QCOMPARE(b.type, a.type);
        QCOMPARE(b.algorithm, a.algorithm);
        QCOMPARE(b.bypass, a.bypass);
    }

    void testFilterTypeValuesNotEqual()
    {
        QVERIFY(FilterType::Peak != FilterType::LowShelf);
        QVERIFY(FilterType::Peak != FilterType::HighShelf);
        QVERIFY(FilterType::Peak != FilterType::LowPass);
        QVERIFY(FilterType::Peak != FilterType::HighPass);
        QVERIFY(FilterType::Peak != FilterType::BandPass);

        QVERIFY(FilterType::LowShelf != FilterType::HighShelf);
        QVERIFY(FilterType::LowShelf != FilterType::LowPass);
        QVERIFY(FilterType::LowShelf != FilterType::HighPass);
        QVERIFY(FilterType::LowShelf != FilterType::BandPass);

        QVERIFY(FilterType::HighShelf != FilterType::LowPass);
        QVERIFY(FilterType::HighShelf != FilterType::HighPass);
        QVERIFY(FilterType::HighShelf != FilterType::BandPass);

        QVERIFY(FilterType::LowPass != FilterType::HighPass);
        QVERIFY(FilterType::LowPass != FilterType::BandPass);

        QVERIFY(FilterType::HighPass != FilterType::BandPass);
    }
};

QTEST_MAIN(TestTypes)
#include "TestTypes.moc"
