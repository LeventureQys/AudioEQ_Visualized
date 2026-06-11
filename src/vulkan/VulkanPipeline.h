#pragma once

#include "volk.h"
#include <array>

class VulkanContext;

enum class PipelineType { Grid = 0, Curve, Fill, Glyph };

class VulkanPipeline {
public:
    explicit VulkanPipeline(VulkanContext* ctx);
    ~VulkanPipeline();

    bool create(VkRenderPass renderPass);
    void destroy();

    VkPipeline           pipeline(PipelineType type) const;
    VkPipelineLayout     layout(PipelineType type)   const;
    VkDescriptorSetLayout descriptorSetLayout(PipelineType type) const;

private:
    struct PipelineSet {
        VkPipeline            pipeline            = VK_NULL_HANDLE;
        VkPipelineLayout      layout              = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkShaderModule        vertModule          = VK_NULL_HANDLE;
        VkShaderModule        fragModule          = VK_NULL_HANDLE;
    };

    VulkanContext*              m_ctx;
    std::array<PipelineSet, 4>  m_pipelines;

    bool createPipeline(PipelineType type, VkRenderPass renderPass);
    VkShaderModule loadShaderModule(const uint32_t* spvData, size_t spvSize);
    VkDescriptorSetLayout createDescriptorSetLayout(PipelineType type);
};
