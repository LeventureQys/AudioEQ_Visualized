#include <QtTest>
#include <QVector>
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanBufferPool.h"
#include "vulkan/VulkanFontAtlas.h"
#include "vulkan/VulkanFrameSync.h"
#include "vulkan/VulkanRenderer.h"

class TestVulkanRenderer : public QObject {
	Q_OBJECT
private slots:
	void testConstruction();
	void testCleanupWithoutInit();
	void testColorSetters();
	void testUpdateVBOsWithoutInit();
	void testInitialize();
};

void TestVulkanRenderer::testConstruction()
{
	VulkanRenderer renderer;
	QVERIFY(true);
}

void TestVulkanRenderer::testCleanupWithoutInit()
{
	VulkanRenderer renderer;
	renderer.cleanup();
	QVERIFY(true);
}

void TestVulkanRenderer::testColorSetters()
{
	VulkanRenderer renderer;
	renderer.setCurveColor(QColor(255, 0, 0));
	renderer.setBackgroundColor(QColor(0, 0, 0));
	QVERIFY(true);
}

void TestVulkanRenderer::testUpdateVBOsWithoutInit()
{
	VulkanRenderer renderer;
	QVector<float> v = {0.0f, 0.5f, 0.0f};
	renderer.updateGridVertices(v);
	renderer.updateCurveVertices(v);
	renderer.updateFillVertices({0.0f, 0.5f});
	renderer.updateGlyphVertices({0.0f, 0.5f, 0.0f, 1.0f});
	QVERIFY(true);
}

void TestVulkanRenderer::testInitialize()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	if (!ctx.initialize()) {
		QSKIP("Failed to initialize VulkanContext");
	}

	VkRenderPass renderPass = VK_NULL_HANDLE;
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkResult result = vkCreateRenderPass(ctx.device(), &renderPassInfo, nullptr, &renderPass);
		if (result != VK_SUCCESS) {
			QSKIP("vkCreateRenderPass failed");
		}
	}

	VulkanSwapchain swapchain;
	swapchain.setRenderPassForTest(renderPass);
	swapchain.setExtentForTest({800, 600});
	swapchain.setMsaaSamplesForTest(VK_SAMPLE_COUNT_1_BIT);

	VulkanPipeline pipeline;
	QVERIFY(pipeline.initialize(&ctx, &swapchain));

	VulkanBufferPool bufferPool;
	QVERIFY(bufferPool.initialize(&ctx));

	VulkanFontAtlas fontAtlas;

	VulkanFrameSync frameSync;
	QVERIFY(frameSync.initialize(&ctx, &swapchain));

	{
		VulkanRenderer renderer;
		QVERIFY(renderer.initialize(&ctx, &swapchain, &pipeline, &bufferPool, &fontAtlas, &frameSync));

		QVector<float> gridVerts = {0.0f, 0.5f, 0.0f, 0.0f, 0.0f};
		renderer.updateGridVertices(gridVerts);

		QVector<float> curveVerts = {0.0f, 0.5f, 0.0f};
		renderer.updateCurveVertices(curveVerts);

		renderer.cleanup();
	}

	frameSync.cleanup();
	pipeline.cleanup();
	bufferPool.cleanup();

	if (renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(ctx.device(), renderPass, nullptr);
	}

	ctx.cleanup();
}

QTEST_MAIN(TestVulkanRenderer)
#include "TestVulkanRenderer.moc"
