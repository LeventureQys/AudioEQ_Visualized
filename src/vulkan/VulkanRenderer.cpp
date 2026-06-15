#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "../CoordinateMapper.h"
#include <QDebug>
#include <cmath>
#include <cstring>

// ── Stage11: Shared frequency tick table (used by buildGridVBO and buildTextGeometry) ──
namespace {
struct FreqTick { double hz; const char* label; };
constexpr FreqTick FREQ_TICKS[] = {
    {    20.0, "20Hz"    },
    {    50.0, "50Hz"    },
    {   100.0, "100Hz"   },
    {   200.0, "200Hz"   },
    {   500.0, "500Hz"   },
    {  1000.0, "1KHz"    },
    {  2000.0, "2KHz"    },
    {  5000.0, "5KHz"    },
    { 10000.0, "10KHz"   },
    { 20000.0, "20KHz"   },
};
} // anonymous namespace

VulkanRenderer::VulkanRenderer(VulkanContext* ctx, QObject* parent)
    : QObject(parent),
      m_ctx(ctx),
      m_swapchain(ctx),
      m_frameSync(ctx),
      m_pipeline(ctx),
      m_bufferPool(ctx),
      m_fontAtlas(ctx)
{
}

VulkanRenderer::~VulkanRenderer()
{
    if (m_initialized) destroy();
}

bool VulkanRenderer::initialize(VkSurfaceKHR surface, QSize size)
{
    m_surface = surface;
    m_size = size;

    if (!m_swapchain.create(surface, size)) {
        qWarning("VulkanRenderer: Failed to create swapchain");
        return false;
    }
    if (!m_frameSync.create()) {
        qWarning("VulkanRenderer: Failed to create frame sync");
        return false;
    }
    if (!m_pipeline.create(m_swapchain.renderPass())) {
        qWarning("VulkanRenderer: Failed to create pipelines");
        return false;
    }
    bool fontOk = m_fontAtlas.initialize(20.0f);
    qDebug() << "Font atlas:" << (fontOk ? "OK" : "FAIL") << "glyphs loaded, imageView=" << (m_fontAtlas.atlasImageView() ? "yes" : "no") << "sampler=" << (m_fontAtlas.atlasSampler() ? "yes" : "no");
    if (!fontOk) {
        qWarning("VulkanRenderer: Font atlas init failed (non-fatal)");
    }

    for (int i = 0; i < VulkanFrameSync::MAX_FRAMES_IN_FLIGHT; ++i) {
        m_gridUBO[i]  = m_bufferPool.createUniformBuffer(64);
        m_curveUBO[i] = m_bufferPool.createUniformBuffer(64);
        m_fillUBO[i]  = m_bufferPool.createUniformBuffer(64);
        m_glyphUBO[i] = m_bufferPool.createUniformBuffer(64);
    }

    if (!createDescriptorPool()) {
        qWarning("VulkanRenderer: Failed to create descriptor pool");
        return false;
    }
    if (!allocateDescriptorSets()) {
        qWarning("VulkanRenderer: Failed to allocate descriptor sets");
        return false;
    }

    VkCommandPoolCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = m_ctx->graphicsFamily();
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_ctx->device(), &cpci, nullptr, &m_commandPool);

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = m_commandPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = VulkanFrameSync::MAX_FRAMES_IN_FLIGHT;
    vkAllocateCommandBuffers(m_ctx->device(), &cbai, m_commandBuffers);

    auto gridVerts = buildGridVBO(&m_gridVertexCount);
    m_gridVBO = m_bufferPool.createVertexBuffer(gridVerts.size() * sizeof(float));
    m_bufferPool.uploadData(m_gridVBO, gridVerts.data(), gridVerts.size() * sizeof(float));

    auto axisVerts = buildAxisVBO();
    m_axisVertexCount = static_cast<uint32_t>(axisVerts.size() / 2);
    m_axisVBO = m_bufferPool.createVertexBuffer(axisVerts.size() * sizeof(float));
    m_bufferPool.uploadData(m_axisVBO, axisVerts.data(), axisVerts.size() * sizeof(float));

    buildTextGeometry();
    m_textDirty = false;
    qDebug() << "Text VBO built:" << m_textVertexCount << "vertices";

    m_initialized = true;
    qDebug() << "VulkanRenderer: Initialized" << size.width() << "x" << size.height();
    return true;
}

