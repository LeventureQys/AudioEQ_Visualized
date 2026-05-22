#include <QtTest/QtTest>
#include "AudioEQ.h"
#include "AudioEQTypes.h"
#include <memory>

class TestCompatibility : public QObject {
    Q_OBJECT
private slots:
    void testMultipleInstances()
    {
        auto eq1 = std::make_unique<AudioEQ>();
        auto eq2 = std::make_unique<AudioEQ>();
        auto eq3 = std::make_unique<AudioEQ>();

        QCOMPARE(eq1->bandCount(), 5);
        QCOMPARE(eq2->bandCount(), 5);
        QCOMPARE(eq3->bandCount(), 5);

        ResultCode rc1 = eq1->setBandCount(3);
        ResultCode rc2 = eq2->setBandCount(7);
        ResultCode rc3 = eq3->setBandCount(10);
        QCOMPARE(rc1, ResultCode::OK);
        QCOMPARE(rc2, ResultCode::OK);
        QCOMPARE(rc3, ResultCode::OK);

        QCOMPARE(eq1->bandCount(), 3);
        QCOMPARE(eq2->bandCount(), 7);
        QCOMPARE(eq3->bandCount(), 10);

        EQBand band1;
        band1.frequency = 500.0;
        band1.gain = 3.0;
        band1.q = 1.5;
        band1.type = FilterType::LowShelf;
        int idx1 = -1;
        eq1->addBand(band1, &idx1);

        EQBand band2;
        band2.frequency = 8000.0;
        band2.gain = -5.0;
        band2.q = 0.7;
        band2.type = FilterType::HighShelf;
        int idx2 = -1;
        eq2->addBand(band2, &idx2);

        QCOMPARE(eq1->bandCount(), 4);
        QCOMPARE(eq2->bandCount(), 8);
        QCOMPARE(eq3->bandCount(), 10);

        EQBand read1 = eq1->bandParams(idx1);
        QCOMPARE(read1.frequency, 500.0);
        QCOMPARE(read1.gain, 3.0);

        EQBand read2 = eq2->bandParams(idx2);
        QCOMPARE(read2.frequency, 8000.0);
        QCOMPARE(read2.gain, -5.0);

        rc1 = eq1->setBandCount(2);
        QCOMPARE(rc1, ResultCode::OK);
        QCOMPARE(eq1->bandCount(), 2);

        QCOMPARE(eq2->bandCount(), 8);
        QCOMPARE(eq3->bandCount(), 10);
    }

    void testMultipleSampleRates()
    {
        AudioEQ eq;

        struct RateEntry {
            SampleRate rate;
            int intValue;
        };
        RateEntry entries[] = {
            { SampleRate::SR_44100,  44100  },
            { SampleRate::SR_48000,  48000  },
            { SampleRate::SR_96000,  96000  },
            { SampleRate::SR_192000, 192000 },
        };

        for (const auto& entry : entries) {
            ResultCode rc = eq.setSampleRate(entry.rate);
            QCOMPARE(rc, ResultCode::OK);
            QCOMPARE(eq.sampleRate(), entry.rate);
            QCOMPARE(static_cast<int>(entry.rate), entry.intValue);
        }
    }

    void testEnumValuesComplete()
    {
        FilterType filterTypes[] = {
            FilterType::Peak,
            FilterType::LowShelf,
            FilterType::HighShelf,
            FilterType::LowPass,
            FilterType::HighPass,
            FilterType::BandPass,
        };
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

        ResultCode resultCodes[] = {
            ResultCode::OK,
            ResultCode::Failed,
            ResultCode::IndexOutOfRange,
            ResultCode::IndexConflict,
            ResultCode::InvalidParameter,
            ResultCode::VulkanNotAvailable,
        };
        QCOMPARE(static_cast<int>(ResultCode::OK), 0);
        QVERIFY(static_cast<int>(ResultCode::Failed) != 0);
        QVERIFY(static_cast<int>(ResultCode::IndexOutOfRange) != 0);
        QVERIFY(static_cast<int>(ResultCode::IndexConflict) != 0);
        QVERIFY(static_cast<int>(ResultCode::InvalidParameter) != 0);
        QVERIFY(static_cast<int>(ResultCode::VulkanNotAvailable) != 0);

        QCOMPARE(static_cast<int>(SampleRate::SR_44100), 44100);
        QCOMPARE(static_cast<int>(SampleRate::SR_48000), 48000);
        QCOMPARE(static_cast<int>(SampleRate::SR_96000), 96000);
        QCOMPARE(static_cast<int>(SampleRate::SR_192000), 192000);
    }

