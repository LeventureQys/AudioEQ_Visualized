#define STB_TRUETYPE_IMPLEMENTATION
#include "../../thirdparty/code/stb/stb_truetype.h"
#include "VulkanFontAtlas.h"
#include "VulkanContext.h"
#include <QDebug>
#include <cstring>
#include <vector>

#if __has_include("EmbeddedFont.h")
    #include "EmbeddedFont.h"
    #define FONT_DATA EmbeddedFont::noto_sans_regular
    #define FONT_SIZE EmbeddedFont::noto_sans_regular_size
#else
    static const unsigned char FONT_DATA[1] = {0};
    static const size_t FONT_SIZE = 0;
#endif

VulkanFontAtlas::VulkanFontAtlas(VulkanContext* ctx) : m_ctx(ctx) {}
VulkanFontAtlas::~VulkanFontAtlas() { destroy(); }

bool VulkanFontAtlas::initialize(float fontSize) {
    if (FONT_SIZE == 0) {
        qWarning("VulkanFontAtlas: No embedded font data available");
        return false;
    }

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, FONT_DATA, 0)) {
        qWarning("VulkanFontAtlas: Failed to initialize font");
        return false;
    }

    float scale = stbtt_ScaleForPixelHeight(&font, fontSize);

    const int ATLAS_WIDTH = 512;
    const int ATLAS_HEIGHT = 512;

    std::vector<unsigned char> atlasData(ATLAS_WIDTH * ATLAS_HEIGHT, 0);

    int penX = 0, penY = 0;
    int maxRowHeight = 0;

    for (int c = 32; c < 127; ++c) {
        int w, h, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, scale, scale, c, &w, &h, &xoff, &yoff);

        if (!bitmap) {
            GlyphInfo info;
            info.uvRect = QRectF(0, 0, 0, 0);
            info.bearingRect = QRectF(0, 0, 0, 0);
            info.advance = 0;
            m_glyphs.insert(QChar(c), info);
            continue;
        }

        if (penX + w + 1 > ATLAS_WIDTH) {
            penX = 0;
            penY += maxRowHeight + 1;
            maxRowHeight = 0;
        }

        if (penY + h + 1 > ATLAS_HEIGHT) {
            stbtt_FreeBitmap(bitmap, nullptr);
            qWarning("VulkanFontAtlas: Atlas full at character %d", c);
            break;
        }

        for (int row = 0; row < h; ++row) {
            memcpy(&atlasData[(penY + row) * ATLAS_WIDTH + penX], &bitmap[row * w], w);
        }

        GlyphInfo info;
        info.uvRect = QRectF(
            (float)penX / ATLAS_WIDTH,
            (float)penY / ATLAS_HEIGHT,
            (float)w / ATLAS_WIDTH,
            (float)h / ATLAS_HEIGHT
        );
        info.bearingRect = QRectF(xoff, yoff, w, h);
        int advance;
        stbtt_GetCodepointHMetrics(&font, c, &advance, nullptr);
        info.advance = advance * scale;
        m_glyphs.insert(QChar(c), info);

        penX += w + 1;
        maxRowHeight = (std::max)(maxRowHeight, h);

        stbtt_FreeBitmap(bitmap, nullptr);
    }

    createAtlasTexture(ATLAS_WIDTH, ATLAS_HEIGHT, atlasData.data());

    qDebug() << "VulkanFontAtlas: Initialized" << m_glyphs.size() << "glyphs at" << fontSize << "px";
    qDebug() << "VulkanFontAtlas: atlasView=" << (m_atlasView ? "yes" : "no") << "sampler=" << (m_sampler ? "yes" : "no");
    qDebug() << "VulkanFontAtlas: FONT_SIZE=" << FONT_SIZE;
    return true;
}

void VulkanFontAtlas::createAtlasTexture(int width, int height, const unsigned char* data) {
    VkDevice dev = m_ctx->device();
    VkDeviceSize imageSize = width * height;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    VkBufferCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = imageSize;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(dev, &bci, nullptr, &stagingBuffer);

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(dev, stagingBuffer, &memReq);
    VkMemoryAllocateInfo mai = {};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memReq.size;
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(m_ctx->physicalDevice(), &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
        if ((memReq.memoryTypeBits & (1<<i)) && (mp.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            mai.memoryTypeIndex = i;
            break;
        }
    }
    vkAllocateMemory(dev, &mai, nullptr, &stagingMemory);
    vkBindBufferMemory(dev, stagingBuffer, stagingMemory, 0);

    void* mapped;
    vkMapMemory(dev, stagingMemory, 0, imageSize, 0, &mapped);
    memcpy(mapped, data, imageSize);
    vkUnmapMemory(dev, stagingMemory);

    VkImageCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = VK_FORMAT_R8_UNORM;
    ici.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(dev, &ici, nullptr, &m_atlasImage);

    vkGetImageMemoryRequirements(dev, m_atlasImage, &memReq);
    mai.allocationSize = memReq.size;
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
        if ((memReq.memoryTypeBits & (1<<i)) && (mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            mai.memoryTypeIndex = i;
            break;
        }
    }
    vkAllocateMemory(dev, &mai, nullptr, &m_atlasMemory);
    vkBindImageMemory(dev, m_atlasImage, m_atlasMemory, 0);

    VkCommandPoolCreateInfo cpi = {};
    cpi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpi.queueFamilyIndex = m_ctx->graphicsFamily();
    VkCommandPool pool;
    vkCreateCommandPool(dev, &cpi, nullptr, &pool);

    VkCommandBufferAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(dev, &ai, &cmd);

    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_atlasImage;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region = {};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_atlasImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);
    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    vkQueueSubmit(m_ctx->graphicsQueue(), 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_ctx->graphicsQueue());

    vkFreeCommandBuffers(dev, pool, 1, &cmd);
    vkDestroyCommandPool(dev, pool, nullptr);
    vkDestroyBuffer(dev, stagingBuffer, nullptr);
    vkFreeMemory(dev, stagingMemory, nullptr);

    VkImageViewCreateInfo iv = {};
    iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv.image = m_atlasImage;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = VK_FORMAT_R8_UNORM;
    iv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(dev, &iv, nullptr, &m_atlasView);

    VkSamplerCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter = VK_FILTER_LINEAR;
    sci.minFilter = VK_FILTER_LINEAR;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(dev, &sci, nullptr, &m_sampler);
}

void VulkanFontAtlas::destroy() {
    VkDevice dev = m_ctx ? m_ctx->device() : VK_NULL_HANDLE;
    if (!dev) return;
    if (m_sampler)     { vkDestroySampler(dev, m_sampler, nullptr);     m_sampler = VK_NULL_HANDLE; }
    if (m_atlasView)   { vkDestroyImageView(dev, m_atlasView, nullptr);   m_atlasView = VK_NULL_HANDLE; }
    if (m_atlasImage)  { vkDestroyImage(dev, m_atlasImage, nullptr);      m_atlasImage = VK_NULL_HANDLE; }
    if (m_atlasMemory) { vkFreeMemory(dev, m_atlasMemory, nullptr);       m_atlasMemory = VK_NULL_HANDLE; }
}

bool VulkanFontAtlas::glyphInfo(QChar ch, GlyphInfo* out) const {
    auto it = m_glyphs.find(ch);
    if (it == m_glyphs.end()) return false;
    *out = it.value();
    return true;
}

VkImageView VulkanFontAtlas::atlasImageView() const { return m_atlasView; }
VkSampler   VulkanFontAtlas::atlasSampler()  const { return m_sampler; }
