#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/EmbeddedShaders.h"

VulkanPipeline::VulkanPipeline() = default;

VulkanPipeline::~VulkanPipeline()
{
	cleanup();
}

bool VulkanPipeline::initialize(VulkanContext* ctx, VulkanSwapchain* swapchain)
{
	if (!ctx || !swapchain) return false;
	if (swapchain->renderPass() == VK_NULL_HANDLE) return false;

	m_context = ctx;
	m_swapchain = swapchain;

	m_pipelines[PipelineType::Grid] = createGridPipeline();
	m_pipelines[PipelineType::Curve] = createCurvePipeline();
	m_pipelines[PipelineType::Fill] = createFillPipeline();
	m_pipelines[PipelineType::Glyph] = createGlyphPipeline();

	for (const auto& pd : m_pipelines) {
		if (pd.pipeline == VK_NULL_HANDLE) return false;
	}

	return true;
}

void VulkanPipeline::cleanup()
{
	if (!m_context) return;
	auto device = m_context->device();

	for (auto& pd : m_pipelines) {
		if (pd.pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(device, pd.pipeline, nullptr);
			pd.pipeline = VK_NULL_HANDLE;
		}
		if (pd.layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device, pd.layout, nullptr);
			pd.layout = VK_NULL_HANDLE;
		}
		if (pd.descriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, pd.descriptorSetLayout, nullptr);
			pd.descriptorSetLayout = VK_NULL_HANDLE;
		}
	}

	m_pipelines.clear();
	m_context = nullptr;
	m_swapchain = nullptr;
}

VkPipeline VulkanPipeline::pipeline(PipelineType type) const
{
	return m_pipelines.value(type).pipeline;
}

VkPipelineLayout VulkanPipeline::pipelineLayout(PipelineType type) const
{
	return m_pipelines.value(type).layout;
}

VkDescriptorSetLayout VulkanPipeline::descriptorSetLayout(PipelineType type) const
{
	return m_pipelines.value(type).descriptorSetLayout;
}

VkShaderModule VulkanPipeline::createShaderModule(const uint32_t* code, size_t size)
{
	if (!code || size == 0) return VK_NULL_HANDLE;

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = code;

	VkShaderModule module;
	if (vkCreateShaderModule(m_context->device(), &createInfo, nullptr, &module) != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}

	return module;
}

VkDescriptorSetLayout VulkanPipeline::createDescriptorSetLayout(
	const QVector<VkDescriptorSetLayoutBinding>& bindings)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout layout;
	if (vkCreateDescriptorSetLayout(m_context->device(), &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}

	return layout;
}

VulkanPipeline::PipelineData VulkanPipeline::createGraphicsPipeline(
	VkShaderModule vert, VkShaderModule frag,
	VkDescriptorSetLayout dsl,
	VkPrimitiveTopology topology,
	VkPolygonMode polygonMode,
	float lineWidth,
	bool enableBlend,
	const QVector<VkVertexInputBindingDescription>& vertexBindings,
	const QVector<VkVertexInputAttributeDescription>& vertexAttrs)
{
	PipelineData result;
	auto device = m_context->device();

	VkPipelineShaderStageCreateInfo vertStage{};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = vert;
	vertStage.pName = "main";

	VkPipelineShaderStageCreateInfo fragStage{};
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = frag;
	fragStage.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
	vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttrs.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttrs.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = topology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain->extent().width);
	viewport.height = static_cast<float>(m_swapchain->extent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = m_swapchain->extent();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = polygonMode;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = lineWidth;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = m_swapchain->msaaSamples();
	multisampling.sampleShadingEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = enableBlend ? VK_TRUE : VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &dsl;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &result.layout) != VK_SUCCESS) {
		return PipelineData{};
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = result.layout;
	pipelineInfo.renderPass = m_swapchain->renderPass();
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &result.pipeline) != VK_SUCCESS) {
		vkDestroyPipelineLayout(device, result.layout, nullptr);
		result.layout = VK_NULL_HANDLE;
		return PipelineData{};
	}

	result.descriptorSetLayout = dsl;
	return result;
}

VulkanPipeline::PipelineData VulkanPipeline::createGridPipeline()
{
	VkShaderModule vert = createShaderModule(grid_vert_spv, grid_vert_spv_size);
	VkShaderModule frag = createShaderModule(grid_frag_spv, grid_frag_spv_size);
	if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE) {
		if (vert != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), vert, nullptr);
		if (frag != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), frag, nullptr);
		return PipelineData{};
	}

	QVector<VkDescriptorSetLayoutBinding> bindings = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
		{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
	};

	VkDescriptorSetLayout dsl = createDescriptorSetLayout(bindings);
	if (dsl == VK_NULL_HANDLE) {
		vkDestroyShaderModule(m_context->device(), vert, nullptr);
		vkDestroyShaderModule(m_context->device(), frag, nullptr);
		return PipelineData{};
	}

	QVector<VkVertexInputBindingDescription> vertexBindings = {
		{0, 5 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},
	};

	QVector<VkVertexInputAttributeDescription> vertexAttrs = {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, 2 * sizeof(float)},
	};

	PipelineData result = createGraphicsPipeline(
		vert, frag, dsl,
		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		VK_POLYGON_MODE_FILL,
		1.0f,
		true,
		vertexBindings, vertexAttrs
	);

	vkDestroyShaderModule(m_context->device(), vert, nullptr);
	vkDestroyShaderModule(m_context->device(), frag, nullptr);

	if (result.pipeline == VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(m_context->device(), dsl, nullptr);
		return PipelineData{};
	}

	return result;
}

