#include <QCoreApplication>
#include <QDebug>
#include <cstdio>
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanPipeline.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    fprintf(stderr, "=== VulkanPipeline Creation Test ===\n");
    fflush(stderr);

    VulkanContext ctx;
    fprintf(stderr, "Calling ctx.initialize()...\n");
    fflush(stderr);

    if (!ctx.initialize()) {
        fprintf(stderr, "VulkanContext::initialize() FAILED\n");
        return 1;
    }
    fprintf(stderr, "VulkanContext: OK\n");
    fprintf(stderr, "Device: %p\n", (void*)ctx.device());
    fflush(stderr);

    // Match the render pass from VulkanSwapchain::createRenderPass():
    VkAttachmentDescription attachments[2] = {};

    attachments[0].format = VK_FORMAT_B8G8R8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    attachments[1].format = VK_FORMAT_B8G8R8A8_UNORM;
    attachments[1].samples = VK_SAMPLE_COUNT_4_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveRef = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference msaaRef    = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &msaaRef;
    subpass.pResolveAttachments = &resolveRef;

    VkRenderPassCreateInfo rpci = {};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 2;
    rpci.pAttachments = attachments;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    fprintf(stderr, "Calling vkCreateRenderPass...\n");
    fflush(stderr);

    VkResult r = vkCreateRenderPass(ctx.device(), &rpci, nullptr, &renderPass);
    if (r != VK_SUCCESS) {
        fprintf(stderr, "vkCreateRenderPass FAILED: %d\n", r);
        return 1;
    }
    fprintf(stderr, "RenderPass: OK (%p)\n", (void*)renderPass);
    fflush(stderr);

    fprintf(stderr, "Creating VulkanPipeline...\n");
    fflush(stderr);
    VulkanPipeline pipeline(&ctx);

    fprintf(stderr, "Calling pipeline.create(renderPass)...\n");
    fflush(stderr);
    bool ok = pipeline.create(renderPass);
    fprintf(stderr, "VulkanPipeline::create(): %s\n", ok ? "SUCCESS" : "FAILED");
    fflush(stderr);

    fprintf(stderr, "Cleaning up...\n");
    fflush(stderr);
    vkDeviceWaitIdle(ctx.device());
    pipeline.destroy();
    vkDestroyRenderPass(ctx.device(), renderPass, nullptr);
    ctx.destroy();

    fprintf(stderr, "=== Test Complete ===\n");
    fflush(stderr);
    return ok ? 0 : 1;
}
