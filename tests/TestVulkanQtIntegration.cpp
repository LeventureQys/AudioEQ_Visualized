#include <QtTest>
#include "vulkan/VulkanQtIntegration.h"
#include "vulkan/VulkanContext.h"

class TestVulkanQtIntegration : public QObject {
	Q_OBJECT
private slots:
	void testInitialize();
	void testCleanup();
};

void TestVulkanQtIntegration::testInitialize()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan not available");
	}
	VulkanQtIntegration integration;
	QWidget parent;
	bool ok = integration.initialize(&parent);
	QVERIFY(ok);
	QVERIFY(integration.context() != nullptr);
	QVERIFY(integration.renderer() != nullptr);
}

void TestVulkanQtIntegration::testCleanup()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan not available");
	}
	VulkanQtIntegration integration;
	QWidget parent;
	integration.initialize(&parent);
	integration.cleanup();
	QVERIFY(integration.context() == nullptr);
	QVERIFY(integration.renderer() == nullptr);
	integration.initialize(&parent);
	integration.cleanup();
}

QTEST_MAIN(TestVulkanQtIntegration)
#include "TestVulkanQtIntegration.moc"
