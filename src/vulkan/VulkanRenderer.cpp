#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "../CoordinateMapper.h"
#include <QDebug>
#include <cmath>
#include <cstring>

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
    destroy();
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
    if (!m_fontAtlas.initialize(14.0f)) {
        qWarning("VulkanRenderer: Font atlas init failed (non-fatal)");
    }

    for (int i = 0; i < VulkanFrameSync::MAX_FRAMES_IN_FLIGHT; ++i) {
        m_gridUBO[i]  = m_bufferPool.createUniformBuffer(sizeof(UBOData));
        m_curveUBO[i] = m_bufferPool.createUniformBuffer(sizeof(UBOData));
        m_fillUBO[i]  = m_bufferPool.createUniformBuffer(sizeof(UBOData));
        m_glyphUBO[i] = m_bufferPool.createUniformBuffer(sizeof(UBOData));
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
    m_bufferPool.free(m_totalCurveVBO);
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
    m_swapchain.resize(newSize);
    m_gridDirty = true;
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
void VulkanRenderer::setCoordinateMapper(const CoordinateMapper* mapper) { m_mapper = mapper; }
void VulkanRenderer::setCurveColor(QColor c) { m_curveColor = c; }
void VulkanRenderer::setBackgroundColor(QColor c) { m_backgroundColor = c; }

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

        VkDescriptorBufferInfo gridUBI = {m_gridUBO[i].buffer, 0, sizeof(UBOData)};
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

        VkDescriptorBufferInfo curveUBI = {m_curveUBO[i].buffer, 0, sizeof(UBOData)};
        w.dstSet = m_curveDescSet[i];
        w.pBufferInfo = &curveUBI;
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);

        // Fill
        VkDescriptorSetLayout fillLayout = m_pipeline.descriptorSetLayout(PipelineType::Fill);
        ai.pSetLayouts = &fillLayout;
        if (vkAllocateDescriptorSets(dev, &ai, &m_fillDescSet[i]) != VK_SUCCESS)
            return false;

        VkDescriptorBufferInfo fillUBI = {m_fillUBO[i].buffer, 0, sizeof(UBOData)};
        w.dstSet = m_fillDescSet[i];
        w.pBufferInfo = &fillUBI;
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);

        // Glyph
        VkDescriptorSetLayout glyphLayout = m_pipeline.descriptorSetLayout(PipelineType::Glyph);
        ai.pSetLayouts = &glyphLayout;
        if (vkAllocateDescriptorSets(dev, &ai, &m_glyphDescSet[i]) != VK_SUCCESS)
            return false;

        VkDescriptorBufferInfo glyphUBI = {m_glyphUBO[i].buffer, 0, sizeof(UBOData)};

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

void VulkanRenderer::updateUBO(int frameIdx, const QColor& color)
{
    UBOData ubo;
    memset(&ubo, 0, sizeof(UBOData));
    ubo.projMatrix[0]  = 2.0f;
    ubo.projMatrix[5]  = 2.0f;
    ubo.projMatrix[10] = 1.0f;
    ubo.projMatrix[12] = -1.0f;
    ubo.projMatrix[13] = -1.0f;
    ubo.projMatrix[15] = 1.0f;

    if (m_gridUBO[frameIdx].mapped)
        memcpy(m_gridUBO[frameIdx].mapped, &ubo, sizeof(UBOData));
    if (m_curveUBO[frameIdx].mapped)
        memcpy(m_curveUBO[frameIdx].mapped, &ubo, sizeof(UBOData));
    if (m_fillUBO[frameIdx].mapped)
        memcpy(m_fillUBO[frameIdx].mapped, &ubo, sizeof(UBOData));
    if (m_glyphUBO[frameIdx].mapped)
        memcpy(m_glyphUBO[frameIdx].mapped, &ubo, sizeof(UBOData));
}

QVector<float> VulkanRenderer::buildGridVBO(int* outVertexCount)
{
    QVector<float> verts;
    int vertCount = 0;

    double gMin = -48.0;
    double gMax = 48.0;
    if (m_mapper) {
        gMin = m_mapper->gainMin();
        gMax = m_mapper->gainMax();
    }

    for (double g = gMin; g <= gMax + 1e-6; g += 12.0) {
        double normalizedGain = (gMax - g) / (gMax - gMin);
        float yPx = static_cast<float>(normalizedGain * m_size.height());
        float y = toNdcY(yPx);
        verts.append(-1.0f);
        verts.append(y);
        verts.append(1.0f);
        verts.append(y);
        vertCount += 2;
    }

    double freqs[] = {20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0};
    for (double f : freqs) {
        if (f < 20.0 || f > 22050.0)
            continue;
        float xPx = 0.0f;
        if (m_mapper)
            xPx = static_cast<float>(m_mapper->freqToX(f));
        float nx = toNdcX(xPx);
        verts.append(nx);
        verts.append(-1.0f);
        verts.append(nx);
        verts.append(1.0f);
        vertCount += 2;
    }

    *outVertexCount = vertCount;
    return verts;
}

