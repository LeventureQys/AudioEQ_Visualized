#pragma once

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QMap>
#include "volk.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanFrameSync.h"
#include "VulkanPipeline.h"
#include "VulkanBufferPool.h"
#include "VulkanFontAtlas.h"
#include "../AudioEQTypes.h"

class CoordinateMapper;

#ifndef AUDIOEQ_EXPORT
  #ifdef _WIN32
    #define AUDIOEQ_EXPORT __declspec(dllimport)
  #else
    #define AUDIOEQ_EXPORT
  #endif
#endif

struct BandRenderData {
    double  freqHz  = 1000.0;
    double  gainDb  = 0.0;
    int     index   = 0;
    bool    focused = false;
    bool    bypass  = false;
};

struct LpfRenderData { double freqHz = 20000.0; bool enabled = false; };
struct HpfRenderData { double freqHz = 20.0;    bool enabled = false; };

class AUDIOEQ_EXPORT VulkanRenderer : public QObject {
    Q_OBJECT
public:
    explicit VulkanRenderer(VulkanContext* ctx, QObject* parent = nullptr);
    ~VulkanRenderer() override;

    bool initialize(VkSurfaceKHR surface, QSize size);
    void destroy();
    void resize(QSize newSize);

    void setTotalCurve(const QVector<QPointF>& points);
    void setBandCurve(int index, const QVector<QPointF>& points);
    void clearBandCurve(int index);
    void setBands(const QVector<BandRenderData>& bands);
    void setLpf(const LpfRenderData& lpf);
    void setHpf(const HpfRenderData& hpf);
    void setCoordinateMapper(const CoordinateMapper* mapper);

    void setCurveColor(QColor color);
    void setBackgroundColor(QColor color);

    void renderFrame();

private:
    struct GridUBO {
        float color[4];
        float alpha;
        float _pad0, _pad1, _pad2;
    };
    struct CurveUBO {
        float color[4];
        float lineWidth;
        float _pad0, _pad1, _pad2;
    };
    struct FillUBO {
        float color[4];
    };
    struct GlyphUBO {
        float color[4];
    };

    QVector<float> buildCurveVBO(const QVector<QPointF>& points, float lineWidthPx);
    QVector<float> buildGridVBO(int* outVertexCount);
    QVector<float> buildAxisVBO();
    QVector<float> buildCircleVBO(float cx, float cy, float radius, int segments);
    QVector<float> buildEllipseVBO(float cx, float cy, float rx, float ry, int segments);
    QVector<float> buildFillVBO(const QVector<QPointF>& curvePoints);
    void buildTextGeometry();
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    float toNdcX(float pixelX) const;
    float toNdcY(float pixelY) const;
    void setInnerScissor(VkCommandBuffer cmd) const;
    void setFullScissor(VkCommandBuffer cmd) const;
    bool createDescriptorPool();
    bool allocateDescriptorSets();
    void updateUBO(int frameIdx);
    void ensureTotalCurveVBO(size_t floatCount);
    void ensureBandCurveVBO(int index, size_t floatCount);

    VulkanContext*    m_ctx;
    VulkanSwapchain   m_swapchain;
    VulkanFrameSync   m_frameSync;
    VulkanPipeline    m_pipeline;
    VulkanBufferPool  m_bufferPool;
    VulkanFontAtlas   m_fontAtlas;

    QVector<QPointF>              m_totalCurve;
    QMap<int, QVector<QPointF>>   m_bandCurves;
    QVector<BandRenderData>       m_bands;
    LpfRenderData                 m_lpf;
    HpfRenderData                 m_hpf;
    const CoordinateMapper*       m_mapper = nullptr;

    BufferAllocation m_gridVBO;
    int              m_gridVertexCount = 0;
    BufferAllocation m_axisVBO;
    uint32_t         m_axisVertexCount = 0;
    BufferAllocation m_totalCurveVBO;
    QMap<int, BufferAllocation> m_bandCurveVBOs;
    BufferAllocation m_textVBO;
    uint32_t         m_textVertexCount = 0;

    BufferAllocation m_gridUBO[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT];
    BufferAllocation m_curveUBO[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT];
    BufferAllocation m_fillUBO[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT];
    BufferAllocation m_glyphUBO[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT];

    VkDescriptorPool  m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet   m_glyphDescSet[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT] = {};
    VkDescriptorSet   m_gridDescSet[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT]  = {};
    VkDescriptorSet   m_curveDescSet[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT] = {};
    VkDescriptorSet   m_fillDescSet[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT]  = {};

    VkCommandPool     m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer   m_commandBuffers[VulkanFrameSync::MAX_FRAMES_IN_FLIGHT] = {};

    QColor m_curveColor      = QColor(0xF0, 0xA0, 0x30);  // Orange-yellow (default, overridable via setCurveColor)
    QColor m_backgroundColor = QColor(0x26, 0x2A, 0x33);  // Dark cool-gray (overridable via setBackgroundColor)
    QColor m_gridColor       = QColor(0x3A, 0x3F, 0x4A, 153);  // ≈ α=0.6
    QColor m_gridZeroColor   = QColor(0x5A, 0x60, 0x70, 230);  // ≈ α=0.9
    QColor m_axisColor       = QColor(0x5A, 0x60, 0x70, 255);  // L-shape axis, α=1.0
    QColor m_glyphColor      = QColor(0x90, 0x98, 0xA8, 255);  // Text labels, α=1.0
    QColor m_fillColor       = QColor(0xF0, 0xA0, 0x30, 102);  // Curve fill (reserved), α=0.4

    static constexpr float MARGIN_LEFT   = 44.0f;
    static constexpr float MARGIN_RIGHT  = 12.0f;
    static constexpr float MARGIN_TOP    = 14.0f;
    static constexpr float MARGIN_BOTTOM = 28.0f;

    static constexpr float LABEL_FONT_SIZE_PX         = 12.0f;
    static constexpr float LABEL_GAP_PX               = 4.0f;
    static constexpr float FREQ_LABEL_Y_OFFSET_PX     = 4.0f;
    static constexpr float GAIN_LABEL_BASELINE_GAP_PX = 6.0f;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    QSize        m_size;
    bool         m_initialized = false;
    bool         m_gridDirty   = true;
    bool         m_curveDirty  = true;
    bool         m_textDirty   = true;
};
