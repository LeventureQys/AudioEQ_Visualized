#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "vulkan/VulkanFontAtlas.h"
#include "vulkan/VulkanContext.h"
#include <vector>
#include <cstring>

VulkanFontAtlas::VulkanFontAtlas()
{
}

VulkanFontAtlas::~VulkanFontAtlas()
{
	cleanup();
}

bool VulkanFontAtlas::initialize(VulkanContext* ctx, const QByteArray& fontData, float fontSize, float devicePixelRatio)
{
	if (!ctx || ctx->device() == VK_NULL_HANDLE) {
		return false;
	}
	m_context = ctx;

	if (!rasterizeFont(fontData, fontSize, devicePixelRatio)) {
		return false;
	}

	if (!createAtlasImage()) {
		return false;
	}

	QImage atlasImage(static_cast<int>(m_atlasSize.width), static_cast<int>(m_atlasSize.height), QImage::Format_RGBA8888);
	atlasImage.fill(Qt::transparent);

	const int cellW = static_cast<int>(m_atlasSize.width) / 16;
	const int cellH = static_cast<int>(m_atlasSize.height) / 6;
	const float pixelHeight = fontSize * devicePixelRatio;

	const unsigned char* rawData = reinterpret_cast<const unsigned char*>(fontData.constData());
	stbtt_fontinfo fontInfo;
	if (!stbtt_InitFont(&fontInfo, rawData, 0)) {
		return false;
	}

	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

	float realScale = pixelHeight / static_cast<float>(ascent - descent);

	int col = 0;
	int row = 0;

	for (int chCode = 32; chCode <= 126; ++chCode) {
		if (col >= 16) {
			col = 0;
			++row;
		}
		if (row >= 6) {
			break;
		}

		QChar ch(static_cast<char16_t>(chCode));
		int glyphIdx = stbtt_FindGlyphIndex(&fontInfo, chCode);
		if (glyphIdx == 0) {
			GlyphMetrics m;
			m.texCoords = QRectF(0, 0, 0, 0);
			m.quadRect = QRectF(0, 0, 0, 0);
			m.advanceX = 0;
			m_glyphs.insert(ch, m);
			++col;
			continue;
		}

		int ix0, iy0, ix1, iy1;
		stbtt_GetCodepointBitmapBox(&fontInfo, chCode, realScale, realScale, &ix0, &iy0, &ix1, &iy1);

		int glyphW = ix1 - ix0;
		int glyphH = iy1 - iy0;

		int penX = col * cellW;
		int penY = row * cellH;
		int xOff = ix0;
		int yOff = iy0;

		std::vector<unsigned char> glyphBitmap(glyphW * glyphH, 0);
		if (glyphW > 0 && glyphH > 0) {
			stbtt_MakeCodepointBitmap(&fontInfo, glyphBitmap.data(), glyphW, glyphH, glyphW, realScale, realScale, chCode);

			int drawX = penX;
			int drawY = penY;
			if (xOff < 0) {
				drawX = penX - xOff;
			}
			if (yOff < 0) {
				drawY = penY - yOff;
			}

			for (int gy = 0; gy < glyphH; ++gy) {
				for (int gx = 0; gx < glyphW; ++gx) {
					int px = drawX + gx;
					int py = drawY + gy;
					if (px < 0 || px >= static_cast<int>(m_atlasSize.width) ||
						py < 0 || py >= static_cast<int>(m_atlasSize.height)) {
						continue;
					}
					unsigned char val = glyphBitmap[gy * glyphW + gx];
					atlasImage.setPixelColor(px, py, QColor(val, val, val, val));
				}
			}
		}

		int advance, lsb;
		stbtt_GetCodepointHMetrics(&fontInfo, chCode, &advance, &lsb);
		float scaledAdvance = advance * realScale;
		float scaledLSB = lsb * realScale;

		GlyphMetrics m;
		m.texCoords = QRectF(
			static_cast<float>(penX) / m_atlasSize.width,
			static_cast<float>(penY) / m_atlasSize.height,
			static_cast<float>(cellW) / m_atlasSize.width,
			static_cast<float>(cellH) / m_atlasSize.height
		);
		m.quadRect = QRectF(
			xOff,
			yOff,
			static_cast<float>(glyphW),
			static_cast<float>(glyphH)
		);
		m.advanceX = scaledAdvance;
		m_glyphs.insert(ch, m);

		++col;
	}

	if (!uploadAtlas(atlasImage)) {
		return false;
	}

	if (!createSampler()) {
		return false;
	}

	return true;
}

void VulkanFontAtlas::cleanup()
{
	VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;

	if (device != VK_NULL_HANDLE) {
		if (m_sampler != VK_NULL_HANDLE) {
			vkDestroySampler(device, m_sampler, nullptr);
			m_sampler = VK_NULL_HANDLE;
		}
		if (m_atlasImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, m_atlasImageView, nullptr);
			m_atlasImageView = VK_NULL_HANDLE;
		}
		if (m_atlasImage != VK_NULL_HANDLE) {
			vkDestroyImage(device, m_atlasImage, nullptr);
			m_atlasImage = VK_NULL_HANDLE;
		}
		if (m_atlasMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, m_atlasMemory, nullptr);
			m_atlasMemory = VK_NULL_HANDLE;
		}
		if (m_staging.stagingBuffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, m_staging.stagingBuffer, nullptr);
			m_staging.stagingBuffer = VK_NULL_HANDLE;
		}
		if (m_staging.stagingMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, m_staging.stagingMemory, nullptr);
			m_staging.stagingMemory = VK_NULL_HANDLE;
		}
	}

	m_context = nullptr;
	m_glyphs.clear();
	m_atlasSize = {};
}