QVector<float> VulkanRenderer::buildCurveVBO(const QVector<QPointF>& points, float lineWidthPx)
{
    QVector<float> verts;
    if (points.size() < 2)
        return verts;

    float halfW = lineWidthPx * 0.5f;

    for (int i = 0; i < points.size(); ++i) {
        float xPx = static_cast<float>(points[i].x());
        float yPx;
        if (m_mapper)
            yPx = static_cast<float>(m_mapper->gainToY(points[i].y()));
        else
            yPx = static_cast<float>((48.0 - points[i].y()) / 96.0 * m_size.height());

        float nx = toNdcX(xPx);
        float ny = toNdcY(yPx);

        verts.append(nx);
        verts.append(ny);
        verts.append(-halfW);

        verts.append(nx);
        verts.append(ny);
        verts.append(halfW);
    }

    return verts;
}

QVector<float> VulkanRenderer::buildGlyphVBO(float x, float y, const QString& text)
{
    QVector<float> verts;
    if (!m_mapper)
        return verts;

    float cx = x;
    for (int i = 0; i < text.size(); ++i) {
        QChar ch = text[i];
        GlyphInfo gi;
        if (!m_fontAtlas.glyphInfo(ch, &gi) || gi.uvRect.width() <= 0.0f) {
            cx += 8.0f;
            continue;
        }

        float bx = cx + static_cast<float>(gi.bearingRect.x());
        float by = y  - static_cast<float>(gi.bearingRect.y());
        float bw = static_cast<float>(gi.bearingRect.width());
        float bh = static_cast<float>(gi.bearingRect.height());

        float nLeft   = toNdcX(bx);
        float nRight  = toNdcX(bx + bw);
        float nTop    = toNdcY(by - bh);
        float nBottom = toNdcY(by);

        float uLeft   = static_cast<float>(gi.uvRect.x());
        float uTop    = static_cast<float>(gi.uvRect.y());
        float uRight  = static_cast<float>(gi.uvRect.x() + gi.uvRect.width());
        float uBottom = static_cast<float>(gi.uvRect.y() + gi.uvRect.height());

        // Triangle strip: 4 vertices
        verts.append(nLeft);
        verts.append(nBottom);
        verts.append(uLeft);
        verts.append(uBottom);
        verts.append(nLeft);
        verts.append(nTop);
        verts.append(uLeft);
        verts.append(uTop);
        verts.append(nRight);
        verts.append(nBottom);
        verts.append(uRight);
        verts.append(uBottom);
        verts.append(nRight);
        verts.append(nTop);
        verts.append(uRight);
        verts.append(uTop);

        cx += gi.advance;
    }
    return verts;
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

    for (int i = 0; i < curvePoints.size(); ++i) {
        float xPx = static_cast<float>(curvePoints[i].x());
        float yPx;
        if (m_mapper)
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

    int frameIdx = m_frameSync.currentFrame();
    m_frameSync.waitAndResetFence(frameIdx);

    uint32_t imageIndex;
    if (!m_swapchain.acquireNextImage(m_frameSync.imageAvailableSemaphore(frameIdx), &imageIndex)) {
        resize(m_size);
        return;
    }

    updateUBO(frameIdx, m_curveColor);

    if (m_gridDirty) {
        auto verts = buildGridVBO(&m_gridVertexCount);
        m_bufferPool.free(m_gridVBO);
        m_gridVBO = m_bufferPool.createVertexBuffer(verts.size() * sizeof(float));
        m_bufferPool.uploadData(m_gridVBO, verts.data(), verts.size() * sizeof(float));
        m_gridDirty = false;
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

    VkClearValue clearVal = {};
    clearVal.color.float32[0] = static_cast<float>(m_backgroundColor.redF());
    clearVal.color.float32[1] = static_cast<float>(m_backgroundColor.greenF());
    clearVal.color.float32[2] = static_cast<float>(m_backgroundColor.blueF());
    clearVal.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = m_swapchain.renderPass();
    rpbi.framebuffer = m_swapchain.framebuffer(imageIndex);
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = {static_cast<uint32_t>(m_size.width()), static_cast<uint32_t>(m_size.height())};
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clearVal;
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

    // 1. Grid
    if (m_gridVBO.buffer != VK_NULL_HANDLE && m_gridVertexCount > 0) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline(PipelineType::Grid));
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout(PipelineType::Grid), 0, 1, &m_gridDescSet[frameIdx], 0, nullptr);
        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_gridVBO.buffer, &off);
        vkCmdDraw(cmd, static_cast<uint32_t>(m_gridVertexCount), 1, 0, 0);
    }

    // 2. Total Curve
    if (m_totalCurveVBO.buffer != VK_NULL_HANDLE && !m_totalCurve.isEmpty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline(PipelineType::Curve));
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout(PipelineType::Curve), 0, 1, &m_curveDescSet[frameIdx], 0, nullptr);
        vkCmdSetLineWidth(cmd, 2.0f);
        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_totalCurveVBO.buffer, &off);
        uint32_t totalCurveVerts = static_cast<uint32_t>(m_totalCurve.size() * 2);
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
        uint32_t bandVerts = static_cast<uint32_t>(bp.size() * 2);
        vkCmdDraw(cmd, bandVerts, 1, 0, 0);
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}