VulkanPipeline::PipelineData VulkanPipeline::createCurvePipeline()
{
	VkShaderModule vert = createShaderModule(curve_vert_spv, curve_vert_spv_size);
	VkShaderModule frag = createShaderModule(curve_frag_spv, curve_frag_spv_size);
	if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE) {
		if (vert != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), vert, nullptr);
		if (frag != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), frag, nullptr);
		return PipelineData{};
	}

	QVector<VkDescriptorSetLayoutBinding> bindings = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
		{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
	};

	VkDescriptorSetLayout dsl = createDescriptorSetLayout(bindings);
	if (dsl == VK_NULL_HANDLE) {
		vkDestroyShaderModule(m_context->device(), vert, nullptr);
		vkDestroyShaderModule(m_context->device(), frag, nullptr);
		return PipelineData{};
	}

	QVector<VkVertexInputBindingDescription> vertexBindings = {
		{0, 3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},
	};

	QVector<VkVertexInputAttributeDescription> vertexAttrs = {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
		{1, 0, VK_FORMAT_R32_SFLOAT, 2 * sizeof(float)},
	};

	PipelineData result = createGraphicsPipeline(
		vert, frag, dsl,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_POLYGON_MODE_FILL,
		2.0f,
		true,
		vertexBindings, vertexAttrs
	);

	vkDestroyShaderModule(m_context->device(), vert, nullptr);
	vkDestroyShaderModule(m_context->device(), frag, nullptr);

	if (result.pipeline == VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(m_context->device(), dsl, nullptr);
		return PipelineData{};
	}

	return result;
}

VulkanPipeline::PipelineData VulkanPipeline::createFillPipeline()
{
	VkShaderModule vert = createShaderModule(fill_vert_spv, fill_vert_spv_size);
	VkShaderModule frag = createShaderModule(fill_frag_spv, fill_frag_spv_size);
	if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE) {
		if (vert != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), vert, nullptr);
		if (frag != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), frag, nullptr);
		return PipelineData{};
	}

	QVector<VkDescriptorSetLayoutBinding> bindings = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
		{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
	};

	VkDescriptorSetLayout dsl = createDescriptorSetLayout(bindings);
	if (dsl == VK_NULL_HANDLE) {
		vkDestroyShaderModule(m_context->device(), vert, nullptr);
		vkDestroyShaderModule(m_context->device(), frag, nullptr);
		return PipelineData{};
	}

	QVector<VkVertexInputBindingDescription> vertexBindings = {
		{0, 2 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},
	};

	QVector<VkVertexInputAttributeDescription> vertexAttrs = {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
	};

	PipelineData result = createGraphicsPipeline(
		vert, frag, dsl,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_POLYGON_MODE_FILL,
		1.0f,
		false,
		vertexBindings, vertexAttrs
	);

	vkDestroyShaderModule(m_context->device(), vert, nullptr);
	vkDestroyShaderModule(m_context->device(), frag, nullptr);

	if (result.pipeline == VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(m_context->device(), dsl, nullptr);
		return PipelineData{};
	}

	return result;
}

VulkanPipeline::PipelineData VulkanPipeline::createGlyphPipeline()
{
	VkShaderModule vert = createShaderModule(glyph_vert_spv, glyph_vert_spv_size);
	VkShaderModule frag = createShaderModule(glyph_frag_spv, glyph_frag_spv_size);
	if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE) {
		if (vert != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), vert, nullptr);
		if (frag != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), frag, nullptr);
		return PipelineData{};
	}

	QVector<VkDescriptorSetLayoutBinding> bindings = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
		{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
	};

	VkDescriptorSetLayout dsl = createDescriptorSetLayout(bindings);
	if (dsl == VK_NULL_HANDLE) {
		vkDestroyShaderModule(m_context->device(), vert, nullptr);
		vkDestroyShaderModule(m_context->device(), frag, nullptr);
		return PipelineData{};
	}

	QVector<VkVertexInputBindingDescription> vertexBindings = {
		{0, 4 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},
	};

	QVector<VkVertexInputAttributeDescription> vertexAttrs = {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
		{1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float)},
	};

	PipelineData result = createGraphicsPipeline(
		vert, frag, dsl,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_POLYGON_MODE_FILL,
		1.0f,
		true,
		vertexBindings, vertexAttrs
	);

	vkDestroyShaderModule(m_context->device(), vert, nullptr);
	vkDestroyShaderModule(m_context->device(), frag, nullptr);

	if (result.pipeline == VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(m_context->device(), dsl, nullptr);
		return PipelineData{};
	}

	return result;
}
