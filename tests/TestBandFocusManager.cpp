#include <QtTest>
#include <QSignalSpy>
#include "BandFocusManager.h"
#include "BandHandle.h"

class TestBandFocusManager : public QObject {
    Q_OBJECT
private slots:
    void testInitialNoFocus();
    void testRegisterAndFocus();
    void testFocusChange();
    void testClearFocus();
    void testMultipleBands();
};

void TestBandFocusManager::testInitialNoFocus()
{
    BandFocusManager mgr;
    QCOMPARE(mgr.focusedIndex(), -1);
}

void TestBandFocusManager::testRegisterAndFocus()
{
    BandFocusManager mgr;
    QWidget parent;
    BandHandle h0(0, &parent);
    BandHandle h1(1, &parent);

    mgr.registerBand(&h0);
    mgr.registerBand(&h1);

    QSignalSpy spy(&mgr, &BandFocusManager::focusChanged);
    mgr.setFocusedIndex(0);

    QCOMPARE(mgr.focusedIndex(), 0);
    QVERIFY(h0.isFocused());
    QVERIFY(!h1.isFocused());
    QCOMPARE(spy.count(), 1);
}

void TestBandFocusManager::testFocusChange()
{
    BandFocusManager mgr;
    QWidget parent;
    BandHandle h0(0, &parent);
    BandHandle h1(1, &parent);

    mgr.registerBand(&h0);
    mgr.registerBand(&h1);

    mgr.setFocusedIndex(0);
    mgr.setFocusedIndex(1);

    QCOMPARE(mgr.focusedIndex(), 1);
    QVERIFY(!h0.isFocused());
    QVERIFY(h1.isFocused());
}

void TestBandFocusManager::testClearFocus()
{
    BandFocusManager mgr;
    QWidget parent;
    BandHandle h0(0, &parent);
    BandHandle h1(1, &parent);

    mgr.registerBand(&h0);
    mgr.registerBand(&h1);
    mgr.setFocusedIndex(0);

    mgr.clearFocus();

    QCOMPARE(mgr.focusedIndex(), -1);
    QVERIFY(!h0.isFocused());
    QVERIFY(!h1.isFocused());
}

void TestBandFocusManager::testMultipleBands()
{
    BandFocusManager mgr;
    QWidget parent;
    QVector<BandHandle*> handles;

    for (int i = 0; i < 5; ++i) {
        BandHandle* h = new BandHandle(i, &parent);
        handles.append(h);
        mgr.registerBand(h);
    }

    for (int i = 0; i < 5; ++i) {
        QSignalSpy spy(&mgr, &BandFocusManager::focusChanged);
        mgr.setFocusedIndex(i);

        QCOMPARE(mgr.focusedIndex(), i);
        QCOMPARE(spy.count(), 1);

        for (int j = 0; j < 5; ++j) {
            if (j == i)
                QVERIFY(handles[j]->isFocused());
            else
                QVERIFY(!handles[j]->isFocused());
        }
    }
}

QTEST_MAIN(TestBandFocusManager)
#include "TestBandFocusManager.moc"
