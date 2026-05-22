#include <QtTest>

class TestBuildValidation : public QObject {
    Q_OBJECT
private slots:
    void testQtVersion() { QVERIFY(QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)); }
    void testCppVersion() { static_assert(__cplusplus >= 201703L); }
};

QTEST_MAIN(TestBuildValidation)
#include "TestBuildValidation.moc"
