#include "VulkanPipeline.h"
#include "VulkanContext.h"
#include "EmbeddedShaders.h"
#include <QDebug>
#include <vector>

VulkanPipeline::VulkanPipeline(VulkanContext* ctx) : m_ctx(ctx) {}
VulkanPipeline::~VulkanPipeline() { destroy(); }

VkShaderModule VulkanPipeline::loadShaderModule(const uint32_t* spvData, size_t spvSize) {
    VkShaderModuleCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = spvSize * sizeof(uint32_t);
    ci.pCode = spvData;
    VkShaderModule mod = VK_NULL_HANDLE;
    VkResult r = vkCreateShaderModule(m_ctx->device(), &ci, nullptr, &mod);
    if (r != VK_SUCCESS) {
        qWarning() << "VulkanPipeline: vkCreateShaderModule failed:" << r;
        return VK_NULL_HANDLE;
    }
    return mod;
}

VkDescriptorSetLayout VulkanPipeline::createDescriptorSetLayout(PipelineType type) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    if (type == PipelineType::Glyph) {
        VkDescriptorSetLayoutBinding ubo = {};
        ubo.binding = 0;
        ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo.descriptorCount = 1;
        ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(ubo);

        VkDescriptorSetLayoutBinding sampler = {};
        sampler.binding = 1;
        sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler.descriptorCount = 1;
        sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(sampler);
    } else {
        VkDescriptorSetLayoutBinding ubo = {};
        ubo.binding = 0;
        ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo.descriptorCount = 1;
        ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(ubo);
    }

    VkDescriptorSetLayoutCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = static_cast<uint32_t>(bindings.size());
    ci.pBindings = bindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkResult r = vkCreateDescriptorSetLayout(m_ctx->device(), &ci, nullptr, &layout);
    if (r != VK_SUCCESS) {
        qWarning() << "VulkanPipeline: vkCreateDescriptorSetLayout failed:" << r;
        return VK_NULL_HANDLE;
    }
    return layout;
}

bool VulkanPipeline::create(VkRenderPass renderPass) {
    return createPipeline(PipelineType::Grid, renderPass)
        && createPipeline(PipelineType::Curve, renderPass)
        && createPipeline(PipelineType::Fill, renderPass)
        && createPipeline(PipelineType::Glyph, renderPass);
}

