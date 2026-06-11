#pragma once

#include "volk.h"
#include <QRectF>
#include <QMap>
#include <QChar>

class VulkanContext;

struct GlyphInfo {
    QRectF  uvRect;
    QRectF  bearingRect;
    float   advance;
};

class VulkanFontAtlas {
public:
    explicit VulkanFontAtlas(VulkanContext* ctx);
    ~VulkanFontAtlas();

    bool initialize(float fontSize = 14.0f);

    void destroy();

    bool glyphInfo(QChar ch, GlyphInfo* out) const;

    VkImageView  atlasImageView()  const;
    VkSampler    atlasSampler()    const;

private:
    VulkanContext*          m_ctx;
    VkImage                 m_atlasImage  = VK_NULL_HANDLE;
    VkDeviceMemory          m_atlasMemory = VK_NULL_HANDLE;
    VkImageView             m_atlasView   = VK_NULL_HANDLE;
    VkSampler               m_sampler     = VK_NULL_HANDLE;
    QMap<QChar, GlyphInfo>  m_glyphs;

    void createAtlasTexture(int width, int height, const unsigned char* data);
};