    void testBandOperations()
    {
        AudioEQ eq;
        int count = eq.bandCount();

        int outIndex = -1;
        EQBand b1;
        b1.frequency = 1000.0;
        b1.gain = 5.0;
        b1.q = 2.0;
        b1.type = FilterType::Peak;
        ResultCode rc = eq.addBand(b1, &outIndex);
        QCOMPARE(rc, ResultCode::OK);
        QVERIFY(outIndex >= 0);
        QCOMPARE(eq.bandCount(), count + 1);

        EQBand read = eq.bandParams(outIndex);
        QCOMPARE(read.frequency, 1000.0);
        QCOMPARE(read.gain, 5.0);
        QCOMPARE(read.q, 2.0);
        QCOMPARE(read.type, FilterType::Peak);

        EQBand update;
        update.frequency = 2000.0;
        update.gain = -3.0;
        update.q = 4.0;
        update.type = FilterType::LowShelf;
        update.bypass = true;
        rc = eq.setBandParams(outIndex, update);
        QCOMPARE(rc, ResultCode::OK);

        read = eq.bandParams(outIndex);
        QCOMPARE(read.frequency, 2000.0);
        QCOMPARE(read.gain, -3.0);
        QCOMPARE(read.q, 4.0);
        QCOMPARE(read.type, FilterType::LowShelf);
        QCOMPARE(read.bypass, true);

        rc = eq.removeBand(outIndex);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(eq.bandCount(), count);

        rc = eq.setBandCount(0);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(eq.bandCount(), 0);

        for (int i = 0; i < 100; ++i) {
            EQBand band;
            band.frequency = 100.0 + i * 10.0;
            band.gain = (i % 5) - 2.0;
            band.q = 0.5 + (i % 3) * 0.5;
            int idx = -1;
            rc = eq.addBand(band, &idx);
            QCOMPARE(rc, ResultCode::OK);
            QVERIFY(idx >= 0);
        }
        QCOMPARE(eq.bandCount(), 100);

        for (int i = 99; i >= 0; --i) {
            eq.removeBand(i);
        }
        QCOMPARE(eq.bandCount(), 0);

        for (int i = 0; i < 100; ++i) {
            EQBand band;
            band.frequency = 500.0 + i * 20.0;
            band.gain = 0.0;
            band.q = 1.0;
            int idx = -1;
            eq.addBand(band, &idx);
            QCOMPARE(eq.bandCount(), i + 1);
        }
        for (int i = 99; i >= 0; --i) {
            eq.removeBand(i);
            QCOMPARE(eq.bandCount(), i);
        }
    }

    void testResetAndRecreate()
    {
        for (int iter = 0; iter < 5; ++iter) {
            auto eq = std::make_unique<AudioEQ>();

            QCOMPARE(eq->bandCount(), 5);
            QCOMPARE(eq->sampleRate(), SampleRate::SR_44100);
            QCOMPARE(eq->isLpfEnabled(), false);
            QCOMPARE(eq->isHpfEnabled(), false);

            eq->setBandCount(3);
            eq->setSampleRate(SampleRate::SR_96000);
            eq->setLpfEnabled(true);
            eq->setHpfEnabled(true);

            QCOMPARE(eq->bandCount(), 3);
            QCOMPARE(eq->sampleRate(), SampleRate::SR_96000);
            QCOMPARE(eq->isLpfEnabled(), true);
            QCOMPARE(eq->isHpfEnabled(), true);

            EQBand b;
            b.frequency = 2000.0;
            b.gain = 3.0;
            b.q = 2.0;
            b.type = FilterType::HighShelf;
            int idx = -1;
            eq->addBand(b, &idx);
            QCOMPARE(eq->bandCount(), 4);

            eq->setBandParams(idx, b);
            EQBand read = eq->bandParams(idx);
            QCOMPARE(read.frequency, 2000.0);
            QCOMPARE(read.gain, 3.0);

            eq.reset();
        }

        QVERIFY(true);
    }
};

QTEST_MAIN(TestCompatibility)
#include "TestCompatibility.moc"