void VulkanRenderer::destroy()
{
    if (!m_initialized)
        return;

    VkDevice dev = m_ctx->device();
    vkDeviceWaitIdle(dev);

    if (m_commandPool) {
        vkDestroyCommandPool(dev, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    if (m_descriptorPool) {
        vkDestroyDescriptorPool(dev, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    for (int i = 0; i < VulkanFrameSync::MAX_FRAMES_IN_FLIGHT; ++i) {
        m_bufferPool.free(m_gridUBO[i]);
        m_bufferPool.free(m_curveUBO[i]);
        m_bufferPool.free(m_fillUBO[i]);
        m_bufferPool.free(m_glyphUBO[i]);
    }

    m_bufferPool.free(m_gridVBO);
    m_bufferPool.free(m_axisVBO);
    m_bufferPool.free(m_totalCurveVBO);
    m_bufferPool.free(m_textVBO);
    for (auto& kv : m_bandCurveVBOs)
        m_bufferPool.free(kv);
    m_bandCurveVBOs.clear();

    m_fontAtlas.destroy();
    m_pipeline.destroy();
    m_frameSync.destroy();
    m_swapchain.destroy();

    m_initialized = false;
}

void VulkanRenderer::resize(QSize newSize)
{
    m_size = newSize;
    vkDeviceWaitIdle(m_ctx->device());
    if (!m_swapchain.resize(newSize)) {
        m_initialized = false;
        return;
    }
    m_gridDirty = true;
    m_textDirty = true;
    qDebug() << "VulkanRenderer::resize:" << newSize << "rebuilding text...";
    buildTextGeometry();
}

float VulkanRenderer::toNdcX(float pixelX) const
{
    return (pixelX / static_cast<float>(m_size.width())) * 2.0f - 1.0f;
}

float VulkanRenderer::toNdcY(float pixelY) const
{
    return (pixelY / static_cast<float>(m_size.height())) * 2.0f - 1.0f;
}

void VulkanRenderer::setTotalCurve(const QVector<QPointF>& points)  { m_totalCurve = points; m_curveDirty = true; }
void VulkanRenderer::setBandCurve(int index, const QVector<QPointF>& points) { m_bandCurves[index] = points; m_curveDirty = true; }
void VulkanRenderer::clearBandCurve(int index) { m_bandCurves.remove(index); }
void VulkanRenderer::setBands(const QVector<BandRenderData>& bands) { m_bands = bands; }
void VulkanRenderer::setLpf(const LpfRenderData& lpf) { m_lpf = lpf; }
void VulkanRenderer::setHpf(const HpfRenderData& hpf) { m_hpf = hpf; }
void VulkanRenderer::setCoordinateMapper(const CoordinateMapper* mapper) { m_mapper = mapper; m_textDirty = true; m_gridDirty = true; }
void VulkanRenderer::setCurveColor(QColor c) { m_curveColor = c; }
void VulkanRenderer::setBackgroundColor(QColor c) { m_backgroundColor = c; }

void VulkanRenderer::setInnerScissor(VkCommandBuffer cmd) const {
    VkRect2D s;
    s.offset.x = static_cast<int32_t>(MARGIN_LEFT);
    s.offset.y = static_cast<int32_t>(MARGIN_TOP);
    s.extent.width  = static_cast<uint32_t>((std::max)(0, static_cast<int>(m_size.width())  - static_cast<int>(MARGIN_LEFT) - static_cast<int>(MARGIN_RIGHT)));
    s.extent.height = static_cast<uint32_t>((std::max)(0, static_cast<int>(m_size.height()) - static_cast<int>(MARGIN_TOP)  - static_cast<int>(MARGIN_BOTTOM)));
    vkCmdSetScissor(cmd, 0, 1, &s);
}

void VulkanRenderer::setFullScissor(VkCommandBuffer cmd) const {
    VkRect2D s = { {0, 0}, { static_cast<uint32_t>(m_size.width()), static_cast<uint32_t>(m_size.height()) } };
    vkCmdSetScissor(cmd, 0, 1, &s);
}

bool VulkanRenderer::createDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(VulkanFrameSync::MAX_FRAMES_IN_FLIGHT * 4);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(VulkanFrameSync::MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo dpci = {};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.poolSizeCount = 2;
    dpci.pPoolSizes = poolSizes;
    dpci.maxSets = static_cast<uint32_t>(VulkanFrameSync::MAX_FRAMES_IN_FLIGHT * 4);

    return vkCreateDescriptorPool(m_ctx->device(), &dpci, nullptr, &m_descriptorPool) == VK_SUCCESS;
}

bool VulkanRenderer::allocateDescriptorSets()
{
    VkDevice dev = m_ctx->device();
    VkDescriptorSetLayout layouts[4];

    for (int i = 0; i < VulkanFrameSync::MAX_FRAMES_IN_FLIGHT; ++i) {
        // Grid
        VkDescriptorSetLayout gridLayout = m_pipeline.descriptorSetLayout(PipelineType::Grid);
        VkDescriptorSetAllocateInfo ai = {};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = m_descriptorPool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &gridLayout;
        if (vkAllocateDescriptorSets(dev, &ai, &m_gridDescSet[i]) != VK_SUCCESS)
            return false;

        VkDescriptorBufferInfo gridUBI = {m_gridUBO[i].buffer, 0, 64};
        VkWriteDescriptorSet w = {};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = m_gridDescSet[i];
        w.dstBinding = 0;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.pBufferInfo = &gridUBI;
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);

        // Curve
        VkDescriptorSetLayout curveLayout = m_pipeline.descriptorSetLayout(PipelineType::Curve);
        ai.pSetLayouts = &curveLayout;
        if (vkAllocateDescriptorSets(dev, &ai, &m_curveDescSet[i]) != VK_SUCCESS)
            return false;

        VkDescriptorBufferInfo curveUBI = {m_curveUBO[i].buffer, 0, 64};
        w.dstSet = m_curveDescSet[i];
        w.pBufferInfo = &curveUBI;
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);

        // Fill
        VkDescriptorSetLayout fillLayout = m_pipeline.descriptorSetLayout(PipelineType::Fill);
        ai.pSetLayouts = &fillLayout;
        if (vkAllocateDescriptorSets(dev, &ai, &m_fillDescSet[i]) != VK_SUCCESS)
            return false;

        VkDescriptorBufferInfo fillUBI = {m_fillUBO[i].buffer, 0, 64};
        w.dstSet = m_fillDescSet[i];
        w.pBufferInfo = &fillUBI;
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);

        // Glyph
        VkDescriptorSetLayout glyphLayout = m_pipeline.descriptorSetLayout(PipelineType::Glyph);
        ai.pSetLayouts = &glyphLayout;
        if (vkAllocateDescriptorSets(dev, &ai, &m_glyphDescSet[i]) != VK_SUCCESS)
            return false;

        VkDescriptorBufferInfo glyphUBI = {m_glyphUBO[i].buffer, 0, 64};

        VkWriteDescriptorSet glyphUboWrite = {};
        glyphUboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        glyphUboWrite.dstSet = m_glyphDescSet[i];
        glyphUboWrite.dstBinding = 0;
        glyphUboWrite.descriptorCount = 1;
        glyphUboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        glyphUboWrite.pBufferInfo = &glyphUBI;
        vkUpdateDescriptorSets(dev, 1, &glyphUboWrite, 0, nullptr);

        if (m_fontAtlas.atlasImageView() != VK_NULL_HANDLE && m_fontAtlas.atlasSampler() != VK_NULL_HANDLE) {
            VkDescriptorImageInfo glyphII = {m_fontAtlas.atlasSampler(), m_fontAtlas.atlasImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet samplerWrite = {};
            samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            samplerWrite.dstSet = m_glyphDescSet[i];
            samplerWrite.dstBinding = 1;
            samplerWrite.descriptorCount = 1;
            samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerWrite.pImageInfo = &glyphII;
            vkUpdateDescriptorSets(dev, 1, &samplerWrite, 0, nullptr);
        }
    }
    return true;
}

void VulkanRenderer::updateUBO(int frameIdx)
{
    GridUBO gubo;
    memset(&gubo, 0, sizeof(GridUBO));
    gubo.color[0] = static_cast<float>(m_gridColor.redF());
    gubo.color[1] = static_cast<float>(m_gridColor.greenF());
    gubo.color[2] = static_cast<float>(m_gridColor.blueF());
    gubo.color[3] = static_cast<float>(m_gridColor.alphaF());
    gubo.alpha     = 1.0f;
    if (m_gridUBO[frameIdx].mapped)
        memcpy(m_gridUBO[frameIdx].mapped, &gubo, sizeof(GridUBO));

    CurveUBO cubo;
    memset(&cubo, 0, sizeof(CurveUBO));
    cubo.color[0] = static_cast<float>(m_curveColor.redF());
    cubo.color[1] = static_cast<float>(m_curveColor.greenF());
    cubo.color[2] = static_cast<float>(m_curveColor.blueF());
    cubo.color[3] = static_cast<float>(m_curveColor.alphaF());
    cubo.lineWidth = 2.0f;
    if (m_curveUBO[frameIdx].mapped)
        memcpy(m_curveUBO[frameIdx].mapped, &cubo, sizeof(CurveUBO));

    FillUBO fubo;
    memset(&fubo, 0, sizeof(FillUBO));
    fubo.color[0] = static_cast<float>(m_fillColor.redF());
    fubo.color[1] = static_cast<float>(m_fillColor.greenF());
    fubo.color[2] = static_cast<float>(m_fillColor.blueF());
    fubo.color[3] = static_cast<float>(m_fillColor.alphaF());
    if (m_fillUBO[frameIdx].mapped)
        memcpy(m_fillUBO[frameIdx].mapped, &fubo, sizeof(FillUBO));

    GlyphUBO glubo;
    memset(&glubo, 0, sizeof(GlyphUBO));
    glubo.color[0] = static_cast<float>(m_glyphColor.redF());
    glubo.color[1] = static_cast<float>(m_glyphColor.greenF());
    glubo.color[2] = static_cast<float>(m_glyphColor.blueF());
    glubo.color[3] = static_cast<float>(m_glyphColor.alphaF());
    if (m_glyphUBO[frameIdx].mapped)
        memcpy(m_glyphUBO[frameIdx].mapped, &glubo, sizeof(GlyphUBO));
}

QVector<float> VulkanRenderer::buildGridVBO(int* outVertexCount)
{
    QVector<float> verts;
    if (!m_mapper) { if (outVertexCount) *outVertexCount = 0; return verts; }

    double gMin = m_mapper->gainMin();
    double gMax = m_mapper->gainMax();
    double fMin = m_mapper->freqMin();
    double fMax = m_mapper->freqMax();

    constexpr double GAIN_STEP_DB = 6.0;

    double g0 = std::ceil(gMin / GAIN_STEP_DB) * GAIN_STEP_DB;
    for (double g = g0; g <= gMax + 0.001; g += GAIN_STEP_DB) {
        if (std::abs(g) < 0.001) continue;
        float py = static_cast<float>(m_mapper->gainToY(g));
        float ny = toNdcY(py);
        float nxLeft  = toNdcX(MARGIN_LEFT);
        float nxRight = toNdcX(static_cast<float>(m_size.width()) - MARGIN_RIGHT);
        verts.append(nxLeft);  verts.append(ny);
        verts.append(nxRight); verts.append(ny);
    }

    for (const auto& tick : FREQ_TICKS) {
        if (tick.hz < fMin || tick.hz > fMax) continue;
        float px = static_cast<float>(m_mapper->freqToX(tick.hz));
        float nx = toNdcX(px);
        float nyTop    = toNdcY(MARGIN_TOP);
        float nyBottom = toNdcY(static_cast<float>(m_size.height()) - MARGIN_BOTTOM);
        verts.append(nx); verts.append(nyTop);
        verts.append(nx); verts.append(nyBottom);
    }

    if (outVertexCount) *outVertexCount = verts.size() / 2;
    return verts;
}

QVector<float> VulkanRenderer::buildAxisVBO()
{
    QVector<float> verts;
    float h = static_cast<float>(m_size.height());
    float w = static_cast<float>(m_size.width());

    float leftNdc   = toNdcX(MARGIN_LEFT);
    float rightNdc  = toNdcX(w - MARGIN_RIGHT);
    float topNdc    = toNdcY(MARGIN_TOP);
    float bottomNdc = toNdcY(h - MARGIN_BOTTOM);

    auto addLine = [&](float x1, float y1, float x2, float y2) {
        verts.append(x1); verts.append(y1);
        verts.append(x2); verts.append(y2);
    };

    // Left axis (vertical)
    addLine(leftNdc, topNdc, leftNdc, bottomNdc);

    // Bottom axis (horizontal) at 0dB line, not at the bottom edge
    if (m_mapper && m_mapper->gainMin() <= 0.0 && m_mapper->gainMax() >= 0.0) {
        float py = static_cast<float>(m_mapper->gainToY(0.0));
        float zeroNy = toNdcY(py);
        addLine(leftNdc, zeroNy, rightNdc, zeroNy);
    } else {
        // Fallback: use bottom edge if 0dB is not in range
        addLine(leftNdc, bottomNdc, rightNdc, bottomNdc);
    }

    return verts;
}

QVector<float> VulkanRenderer::buildCurveVBO(const QVector<QPointF>& points, float lineWidthPx)
{
    QVector<float> verts;
    if (points.size() < 2)
        return verts;

    QRectF vp;
    bool hasMapper = m_mapper != nullptr;
    if (hasMapper)
        vp = m_mapper->viewport();

    // Curve pipeline = LINE_STRIP with width set via vkCmdSetLineWidth.
    // One vertex per curve point (stride 3 floats = vec2 pos + float dist; dist unused for line topology).
    for (int i = 0; i < points.size(); ++i) {
        float xPx;
        if (hasMapper)
            xPx = static_cast<float>(vp.left() + points[i].x() * vp.width());
        else
            xPx = static_cast<float>(points[i].x());
        float yPx;
        if (hasMapper)
            yPx = static_cast<float>(m_mapper->gainToY(points[i].y()));
        else
            yPx = static_cast<float>((48.0 - points[i].y()) / 96.0 * m_size.height());

        float nx = toNdcX(xPx);
        float ny = toNdcY(yPx);

        verts.append(nx);
        verts.append(ny);
        verts.append(0.0f);
    }

    return verts;
}

void VulkanRenderer::buildTextGeometry()
{
    QVector<float> verts;
    if (!m_mapper || !m_fontAtlas.atlasImageView())
        return;

    float fontSize = LABEL_FONT_SIZE_PX;
    float fontScale = fontSize / 20.0f;

    double fMax = m_mapper->freqMax();

    for (const auto& tick : FREQ_TICKS) {
        if (tick.hz > fMax) continue;
        QString labelStr = QString::fromLatin1(tick.label);
        float px = static_cast<float>(m_mapper->freqToX(tick.hz));

        float textWidth = 0;
        for (int ci = 0; ci < labelStr.length(); ++ci) {
            GlyphInfo gi;
            if (!m_fontAtlas.glyphInfo(labelStr[ci], &gi)) continue;
            textWidth += gi.advance * fontScale;
        }
        float xBase = px - textWidth / 2.0f;

        // Freq labels sit below the 0dB axis line
        float zeroDbY = static_cast<float>(m_mapper->gainToY(0.0));
        float yBase = zeroDbY + FREQ_LABEL_Y_OFFSET_PX + fontSize;

        yBase = (std::min)(yBase, static_cast<float>(m_size.height()) - 2.0f);

        float xCursor = xBase;
        for (int ci = 0; ci < labelStr.length(); ++ci) {
            GlyphInfo gi;
            if (!m_fontAtlas.glyphInfo(labelStr[ci], &gi)) continue;
            float bx = xCursor + static_cast<float>(gi.bearingRect.x()) * fontScale;
            // stb yoff convention: top-of-glyph = baseline + yoff (yoff typically negative)
            float by = yBase + static_cast<float>(gi.bearingRect.y()) * fontScale;
            float bw = static_cast<float>(gi.bearingRect.width()) * fontScale;
            float bh = static_cast<float>(gi.bearingRect.height()) * fontScale;

            by = (std::min)(by, static_cast<float>(m_size.height()) - 2.0f - bh);
            bx = (std::max)(0.0f, bx);

            float xl = toNdcX(bx);
            float xr = toNdcX((std::min)(bx + bw, static_cast<float>(m_size.width()) - 2.0f));
            float yt = toNdcY(by);            // pixel-top = smaller pixelY ⇒ smaller NDC.Y (Vulkan NDC: +Y down)
            float yb = toNdcY(by + bh);       // pixel-bottom = larger pixelY ⇒ larger NDC.Y

            float u0 = static_cast<float>(gi.uvRect.left());
            float v0 = static_cast<float>(gi.uvRect.top());     // atlas top
            float u1 = static_cast<float>(gi.uvRect.right());
            float v1 = static_cast<float>(gi.uvRect.bottom());  // atlas bottom

            // Triangle 1: top-left, top-right, bottom-left
            verts.append({xl, yt, u0, v0});
            verts.append({xr, yt, u1, v0});
            verts.append({xl, yb, u0, v1});
            // Triangle 2: top-right, bottom-right, bottom-left
            verts.append({xr, yt, u1, v0});
            verts.append({xr, yb, u1, v1});
            verts.append({xl, yb, u0, v1});

            xCursor += gi.advance * fontScale;
        }
    }

    double gMin = m_mapper->gainMin();
    double gMax = m_mapper->gainMax();
    constexpr double GAIN_STEP_DB = 6.0;
    double g0 = std::ceil(gMin / GAIN_STEP_DB) * GAIN_STEP_DB;

    float xRightBaseline = MARGIN_LEFT - GAIN_LABEL_BASELINE_GAP_PX;

    for (double g = g0; g <= gMax + 0.001; g += GAIN_STEP_DB) {
        float py = static_cast<float>(m_mapper->gainToY(g));

        QString numText;
        if (std::abs(g) < 0.001) numText = "0";
        else if (g > 0) numText = QString::number((int)g);
        else numText = QString("-%1").arg((int)(-g));

        float strWidth = 0;
        for (QChar ch : numText) {
            GlyphInfo gi;
            if (!m_fontAtlas.glyphInfo(ch, &gi)) continue;
            strWidth += gi.advance * fontScale;
        }
        float xStart = xRightBaseline - strWidth;

        // Center digit vertically on the gridline (bearingY pushes top above baseline)
        float yBase = py + fontSize * 0.5f;
        yBase = (std::min)(yBase, static_cast<float>(m_size.height()) - 2.0f);

        float currentOffset = 0;
        for (QChar ch : numText) {
            GlyphInfo gi;
            if (!m_fontAtlas.glyphInfo(ch, &gi)) continue;

            float bx = xStart + currentOffset + static_cast<float>(gi.bearingRect.x()) * fontScale;
            float by = yBase + static_cast<float>(gi.bearingRect.y()) * fontScale;
            float bw = static_cast<float>(gi.bearingRect.width()) * fontScale;
            float bh = static_cast<float>(gi.bearingRect.height()) * fontScale;

            by = (std::min)(by, static_cast<float>(m_size.height()) - 2.0f - bh);
            bx = (std::max)(0.0f, bx);

            float xl = toNdcX(bx);
            float xr = toNdcX((std::min)(bx + bw, static_cast<float>(m_size.width()) - 2.0f));
            float yt = toNdcY(by);
            float yb = toNdcY(by + bh);

            float u0 = static_cast<float>(gi.uvRect.left());
            float v0 = static_cast<float>(gi.uvRect.top());
            float u1 = static_cast<float>(gi.uvRect.right());
            float v1 = static_cast<float>(gi.uvRect.bottom());

            verts.append({xl, yt, u0, v0});
            verts.append({xr, yt, u1, v0});
            verts.append({xl, yb, u0, v1});
            verts.append({xr, yt, u1, v0});
            verts.append({xr, yb, u1, v1});
            verts.append({xl, yb, u0, v1});

            currentOffset += gi.advance * fontScale;
        }
    }

    m_textVertexCount = verts.size() / 4;

    if (m_textVBO.buffer == VK_NULL_HANDLE) {
        m_textVBO = m_bufferPool.createVertexBuffer(verts.size() * sizeof(float));
    }
    m_bufferPool.uploadData(m_textVBO, verts.data(), verts.size() * sizeof(float));
}

QVector<float> VulkanRenderer::buildCircleVBO(float cx, float cy, float radius, int segments)
{
    QVector<float> verts;
    float ncx = toNdcX(cx);
    float ncy = toNdcY(cy);
    float nrx = radius / static_cast<float>(m_size.width()) * 2.0f;
    float nry = radius / static_cast<float>(m_size.height()) * 2.0f;

    for (int i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * 3.14159265f;
        float px = ncx + nrx * cosf(angle);
        float py = ncy + nry * sinf(angle);
        verts.append(px);
        verts.append(py);
    }
    return verts;
}

QVector<float> VulkanRenderer::buildEllipseVBO(float cx, float cy, float rx, float ry, int segments)
{
    QVector<float> verts;
    float ncx = toNdcX(cx);
    float ncy = toNdcY(cy);
    float nrx = rx / static_cast<float>(m_size.width()) * 2.0f;
    float nry = ry / static_cast<float>(m_size.height()) * 2.0f;

    for (int i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * 3.14159265f;
        float px = ncx + nrx * cosf(angle);
        float py = ncy + nry * sinf(angle);
        verts.append(px);
        verts.append(py);
    }
    return verts;
}

QVector<float> VulkanRenderer::buildFillVBO(const QVector<QPointF>& curvePoints)
{
    QVector<float> verts;
    if (curvePoints.size() < 2)
        return verts;

    QRectF vp;
    bool hasMapper = m_mapper != nullptr;
    if (hasMapper)
        vp = m_mapper->viewport();

    for (int i = 0; i < curvePoints.size(); ++i) {
        float xPx;
        if (hasMapper)
            xPx = static_cast<float>(vp.left() + curvePoints[i].x() * vp.width());
        else
            xPx = static_cast<float>(curvePoints[i].x());
        float yPx;
        if (hasMapper)
            yPx = static_cast<float>(m_mapper->gainToY(curvePoints[i].y()));
        else
            yPx = static_cast<float>((48.0 - curvePoints[i].y()) / 96.0 * m_size.height());

        float nx = toNdcX(xPx);
        float ny = toNdcY(yPx);

        verts.append(nx);
        verts.append(ny);
        verts.append(1.0f);
    }

    return verts;
}

void VulkanRenderer::ensureTotalCurveVBO(size_t floatCount)
{
    if (m_totalCurveVBO.buffer == VK_NULL_HANDLE) {
        m_totalCurveVBO = m_bufferPool.createVertexBuffer(floatCount * sizeof(float));
    }
}

void VulkanRenderer::ensureBandCurveVBO(int index, size_t floatCount)
{
    auto it = m_bandCurveVBOs.find(index);
    if (it == m_bandCurveVBOs.end() || it->buffer == VK_NULL_HANDLE) {
        m_bandCurveVBOs[index] = m_bufferPool.createVertexBuffer(floatCount * sizeof(float));
    }
}

void VulkanRenderer::renderFrame()
{
    if (!m_initialized)
        return;

    static int frameCount = 0;
    if (frameCount == 0) {
        qDebug() << "VulkanRenderer::renderFrame: first frame, gridVerts:" << m_gridVertexCount << "textVerts:" << m_textVertexCount << " curvePoints:" << m_totalCurve.size();
        frameCount = 1;
    }

    int frameIdx = m_frameSync.currentFrame();
    m_frameSync.waitAndResetFence(frameIdx);

    uint32_t imageIndex;
    if (!m_swapchain.acquireNextImage(m_frameSync.imageAvailableSemaphore(frameIdx), &imageIndex)) {
        resize(m_size);
        return;
    }

    updateUBO(frameIdx);

    if (m_gridDirty) {
        auto verts = buildGridVBO(&m_gridVertexCount);
        m_bufferPool.free(m_gridVBO);
        m_gridVBO = m_bufferPool.createVertexBuffer(verts.size() * sizeof(float));
        m_bufferPool.uploadData(m_gridVBO, verts.data(), verts.size() * sizeof(float));

        auto axisVerts = buildAxisVBO();
        m_axisVertexCount = static_cast<uint32_t>(axisVerts.size() / 2);
        m_bufferPool.free(m_axisVBO);
        m_axisVBO = m_bufferPool.createVertexBuffer(axisVerts.size() * sizeof(float));
        m_bufferPool.uploadData(m_axisVBO, axisVerts.data(), axisVerts.size() * sizeof(float));

        m_gridDirty = false;
    }

    if (m_textDirty) {
        qDebug() << "renderFrame: text dirty, rebuilding...";
        buildTextGeometry();
        m_textDirty = false;
    }

    if (m_curveDirty && !m_totalCurve.isEmpty()) {
        auto verts = buildCurveVBO(m_totalCurve, 2.0f);
        if (m_totalCurveVBO.buffer != VK_NULL_HANDLE) {
            m_bufferPool.free(m_totalCurveVBO);
        }
        m_totalCurveVBO = m_bufferPool.createVertexBuffer(verts.size() * sizeof(float));
        m_bufferPool.uploadData(m_totalCurveVBO, verts.data(), verts.size() * sizeof(float));
        m_curveDirty = false;
    }

    VkCommandBuffer cmd = m_commandBuffers[frameIdx];
    vkResetCommandBuffer(cmd, 0);
    recordCommandBuffer(cmd, imageIndex);

    VkSemaphore waitSem = m_frameSync.imageAvailableSemaphore(frameIdx);
    VkSemaphore signalSem = m_frameSync.renderFinishedSemaphore(frameIdx);
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &waitSem;
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &signalSem;

    vkQueueSubmit(m_ctx->graphicsQueue(), 1, &si, m_frameSync.inFlightFence(frameIdx));
    m_swapchain.present(m_ctx->graphicsQueue(), imageIndex, signalSem);
    m_frameSync.advanceFrame();
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(cmd, &bi);

    VkClearValue clearVals[2] = {};
    clearVals[0].color.float32[0] = static_cast<float>(m_backgroundColor.redF());
    clearVals[0].color.float32[1] = static_cast<float>(m_backgroundColor.greenF());
    clearVals[0].color.float32[2] = static_cast<float>(m_backgroundColor.blueF());
    clearVals[0].color.float32[3] = 1.0f;
    clearVals[1].color.float32[0] = static_cast<float>(m_backgroundColor.redF());
    clearVals[1].color.float32[1] = static_cast<float>(m_backgroundColor.greenF());
    clearVals[1].color.float32[2] = static_cast<float>(m_backgroundColor.blueF());
    clearVals[1].color.float32[3] = 1.0f;

    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = m_swapchain.renderPass();
    rpbi.framebuffer = m_swapchain.framebuffer(imageIndex);
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = {static_cast<uint32_t>(m_size.width()), static_cast<uint32_t>(m_size.height())};
    rpbi.clearValueCount = 2;
    rpbi.pClearValues = clearVals;
    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vp = {};
    vp.x = 0.0f;
    vp.y = 0.0f;
    vp.width = static_cast<float>(m_size.width());
    vp.height = static_cast<float>(m_size.height());
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {static_cast<uint32_t>(m_size.width()), static_cast<uint32_t>(m_size.height())};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    int frameIdx = m_frameSync.currentFrame();

    // Default: full scissor
    setFullScissor(cmd);

    // 1. Grid lines → inner scissor
    setInnerScissor(cmd);
    if (m_gridVBO.buffer != VK_NULL_HANDLE && m_gridVertexCount > 0) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline(PipelineType::Grid));
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout(PipelineType::Grid), 0, 1, &m_gridDescSet[frameIdx], 0, nullptr);
        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_gridVBO.buffer, &off);
        vkCmdDraw(cmd, static_cast<uint32_t>(m_gridVertexCount), 1, 0, 0);
    }

    // 2. Axis → full scissor (axis lines need to reach margin edges)
    setFullScissor(cmd);
    // 1b. Axis border lines (drawn after grid for visual layering, bold white)
    if (m_axisVBO.buffer != VK_NULL_HANDLE && m_axisVertexCount > 0) {
        GridUBO axisUbo;
        memset(&axisUbo, 0, sizeof(GridUBO));
        axisUbo.color[0] = static_cast<float>(m_axisColor.redF());
        axisUbo.color[1] = static_cast<float>(m_axisColor.greenF());
        axisUbo.color[2] = static_cast<float>(m_axisColor.blueF());
        axisUbo.color[3] = static_cast<float>(m_axisColor.alphaF());
        axisUbo.alpha    = 1.0f;
        memcpy(m_gridUBO[frameIdx].mapped, &axisUbo, sizeof(GridUBO));

        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_axisVBO.buffer, &off);
        vkCmdDraw(cmd, m_axisVertexCount, 1, 0, 0);
    }

    // 3. Curves → inner scissor
    setInnerScissor(cmd);
    // 2. Total Curve
    if (m_totalCurveVBO.buffer != VK_NULL_HANDLE && !m_totalCurve.isEmpty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline(PipelineType::Curve));
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout(PipelineType::Curve), 0, 1, &m_curveDescSet[frameIdx], 0, nullptr);
        vkCmdSetLineWidth(cmd, 2.0f);
        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_totalCurveVBO.buffer, &off);
        uint32_t totalCurveVerts = static_cast<uint32_t>(m_totalCurve.size());
        vkCmdDraw(cmd, totalCurveVerts, 1, 0, 0);
    }

    // 3. Band Curves
    for (auto it = m_bandCurves.begin(); it != m_bandCurves.end(); ++it) {
        int idx = it.key();
        const QVector<QPointF>& bp = it.value();
        auto vit = m_bandCurveVBOs.find(idx);
        if (vit == m_bandCurveVBOs.end() || vit->buffer == VK_NULL_HANDLE || bp.size() < 2)
            continue;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline(PipelineType::Curve));
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout(PipelineType::Curve), 0, 1, &m_curveDescSet[frameIdx], 0, nullptr);
        vkCmdSetLineWidth(cmd, 2.0f);
        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vit->buffer, &off);
        uint32_t bandVerts = static_cast<uint32_t>(bp.size());
        vkCmdDraw(cmd, bandVerts, 1, 0, 0);
    }

    // 4. Text → full scissor
    setFullScissor(cmd);
    // 4. Text labels (Glyph pipeline)
    qDebug() << "recordCmd: textVerts=" << m_textVertexCount << " textVBO=" << (m_textVBO.buffer ? "yes" : "no");
    if (m_textVertexCount > 0 && m_textVBO.buffer != VK_NULL_HANDLE) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline(PipelineType::Glyph));
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline.layout(PipelineType::Glyph), 0, 1,
            &m_glyphDescSet[frameIdx], 0, nullptr);

        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_textVBO.buffer, &off);
        vkCmdDraw(cmd, m_textVertexCount, 1, 0, 0);
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}
