#pragma once
#include <volk.h>
#include <QHash>
#include <QRectF>
#include <QImage>

class VulkanContext;

struct GlyphMetrics {
	QRectF texCoords;
	QRectF quadRect;
	float advanceX = 0;
};

class VulkanFontAtlas {
public:
	VulkanFontAtlas();
	~VulkanFontAtlas();

	VulkanFontAtlas(const VulkanFontAtlas&) = delete;
	VulkanFontAtlas& operator=(const VulkanFontAtlas&) = delete;

	bool initialize(VulkanContext* ctx, const QByteArray& fontData, float fontSize, float devicePixelRatio = 1.0f);
	void cleanup();

	VkImage image() const;
	VkImageView imageView() const;
	VkSampler sampler() const;
	VkExtent2D atlasSize() const;

	const GlyphMetrics& glyphMetrics(QChar ch) const;
	bool hasGlyph(QChar ch) const;

private:
	VulkanContext* m_context = nullptr;

	VkImage m_atlasImage = VK_NULL_HANDLE;
	VkDeviceMemory m_atlasMemory = VK_NULL_HANDLE;
	VkImageView m_atlasImageView = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;
	VkExtent2D m_atlasSize{};

	QHash<QChar, GlyphMetrics> m_glyphs;

	struct {
		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	} m_staging;

	bool createAtlasImage();
	bool rasterizeFont(const QByteArray& fontData, float fontSize, float dpr);
	bool uploadAtlas(const QImage& atlas);
	bool createSampler();
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
