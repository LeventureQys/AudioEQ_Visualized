#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QMouseEvent>
#include "BandHandle.h"

class TestBandHandle : public QObject
{
    Q_OBJECT

private slots:
    void testBandIndex();
    void testFocus();
    void testMousePress();
    void testDragEmit();
};

void TestBandHandle::testBandIndex()
{
    BandHandle handle(3);
    QCOMPARE(handle.bandIndex(), 3);
}

void TestBandHandle::testFocus()
{
    BandHandle handle(0);
    QVERIFY(!handle.isFocused());
    handle.setFocused(true);
    QVERIFY(handle.isFocused());
    handle.setFocused(false);
    QVERIFY(!handle.isFocused());
}

void TestBandHandle::testMousePress()
{
    BandHandle handle(0);
    QSignalSpy spy(&handle, &BandHandle::bandPressed);

    QPoint center(12, 12);
    QTest::mousePress(&handle, Qt::LeftButton, Qt::NoModifier, center);

    QCOMPARE(spy.count(), 1);
    QVERIFY(handle.isDragging());
}

void TestBandHandle::testDragEmit()
{
    BandHandle handle(0);
    QSignalSpy spy(&handle, &BandHandle::bandDragged);

    QPoint center(12, 12);
    QTest::mousePress(&handle, Qt::LeftButton, Qt::NoModifier, center);
    QVERIFY(handle.isDragging());

    QTest::mouseMove(&handle, QPoint(20, 8));
    QTest::qWait(50);

    QTest::mouseRelease(&handle, Qt::LeftButton, Qt::NoModifier, QPoint(20, 8));

    QVERIFY(!handle.isDragging());
    QVERIFY(spy.count() > 0);
}

QTEST_MAIN(TestBandHandle)
#include "TestBandHandle.moc"
