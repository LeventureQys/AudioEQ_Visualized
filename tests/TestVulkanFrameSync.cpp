#include <QtTest>
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanFrameSync.h"

class TestVulkanFrameSync : public QObject {
	Q_OBJECT
private slots:
	void testConstant();
	void testConstruction();
	void testCleanupWithoutInit();
	void testInitializeAndCleanup();
};

void TestVulkanFrameSync::testConstant()
{
	QCOMPARE(VulkanFrameSync::MAX_FRAMES_IN_FLIGHT, 2);
}

void TestVulkanFrameSync::testConstruction()
{
	{
		VulkanFrameSync fs;
		Q_UNUSED(fs);
	}
	QVERIFY(true);
}

void TestVulkanFrameSync::testCleanupWithoutInit()
{
	VulkanFrameSync fs;
	fs.cleanup();
	QVERIFY(true);
}

void TestVulkanFrameSync::testInitializeAndCleanup()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	if (!ctx.initialize()) {
		QSKIP("Failed to initialize VulkanContext");
	}

	VulkanSwapchain swapchain;
	swapchain.setExtentForTest({800, 600});
	swapchain.setMsaaSamplesForTest(VK_SAMPLE_COUNT_1_BIT);
	swapchain.setRenderPassForTest(VK_NULL_HANDLE);

	VulkanFrameSync frameSync;
	QVERIFY(frameSync.initialize(&ctx, &swapchain));
	frameSync.cleanup();

	ctx.cleanup();
}

QTEST_MAIN(TestVulkanFrameSync)
#include "TestVulkanFrameSync.moc"