VkImage VulkanFontAtlas::image() const
{
	return m_atlasImage;
}

VkImageView VulkanFontAtlas::imageView() const
{
	return m_atlasImageView;
}

VkSampler VulkanFontAtlas::sampler() const
{
	return m_sampler;
}

VkExtent2D VulkanFontAtlas::atlasSize() const
{
	return m_atlasSize;
}

const GlyphMetrics& VulkanFontAtlas::glyphMetrics(QChar ch) const
{
	static GlyphMetrics empty;
	auto it = m_glyphs.constFind(ch);
	if (it != m_glyphs.constEnd()) {
		return *it;
	}
	return empty;
}

bool VulkanFontAtlas::hasGlyph(QChar ch) const
{
	return m_glyphs.contains(ch);
}

bool VulkanFontAtlas::rasterizeFont(const QByteArray& fontData, float fontSize, float dpr)
{
	if (fontData.isEmpty()) {
		return false;
	}

	float pixelHeight = fontSize * dpr;

	const unsigned char* rawData = reinterpret_cast<const unsigned char*>(fontData.constData());
	stbtt_fontinfo fontInfo;
	if (!stbtt_InitFont(&fontInfo, rawData, 0)) {
		return false;
	}

	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

	float scale = pixelHeight / static_cast<float>(ascent - descent);

	int maxGlyphW = 0;
	int maxGlyphH = 0;
	for (int chCode = 32; chCode <= 126; ++chCode) {
		int ix0, iy0, ix1, iy1;
		stbtt_GetCodepointBitmapBox(&fontInfo, chCode, scale, scale, &ix0, &iy0, &ix1, &iy1);
		int w = ix1 - ix0;
		int h = iy1 - iy0;
		if (w > maxGlyphW) maxGlyphW = w;
		if (h > maxGlyphH) maxGlyphH = h;
	}

	int cellW = maxGlyphW + 2;
	int cellH = maxGlyphH + 2;
	if (cellW < 1) cellW = 1;
	if (cellH < 1) cellH = 1;

	const int cols = 16;
	const int rows = 6;
	m_atlasSize.width = static_cast<uint32_t>(cellW * cols);
	m_atlasSize.height = static_cast<uint32_t>(cellH * rows);

	return true;
}

bool VulkanFontAtlas::createAtlasImage()
{
	VkDevice device = m_context->device();

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.extent.width = m_atlasSize.width;
	imageInfo.extent.height = m_atlasSize.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(device, &imageInfo, nullptr, &m_atlasImage) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, m_atlasImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &m_atlasMemory) != VK_SUCCESS) {
		vkDestroyImage(device, m_atlasImage, nullptr);
		m_atlasImage = VK_NULL_HANDLE;
		return false;
	}

	vkBindImageMemory(device, m_atlasImage, m_atlasMemory, 0);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_atlasImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, nullptr, &m_atlasImageView) != VK_SUCCESS) {
		vkDestroyImage(device, m_atlasImage, nullptr);
		m_atlasImage = VK_NULL_HANDLE;
		vkFreeMemory(device, m_atlasMemory, nullptr);
		m_atlasMemory = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

bool VulkanFontAtlas::uploadAtlas(const QImage& atlas)
{
	VkDevice device = m_context->device();
	VkDeviceSize imageSize = static_cast<VkDeviceSize>(atlas.width()) * atlas.height() * 4;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = imageSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_staging.stagingBuffer) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, m_staging.stagingBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &m_staging.stagingMemory) != VK_SUCCESS) {
		vkDestroyBuffer(device, m_staging.stagingBuffer, nullptr);
		m_staging.stagingBuffer = VK_NULL_HANDLE;
		return false;
	}

	vkBindBufferMemory(device, m_staging.stagingBuffer, m_staging.stagingMemory, 0);

	void* mapped = nullptr;
	vkMapMemory(device, m_staging.stagingMemory, 0, imageSize, 0, &mapped);
	if (mapped) {
		std::memcpy(mapped, atlas.constBits(), static_cast<size_t>(imageSize));
		vkUnmapMemory(device, m_staging.stagingMemory);
	}

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_context->graphicsQueueFamily();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	VkCommandPool commandPool;
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		return false;
	}

	VkCommandBufferAllocateInfo cmdAllocInfo{};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = commandPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_atlasImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {m_atlasSize.width, m_atlasSize.height, 1};

	vkCmdCopyBufferToImage(commandBuffer, m_staging.stagingBuffer, m_atlasImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence fence;
	vkCreateFence(device, &fenceInfo, nullptr, &fence);

	vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

	vkDestroyFence(device, fence, nullptr);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	vkDestroyCommandPool(device, commandPool, nullptr);

	if (m_staging.stagingBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, m_staging.stagingBuffer, nullptr);
		m_staging.stagingBuffer = VK_NULL_HANDLE;
	}
	if (m_staging.stagingMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, m_staging.stagingMemory, nullptr);
		m_staging.stagingMemory = VK_NULL_HANDLE;
	}

	return true;
}

bool VulkanFontAtlas::createSampler()
{
	VkDevice device = m_context->device();

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
		return false;
	}

	return true;
}

uint32_t VulkanFontAtlas::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_context->physicalDevice(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if ((typeFilter & (1u << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	return 0;
}
