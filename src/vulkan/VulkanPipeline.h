#pragma once
#include <volk.h>
#include <QHash>

class VulkanContext;
class VulkanSwapchain;

enum class PipelineType {
    Grid,
    Curve,
    Fill,
    Glyph,
};

class VulkanPipeline {
public:
    VulkanPipeline();
    ~VulkanPipeline();

    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    bool initialize(VulkanContext* ctx, VulkanSwapchain* swapchain);
    void cleanup();

    VkPipeline pipeline(PipelineType type) const;
    VkPipelineLayout pipelineLayout(PipelineType type) const;
    VkDescriptorSetLayout descriptorSetLayout(PipelineType type) const;

private:
    VulkanContext* m_context = nullptr;
    VulkanSwapchain* m_swapchain = nullptr;

    struct PipelineData {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    };

    QHash<PipelineType, PipelineData> m_pipelines;

    VkShaderModule createShaderModule(const uint32_t* code, size_t size);
    PipelineData createGridPipeline();
    PipelineData createCurvePipeline();
    PipelineData createFillPipeline();
    PipelineData createGlyphPipeline();

    VkDescriptorSetLayout createDescriptorSetLayout(
        const QVector<VkDescriptorSetLayoutBinding>& bindings);
    PipelineData createGraphicsPipeline(
        VkShaderModule vert, VkShaderModule frag,
        VkDescriptorSetLayout descriptorSetLayout,
        VkPrimitiveTopology topology,
        VkPolygonMode polygonMode,
        float lineWidth,
        bool enableBlend,
        const QVector<VkVertexInputBindingDescription>& vertexBindings,
        const QVector<VkVertexInputAttributeDescription>& vertexAttrs);
};
