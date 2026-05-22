#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QMouseEvent>
#include "LpfHandle.h"
#include "HpfHandle.h"

class TestLpfHpfHandle : public QObject
{
    Q_OBJECT

private slots:
    void testLpfDefaultDisabled();
    void testLpfEnable();
    void testHpfDefaultDisabled();
    void testHpfEnable();
    void testLpfMousePress();
    void testHpfHorizontalOnly();
    void testLpfPaint();
};

void TestLpfHpfHandle::testLpfDefaultDisabled()
{
    LpfHandle handle;
    QVERIFY(!handle.isEnabled());
}

void TestLpfHpfHandle::testLpfEnable()
{
    LpfHandle handle;
    handle.setEnabled(true);
    QVERIFY(handle.isEnabled());
}

void TestLpfHpfHandle::testHpfDefaultDisabled()
{
    HpfHandle handle;
    QVERIFY(!handle.isEnabled());
}

void TestLpfHpfHandle::testHpfEnable()
{
    HpfHandle handle;
    handle.setEnabled(true);
    QVERIFY(handle.isEnabled());
}

void TestLpfHpfHandle::testLpfMousePress()
{
    LpfHandle handle;
    QSignalSpy spy(&handle, &LpfHandle::lpfPressed);

    QPoint center(24, 18);
    QTest::mousePress(&handle, Qt::LeftButton, Qt::NoModifier, center);

    QCOMPARE(spy.count(), 1);
    QVERIFY(handle.isDragging());
}

void TestLpfHpfHandle::testHpfHorizontalOnly()
{
    HpfHandle handle;
    QSignalSpy spy(&handle, &HpfHandle::hpfDragged);

    QPoint center(24, 18);
    QTest::mousePress(&handle, Qt::LeftButton, Qt::NoModifier, center);
    QVERIFY(handle.isDragging());

    QPoint startPos = handle.mapToGlobal(QPoint(24, 18));
    QCursor::setPos(startPos);
    QTest::qWait(20);

    QCursor::setPos(startPos + QPoint(30, 0));
    QTest::qWait(20);

    QCursor::setPos(startPos + QPoint(30, 50));
    QTest::qWait(20);

    QTest::mouseRelease(&handle, Qt::LeftButton, Qt::NoModifier, QPoint(24, 18));

    QVERIFY(!handle.isDragging());
    QVERIFY(spy.count() > 0);
}

void TestLpfHpfHandle::testLpfPaint()
{
    LpfHandle handle;
    QSize size = handle.sizeHint();
    QCOMPARE(size.width(), 48);
    QCOMPARE(size.height(), 36);
}

QTEST_MAIN(TestLpfHpfHandle)
#include "TestLpfHpfHandle.moc"
