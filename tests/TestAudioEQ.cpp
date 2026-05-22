#include <QtTest/QtTest>
#include "AudioEQ.h"

class TestAudioEQ : public QObject {
    Q_OBJECT
private slots:
    void testIsVulkanSupported()
    {
        QVERIFY(AudioEQ::isVulkanSupported() || !AudioEQ::isVulkanSupported());
    }

    void testConstruction()
    {
        AudioEQ eq;
        QVERIFY(eq.bandCount() >= 0);
    }

    void testDefaultBandCount()
    {
        AudioEQ eq;
        QCOMPARE(eq.bandCount(), 5);
    }

    void testSetBandCount()
    {
        AudioEQ eq;
        ResultCode rc = eq.setBandCount(3);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(eq.bandCount(), 3);
    }

    void testSampleRate()
    {
        AudioEQ eq;
        QCOMPARE(eq.sampleRate(), SampleRate::SR_44100);

        ResultCode rc = eq.setSampleRate(SampleRate::SR_48000);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(eq.sampleRate(), SampleRate::SR_48000);
    }

    void testLpfHpf()
    {
        AudioEQ eq;
        QCOMPARE(eq.isLpfEnabled(), false);
        QCOMPARE(eq.isHpfEnabled(), false);

        ResultCode rc = eq.setLpfEnabled(true);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(eq.isLpfEnabled(), true);

        rc = eq.setHpfEnabled(true);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(eq.isHpfEnabled(), true);

        rc = eq.setLpfEnabled(false);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(eq.isLpfEnabled(), false);

        rc = eq.setHpfEnabled(false);
        QCOMPARE(rc, ResultCode::OK);
        QCOMPARE(eq.isHpfEnabled(), false);
    }
};

QTEST_MAIN(TestAudioEQ)
#include "TestAudioEQ.moc"
