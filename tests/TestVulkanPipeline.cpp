#include <QtTest>
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanPipeline.h"

class TestVulkanPipeline : public QObject {
	Q_OBJECT

private slots:
	void initTestCase();
	void cleanupTestCase();
	void testCreateAllPipelines();
	void testPipelineLayout();

private:
	VulkanContext ctx;
	VulkanSwapchain swapchain;
	VulkanPipeline pipeline;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	bool m_setupOk = false;
};

void TestVulkanPipeline::initTestCase()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	bool ok = ctx.initialize();
	if (!ok) {
		QSKIP("VulkanContext failed to initialize");
	}

	QVERIFY(ctx.device() != VK_NULL_HANDLE);

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

	VkResult result = vkCreateRenderPass(ctx.device(), &renderPassInfo, nullptr, &m_renderPass);
	if (result != VK_SUCCESS) {
		QSKIP(qPrintable(QString("vkCreateRenderPass failed: %1").arg(static_cast<int>(result))));
	}

	swapchain.setRenderPassForTest(m_renderPass);
	swapchain.setExtentForTest({800, 600});
	swapchain.setMsaaSamplesForTest(VK_SAMPLE_COUNT_1_BIT);

	m_setupOk = true;
}

void TestVulkanPipeline::cleanupTestCase()
{
	pipeline.cleanup();
	if (m_renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(ctx.device(), m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	}
	ctx.cleanup();
}

void TestVulkanPipeline::testCreateAllPipelines()
{
	if (!m_setupOk) {
		QSKIP("Setup failed");
	}

	pipeline.cleanup();
	bool ok = pipeline.initialize(&ctx, &swapchain);
	QVERIFY(ok);

	QVERIFY(pipeline.pipeline(PipelineType::Grid) != VK_NULL_HANDLE);
	QVERIFY(pipeline.pipeline(PipelineType::Curve) != VK_NULL_HANDLE);
	QVERIFY(pipeline.pipeline(PipelineType::Fill) != VK_NULL_HANDLE);
	QVERIFY(pipeline.pipeline(PipelineType::Glyph) != VK_NULL_HANDLE);
}

void TestVulkanPipeline::testPipelineLayout()
{
	if (!m_setupOk) {
		QSKIP("Setup failed");
	}

	pipeline.cleanup();
	bool ok = pipeline.initialize(&ctx, &swapchain);
	QVERIFY(ok);

	QVERIFY(pipeline.pipelineLayout(PipelineType::Grid) != VK_NULL_HANDLE);
	QVERIFY(pipeline.pipelineLayout(PipelineType::Curve) != VK_NULL_HANDLE);
	QVERIFY(pipeline.pipelineLayout(PipelineType::Fill) != VK_NULL_HANDLE);
	QVERIFY(pipeline.pipelineLayout(PipelineType::Glyph) != VK_NULL_HANDLE);

	QVERIFY(pipeline.descriptorSetLayout(PipelineType::Grid) != VK_NULL_HANDLE);
	QVERIFY(pipeline.descriptorSetLayout(PipelineType::Curve) != VK_NULL_HANDLE);
	QVERIFY(pipeline.descriptorSetLayout(PipelineType::Fill) != VK_NULL_HANDLE);
	QVERIFY(pipeline.descriptorSetLayout(PipelineType::Glyph) != VK_NULL_HANDLE);
}

QTEST_MAIN(TestVulkanPipeline)
#include "TestVulkanPipeline.moc"