bool VulkanPipeline::createPipeline(PipelineType type, VkRenderPass renderPass) {
    auto& ps = m_pipelines[static_cast<int>(type)];

    const uint32_t* vertData = nullptr;
    const uint32_t* fragData = nullptr;
    size_t vertSize = 0, fragSize = 0;

    using namespace EmbeddedShaders;
    switch (type) {
        case PipelineType::Grid:
            vertData = GRID_VERT; vertSize = GRID_VERT_size;
            fragData = GRID_FRAG; fragSize = GRID_FRAG_size;
            break;
        case PipelineType::Curve:
            vertData = CURVE_VERT; vertSize = CURVE_VERT_size;
            fragData = CURVE_FRAG; fragSize = CURVE_FRAG_size;
            break;
        case PipelineType::Fill:
            vertData = FILL_VERT; vertSize = FILL_VERT_size;
            fragData = FILL_FRAG; fragSize = FILL_FRAG_size;
            break;
        case PipelineType::Glyph:
            vertData = GLYPH_VERT; vertSize = GLYPH_VERT_size;
            fragData = GLYPH_FRAG; fragSize = GLYPH_FRAG_size;
            break;
    }

    if (!vertData || !fragData) {
        qWarning() << "VulkanPipeline: Missing SPIR-V data for pipeline" << static_cast<int>(type);
        return false;
    }

    ps.vertModule = loadShaderModule(vertData, vertSize);
    ps.fragModule = loadShaderModule(fragData, fragSize);

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = ps.vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = ps.fragModule;
    stages[1].pName = "main";

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attrs;

    VkVertexInputBindingDescription vbind = {};
    vbind.binding = 0;
    vbind.stride = 0;
    vbind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    if (type == PipelineType::Grid) {
        vbind.stride = 2 * sizeof(float);
        bindings.push_back(vbind);
        attrs.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, 0});
    } else if (type == PipelineType::Curve) {
        vbind.stride = 3 * sizeof(float);
        bindings.push_back(vbind);
        attrs.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, 0});
        attrs.push_back({1, 0, VK_FORMAT_R32_SFLOAT, 2 * sizeof(float)});
    } else if (type == PipelineType::Fill) {
        vbind.stride = 3 * sizeof(float);
        bindings.push_back(vbind);
        attrs.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, 0});
        attrs.push_back({1, 0, VK_FORMAT_R32_SFLOAT, 2 * sizeof(float)});
    } else if (type == PipelineType::Glyph) {
        vbind.stride = 4 * sizeof(float);
        bindings.push_back(vbind);
        attrs.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, 0});
        attrs.push_back({1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float)});
    }

    VkPipelineVertexInputStateCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vi.pVertexBindingDescriptions = bindings.data();
    vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vi.pVertexAttributeDescriptions = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo ia = {};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    if (type == PipelineType::Grid)
        ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    else if (type == PipelineType::Curve)
        ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    else if (type == PipelineType::Glyph)
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    else
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VkViewport vp = {0, 0, (float)1000, (float)800, 0, 1};
    VkRect2D scissor = {{0,0}, {1000,800}};
    VkPipelineViewportStateCreateInfo vps = {};
    vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vps.viewportCount = 1;
    vps.pViewports = &vp;
    vps.scissorCount = 1;
    vps.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rs = {};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms = {};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;

    VkPipelineColorBlendAttachmentState cba = {};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cba.blendEnable = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo cb = {};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    if (type == PipelineType::Curve) {
        dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
    }
    VkPipelineDynamicStateCreateInfo ds = {};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    ds.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    ds.pDynamicStates = dynamicStates.data();

    ps.descriptorSetLayout = createDescriptorSetLayout(type);

    VkPipelineLayoutCreateInfo pl = {};
    pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &ps.descriptorSetLayout;
    VkResult plr = vkCreatePipelineLayout(m_ctx->device(), &pl, nullptr, &ps.layout);
    if (plr != VK_SUCCESS) {
        qWarning() << "VulkanPipeline: vkCreatePipelineLayout failed:" << plr;
        return false;
    }

    VkGraphicsPipelineCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount = 2;
    pci.pStages = stages;
    pci.pVertexInputState = &vi;
    pci.pInputAssemblyState = &ia;
    pci.pViewportState = &vps;
    pci.pRasterizationState = &rs;
    pci.pMultisampleState = &ms;
    pci.pColorBlendState = &cb;
    pci.pDynamicState = &ds;
    pci.layout = ps.layout;
    pci.renderPass = renderPass;
    pci.subpass = 0;

    if (vkCreateGraphicsPipelines(m_ctx->device(), VK_NULL_HANDLE, 1, &pci, nullptr, &ps.pipeline) != VK_SUCCESS) {
        qWarning() << "VulkanPipeline: Failed to create pipeline" << static_cast<int>(type);
        return false;
    }

    qDebug() << "VulkanPipeline: Created pipeline" << static_cast<int>(type);
    return true;
}

void VulkanPipeline::destroy() {
    VkDevice dev = m_ctx ? m_ctx->device() : VK_NULL_HANDLE;
    if (!dev) return;
    for (auto& ps : m_pipelines) {
        if (ps.pipeline) vkDestroyPipeline(dev, ps.pipeline, nullptr);
        if (ps.layout) vkDestroyPipelineLayout(dev, ps.layout, nullptr);
        if (ps.descriptorSetLayout) vkDestroyDescriptorSetLayout(dev, ps.descriptorSetLayout, nullptr);
        if (ps.vertModule) vkDestroyShaderModule(dev, ps.vertModule, nullptr);
        if (ps.fragModule) vkDestroyShaderModule(dev, ps.fragModule, nullptr);
        ps = {};
    }
}

VkPipeline           VulkanPipeline::pipeline(PipelineType t) const { return m_pipelines[static_cast<int>(t)].pipeline; }
VkPipelineLayout     VulkanPipeline::layout(PipelineType t)   const { return m_pipelines[static_cast<int>(t)].layout; }
VkDescriptorSetLayout VulkanPipeline::descriptorSetLayout(PipelineType t) const { return m_pipelines[static_cast<int>(t)].descriptorSetLayout; }
