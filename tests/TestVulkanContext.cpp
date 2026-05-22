#include <QtTest>
#include "vulkan/VulkanContext.h"

class TestVulkanContext : public QObject {
	Q_OBJECT
private slots:
	void testIsVulkanSupported();
	void testInitializeAndCleanup();
	void testNoDeviceLeak();
	void testMSAASamples();
};

void TestVulkanContext::testIsVulkanSupported()
{
	bool supported = VulkanContext::isVulkanSupported();
	if (!supported) {
		QSKIP("Vulkan is not available on this system");
	}
	QVERIFY(supported);
}

void TestVulkanContext::testInitializeAndCleanup()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	bool ok = ctx.initialize();
	QVERIFY(ok);
	QVERIFY(ctx.instance() != VK_NULL_HANDLE);
	QVERIFY(ctx.physicalDevice() != VK_NULL_HANDLE);
	QVERIFY(ctx.device() != VK_NULL_HANDLE);
	QVERIFY(ctx.graphicsQueue() != VK_NULL_HANDLE);

	ctx.cleanup();
	QCOMPARE(ctx.instance(), VK_NULL_HANDLE);
	QCOMPARE(ctx.device(), VK_NULL_HANDLE);
}

void TestVulkanContext::testNoDeviceLeak()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	for (int i = 0; i < 3; ++i) {
		VulkanContext ctx;
		bool ok = ctx.initialize();
		QVERIFY(ok);
		ctx.cleanup();
	}
}

void TestVulkanContext::testMSAASamples()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	bool ok = ctx.initialize();
	QVERIFY(ok);

	VkSampleCountFlagBits samples = ctx.msaaSamples();
	QVERIFY(samples == VK_SAMPLE_COUNT_1_BIT ||
		samples == VK_SAMPLE_COUNT_2_BIT ||
		samples == VK_SAMPLE_COUNT_4_BIT ||
		samples == VK_SAMPLE_COUNT_8_BIT ||
		samples == VK_SAMPLE_COUNT_16_BIT ||
		samples == VK_SAMPLE_COUNT_32_BIT ||
		samples == VK_SAMPLE_COUNT_64_BIT);

	ctx.cleanup();
}

QTEST_MAIN(TestVulkanContext)
#include "TestVulkanContext.moc"
