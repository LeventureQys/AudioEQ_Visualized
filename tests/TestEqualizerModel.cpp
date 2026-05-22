#include <QtTest/QtTest>
#include <QSignalSpy>
#include "EqualizerModel.h"

class TestEqualizerModel : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultBandCount()
    {
        EqualizerModel model;
        QCOMPARE(model.bandCount(), 5);
    }

    void testDefaultFrequencies()
    {
        EqualizerModel model;
        auto bands = model.allBands();
        QCOMPARE(bands.size(), 5);

        for (int i = 0; i < 5; ++i) {
            double expectedFreq = 20.0 * std::pow(20000.0 / 20.0, double(i) / 4.0);
            QVERIFY(qAbs(bands[i].frequency - expectedFreq) / expectedFreq < 1e-6);
        }

        QVERIFY(qAbs(bands[0].frequency - 20.0) < 0.01);
        QVERIFY(qAbs(bands[4].frequency - 20000.0) < 0.01);
    }

    void testAddBand()
    {
        EqualizerModel model;
        int outIndex = -1;
        EQBand band;
        band.frequency = 500.0;
        band.gain = 3.0;
        band.q = 2.0;

        ResultCode rc = model.addBand(band, &outIndex);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(outIndex, 5);
        QCOMPARE(model.bandCount(), 6);

        const EQBand& added = model.bandAt(outIndex);
        QCOMPARE(added.frequency, 500.0);
        QCOMPARE(added.gain, 3.0);
        QCOMPARE(added.q, 2.0);
    }

    void testRemoveBand()
    {
        EqualizerModel model;
        QCOMPARE(model.bandCount(), 5);

        ResultCode rc = model.removeBand(0);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(model.bandCount(), 4);

        rc = model.removeBand(999);
        QCOMPARE(rc, ResultCode::IndexOutOfRange);
    }

    void testSetBandParams()
    {
        EqualizerModel model;
        EQBand params;
        params.frequency = 2500.0;
        params.gain = 6.0;
        params.q = 3.0;
        params.type = FilterType::LowShelf;
        params.bypass = true;

        ResultCode rc = model.setBandParams(0, params);
        QCOMPARE(rc, ResultCode::OK);

        const EQBand& band = model.bandAt(0);
        QCOMPARE(band.frequency, 2500.0);
        QCOMPARE(band.gain, 6.0);
        QCOMPARE(band.q, 3.0);
        QCOMPARE(band.type, FilterType::LowShelf);
        QCOMPARE(band.bypass, true);

        rc = model.setBandParams(999, params);
        QCOMPARE(rc, ResultCode::IndexOutOfRange);
    }

    void testFocusManagement()
    {
        EqualizerModel model;

        QCOMPARE(model.focusedBandIndex(), -1);

        model.setFocusedBandIndex(0);
        QCOMPARE(model.focusedBandIndex(), 0);

        model.setFocusedBandIndex(2);
        QCOMPARE(model.focusedBandIndex(), 2);

        model.setFocusedBandIndex(-1);
        QCOMPARE(model.focusedBandIndex(), -1);

        model.setFocusedBandIndex(999);
        QCOMPARE(model.focusedBandIndex(), -1);
    }

    void testSampleRate()
    {
        EqualizerModel model;

        QCOMPARE(model.sampleRate(), SampleRate::SR_44100);
        QCOMPARE(model.nyquistFrequency(), 22050.0);

        model.setSampleRate(SampleRate::SR_48000);
        QCOMPARE(model.sampleRate(), SampleRate::SR_48000);
        QCOMPARE(model.nyquistFrequency(), 24000.0);

        model.setSampleRate(SampleRate::SR_96000);
        QCOMPARE(model.sampleRate(), SampleRate::SR_96000);
        QCOMPARE(model.nyquistFrequency(), 48000.0);
    }

    void testLpfHpf()
    {
        EqualizerModel model;

        ShelfBand defaultLpf = model.lpf();
        QCOMPARE(defaultLpf.enabled, false);
        QCOMPARE(defaultLpf.frequency, 20000.0);

        ShelfBand lpf;
        lpf.frequency = 10000.0;
        lpf.enabled = true;
        lpf.algorithm = FilterAlgorithmType::ButterworthIIR;

        ResultCode rc = model.setLpf(lpf);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(model.lpf().frequency, 10000.0);
        QCOMPARE(model.lpf().enabled, true);

        rc = model.setLpfEnabled(false);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(model.lpf().enabled, false);

        ShelfBand hpf;
        hpf.frequency = 100.0;
        hpf.enabled = true;
        rc = model.setHpf(hpf);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(model.hpf().frequency, 100.0);
        QCOMPARE(model.hpf().enabled, true);

        rc = model.setHpfEnabled(false);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(model.hpf().enabled, false);
    }

    void testSignals()
    {
        EqualizerModel model;

        {
            QSignalSpy spy(&model, &EqualizerModel::bandCountChanged);
            QSignalSpy spyReset(&model, &EqualizerModel::modelReset);
            model.setBandCount(3);
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spy.at(0).at(0).toInt(), 3);
            QCOMPARE(spyReset.count(), 1);
        }

        {
            QSignalSpy spy(&model, &EqualizerModel::bandAdded);
            QSignalSpy spyCount(&model, &EqualizerModel::bandCountChanged);
            EQBand band;
            band.frequency = 1000.0;
            model.addBand(band);
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spyCount.count(), 1);
        }

        {
            QSignalSpy spy(&model, &EqualizerModel::bandRemoved);
            model.removeBand(1);
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spy.at(0).at(0).toInt(), 1);
        }

        {
            QSignalSpy spy(&model, &EqualizerModel::bandChanged);
            EQBand params;
            params.frequency = 2000.0;
            params.gain = 0.0;
            params.q = 1.0;
            params.type = FilterType::Peak;
            params.algorithm = FilterAlgorithmType::ButterworthIIR;
            params.bypass = false;
            model.setBandParams(0, params);
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spy.at(0).at(0).toInt(), 0);
        }

        {
            QSignalSpy spy(&model, &EqualizerModel::focusedBandChanged);
            model.setFocusedBandIndex(0);
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spy.at(0).at(0).toInt(), 0);
        }

        {
            QSignalSpy spy(&model, &EqualizerModel::sampleRateChanged);
            model.setSampleRate(SampleRate::SR_48000);
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spy.at(0).at(0).value<SampleRate>(), SampleRate::SR_48000);
        }

        {
            QSignalSpy spy(&model, &EqualizerModel::lpfChanged);
            model.setLpfEnabled(true);
            QCOMPARE(spy.count(), 1);
        }

        {
            QSignalSpy spy(&model, &EqualizerModel::hpfChanged);
            model.setHpfEnabled(true);
            QCOMPARE(spy.count(), 1);
        }
    }

    void testRemoveFocusedBand()
    {
        EqualizerModel model;
        model.setFocusedBandIndex(2);
        QCOMPARE(model.focusedBandIndex(), 2);

        model.removeBand(2);
        QCOMPARE(model.focusedBandIndex(), -1);
    }
};

QTEST_MAIN(TestEqualizerModel)
#include "TestEqualizerModel.moc"
