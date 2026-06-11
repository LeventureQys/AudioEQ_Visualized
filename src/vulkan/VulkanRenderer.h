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
    struct UBOData {
        float projMatrix[16];
    };

    QVector<float> buildCurveVBO(const QVector<QPointF>& points, float lineWidthPx);
    QVector<float> buildGridVBO(int* outVertexCount);
    QVector<float> buildGlyphVBO(float x, float y, const QString& text);
    QVector<float> buildCircleVBO(float cx, float cy, float radius, int segments);
    QVector<float> buildEllipseVBO(float cx, float cy, float rx, float ry, int segments);
    QVector<float> buildFillVBO(const QVector<QPointF>& curvePoints);
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    float toNdcX(float pixelX) const;
    float toNdcY(float pixelY) const;
    bool createDescriptorPool();
    bool allocateDescriptorSets();
    void updateUBO(int frameIdx, const QColor& color);
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
    BufferAllocation m_totalCurveVBO;
    QMap<int, BufferAllocation> m_bandCurveVBOs;

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

    QColor m_curveColor      = QColor(0, 255, 0);
    QColor m_backgroundColor = QColor(26, 26, 26);
    QColor m_gridColor       = QColor(255, 255, 255, 38);
    QColor m_gridZeroColor   = QColor(255, 255, 255, 64);

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    QSize        m_size;
    bool         m_initialized = false;
    bool         m_gridDirty   = true;
    bool         m_curveDirty  = true;
};
