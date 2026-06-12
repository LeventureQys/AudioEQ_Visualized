#include "AudioEQ.h"
#include "EqualizerModel.h"
#include "CurveEngine.h"
#include "CoordinateMapper.h"
#include "ViewEqualizer.h"
#include "vulkan/VulkanRenderer.h"
#include "vulkan/VulkanContext.h"
#include <QVBoxLayout>
#include <QDebug>
#include <cmath>
#include <algorithm>

bool AudioEQ::isVulkanSupported() {
    return VulkanContext::isVulkanSupported();
}

AudioEQ::AudioEQ(QWidget* parent) : QWidget(parent) {
    initializeComponents();
}

AudioEQ::~AudioEQ() {
    if (m_ctx && m_ctx->device() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_ctx->device());
    }

    delete m_engine;
    m_engine = nullptr;

    if (m_view) {
        delete m_view;
        m_view = nullptr;
    }

    if (m_renderer) {
        m_renderer->destroy();
        delete m_renderer;
        m_renderer = nullptr;
    }

    delete m_ctx;
    m_ctx = nullptr;
    delete m_mapper;
    m_mapper = nullptr;
}

void AudioEQ::initializeComponents() {
    qDebug() << "AudioEQ::initializeComponents: starting";
    m_ctx = new VulkanContext();
    if (!m_ctx->initialize()) {
        qWarning("AudioEQ: Vulkan not available");
        return;
    }
    qDebug() << "AudioEQ::initializeComponents: VulkanContext initialized";

    m_model = new EqualizerModel(this);
    qDebug() << "AudioEQ::initializeComponents: EqualizerModel created";

    m_mapper = new CoordinateMapper();
    m_mapper->setSampleRate(m_model->sampleRate());

    m_engine = new CurveEngine();

    m_renderer = new VulkanRenderer(m_ctx, this);
    m_renderer->setCoordinateMapper(m_mapper);
    qDebug() << "AudioEQ::initializeComponents: VulkanRenderer created";

    m_view = new ViewEqualizer(this);
    m_view->setVulkanContext(m_ctx);
    qDebug() << "AudioEQ::initializeComponents: calling setRenderer...";
    m_view->setRenderer(m_renderer);
    qDebug() << "AudioEQ::initializeComponents: setRenderer done, calling setCoordinateMapper...";
    m_view->setCoordinateMapper(m_mapper);
    m_view->setModel(m_model);
    qDebug() << "AudioEQ::initializeComponents: ViewEqualizer fully set up";

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    connect(m_model, &EqualizerModel::bandChanged, this, &AudioEQ::onBandChanged);
    connect(m_model, &EqualizerModel::bandAdded, this, &AudioEQ::onBandAdded);
    connect(m_model, &EqualizerModel::bandRemoved, this, &AudioEQ::onBandRemoved);
    connect(m_model, &EqualizerModel::lpfChanged, this, &AudioEQ::onLpfChanged);
    connect(m_model, &EqualizerModel::hpfChanged, this, &AudioEQ::onHpfChanged);
    connect(m_model, &EqualizerModel::sampleRateChanged, this, &AudioEQ::onSampleRateChanged);
    connect(m_model, &EqualizerModel::focusedBandChanged, this, &AudioEQ::onFocusedBandChanged);
    connect(m_model, &EqualizerModel::gainRangeChanged, this, &AudioEQ::onGainRangeChanged);

    connect(m_view, &ViewEqualizer::bandDragged, this, &AudioEQ::onBandDragged);
    connect(m_view, &ViewEqualizer::bandClicked, this, &AudioEQ::onBandClicked);
    connect(m_view, &ViewEqualizer::bandReleased, this, &AudioEQ::onBandReleased);
    connect(m_view, &ViewEqualizer::lpfDragged, this, &AudioEQ::onLpfDragged);
    connect(m_view, &ViewEqualizer::hpfDragged, this, &AudioEQ::onHpfDragged);

    connect(m_engine, &CurveEngine::totalCurveReady, this, &AudioEQ::onTotalCurveReady);
    connect(m_engine, &CurveEngine::bandCurveReady, this, &AudioEQ::onBandCurveReady);

    if (m_mapper && m_view) {
        m_mapper->setViewport(QRectF(ViewEqualizer::MARGIN_LEFT, ViewEqualizer::MARGIN_TOP,
                                      800 - ViewEqualizer::MARGIN_LEFT - ViewEqualizer::MARGIN_RIGHT,
                                      500 - ViewEqualizer::MARGIN_TOP - ViewEqualizer::MARGIN_BOTTOM));
    }

    m_initialized = true;
    qDebug() << "AudioEQ::initializeComponents: calling setBandCount(5)...";
    setBandCount(5);
    qDebug() << "AudioEQ::initializeComponents: complete";
}

int AudioEQ::bandCount() const {
    return m_model ? m_model->bandCount() : 0;
}

ResultCode AudioEQ::setBandCount(int count) {
    if (!m_model) return ResultCode::Failed;
    if (count < 0) return ResultCode::InvalidParameter;

    int current = m_model->bandCount();
    if (count > current) {
        for (int i = current; i < count; ++i) {
            EQBand b;
            if (count > 1) {
                b.freqHz = 20.0 * std::pow(20000.0 / 20.0, static_cast<double>(i) / (count - 1));
            } else {
                b.freqHz = 1000.0;
            }
            m_model->addBand(b);
        }
    } else {
        while (m_model->bandCount() > count) {
            m_model->removeBand(m_model->bandCount() - 1);
        }
    }
    return ResultCode::OK;
}

ResultCode AudioEQ::addBand(const EQBand& band, int* outIndex) {
    if (!m_model) return ResultCode::Failed;
    return m_model->addBand(band, outIndex);
}

ResultCode AudioEQ::removeBand(int index) {
    if (!m_model) return ResultCode::Failed;
    return m_model->removeBand(index);
}

EQBand AudioEQ::bandParams(int index) const {
    if (!m_model) return {};
    return m_model->band(index);
}

ResultCode AudioEQ::setBandParams(int index, const EQBand& params) {
    if (!m_model) return ResultCode::Failed;
    return m_model->setBand(index, params);
}

ResultCode AudioEQ::setBandFrequency(int index, double freqHz) {
    if (!m_model) return ResultCode::Failed;
    return m_model->setBandFrequency(index, freqHz);
}

ResultCode AudioEQ::setBandGain(int index, double gainDb) {
    if (!m_model) return ResultCode::Failed;
    return m_model->setBandGain(index, gainDb);
}

ResultCode AudioEQ::setBandQ(int index, double q) {
    if (!m_model) return ResultCode::Failed;
    return m_model->setBandQ(index, q);
}

ResultCode AudioEQ::setBandType(int index, FilterType type) {
    if (!m_model) return ResultCode::Failed;
    return m_model->setBandType(index, type);
}

ResultCode AudioEQ::setBandAlgorithm(int index, FilterAlgorithmType algo) {
    if (!m_model) return ResultCode::Failed;
    return m_model->setBandAlgorithm(index, algo);
}

ResultCode AudioEQ::setBandBypass(int index, bool bypass) {
    if (!m_model) return ResultCode::Failed;
    return m_model->setBandBypass(index, bypass);
}

int AudioEQ::focusedBandIndex() const {
    return m_model ? m_model->focusedBandIndex() : -1;
}

void AudioEQ::setFocusedBandIndex(int index) {
    if (m_model) m_model->setFocusedBandIndex(index);
}

SampleRate AudioEQ::sampleRate() const {
    return m_model ? m_model->sampleRate() : SampleRate::SR_44100;
}

ResultCode AudioEQ::setSampleRate(SampleRate rate) {
    if (!m_model) return ResultCode::Failed;
    if (m_mapper) m_mapper->setSampleRate(rate);
    return m_model->setSampleRate(rate);
}

ResultCode AudioEQ::setLpfEnabled(bool enabled)  { return m_model ? m_model->setLpfEnabled(enabled) : ResultCode::Failed; }
bool       AudioEQ::isLpfEnabled() const         { return m_model ? m_model->lpf().enabled : false; }
ResultCode AudioEQ::setLpfFrequency(double hz)    { return m_model ? m_model->setLpfFrequency(hz) : ResultCode::Failed; }
ResultCode AudioEQ::setLpfAlgorithm(FilterAlgorithmType a) { return m_model ? m_model->setLpfAlgorithm(a) : ResultCode::Failed; }

ResultCode AudioEQ::setHpfEnabled(bool enabled)  { return m_model ? m_model->setHpfEnabled(enabled) : ResultCode::Failed; }
bool       AudioEQ::isHpfEnabled() const         { return m_model ? m_model->hpf().enabled : false; }
ResultCode AudioEQ::setHpfFrequency(double hz)    { return m_model ? m_model->setHpfFrequency(hz) : ResultCode::Failed; }
ResultCode AudioEQ::setHpfAlgorithm(FilterAlgorithmType a) { return m_model ? m_model->setHpfAlgorithm(a) : ResultCode::Failed; }

ResultCode AudioEQ::setQRange(FilterType type, double min, double max) {
    return m_model ? m_model->setQRange(type, min, max) : ResultCode::Failed;
}
QPair<double,double> AudioEQ::qRange(FilterType type) const {
    if (!m_model) return {0.1, 10.0};
    return {m_model->qMin(type), m_model->qMax(type)};
}
ResultCode AudioEQ::setGainRange(double min, double max) {
    if (!m_model) return ResultCode::Failed;
    if (m_mapper) m_mapper->setGainRange(min, max);
    return m_model->setGainRange(min, max);
}
double AudioEQ::gainMin() const { return m_model ? m_model->gainMin() : -48.0; }
double AudioEQ::gainMax() const { return m_model ? m_model->gainMax() : 48.0; }
void AudioEQ::setPointCount(int c) { m_pointCount = c; if (m_engine) m_engine->setPointCount(c); }
int  AudioEQ::pointCount() const { return m_pointCount; }
void AudioEQ::setCurveColor(QColor c) { if (m_renderer) m_renderer->setCurveColor(c); }
void AudioEQ::setBackgroundColor(QColor c) { if (m_renderer) m_renderer->setBackgroundColor(c); }

void AudioEQ::onBandChanged(int index) {
    requestCurveUpdate();
    emit bandParamsChanged(index);
}

void AudioEQ::onBandAdded(int index) {
    m_view->syncBandHandles(m_model->bandCount());
    requestCurveUpdate();
}

void AudioEQ::onBandRemoved(int index) {
    m_view->syncBandHandles(m_model->bandCount());
    requestCurveUpdate();
}

void AudioEQ::onLpfChanged() { requestCurveUpdate(); }
void AudioEQ::onHpfChanged() { requestCurveUpdate(); }
void AudioEQ::onSampleRateChanged(SampleRate rate) {
    if (m_mapper) m_mapper->setSampleRate(rate);
    requestCurveUpdate();
    emit sampleRateChanged(rate);
}
void AudioEQ::onFocusedBandChanged(int index) {
    syncRendererBands();
    emit focusedBandChanged(index);
}
void AudioEQ::onGainRangeChanged(double min, double max) {
    if (m_mapper) m_mapper->setGainRange(min, max);
}

void AudioEQ::onBandDragged(int index, double deltaX, double deltaY) {
    if (!m_model || !m_mapper) return;
    EQBand b = m_model->band(index);
    QPointF curPx = m_mapper->toPixel(b.freqHz, b.gainDb);

    double newX = curPx.x() + deltaX;
    double newY = curPx.y() + deltaY;

    double newFreq = m_mapper->xToFreq(newX);
    double newGain = m_mapper->yToGain(newY);

    newFreq = std::clamp(newFreq, m_mapper->freqMin(), m_mapper->freqMax());
    newGain = std::clamp(newGain, m_mapper->gainMin(), m_mapper->gainMax());

    m_model->setBandFrequency(index, newFreq);
    m_model->setBandGain(index, newGain);
}

void AudioEQ::onLpfDragged(double deltaX) {
    if (!m_model || !m_mapper) return;
    ShelfBand lpf = m_model->lpf();
    double curX = m_mapper->freqToX(lpf.freqHz);
    double newFreq = m_mapper->xToFreq(curX + deltaX);
    newFreq = std::clamp(newFreq, m_mapper->freqMin(), m_mapper->freqMax());
    m_model->setLpfFrequency(newFreq);
}

void AudioEQ::onHpfDragged(double deltaX) {
    if (!m_model || !m_mapper) return;
    ShelfBand hpf = m_model->hpf();
    double curX = m_mapper->freqToX(hpf.freqHz);
    double newFreq = m_mapper->xToFreq(curX + deltaX);
    newFreq = std::clamp(newFreq, m_mapper->freqMin(), m_mapper->freqMax());
    m_model->setHpfFrequency(newFreq);
}

void AudioEQ::onBandClicked(int index) {
    if (m_model) m_model->setFocusedBandIndex(index);
}

void AudioEQ::onBandReleased(int index) {
    Q_UNUSED(index);
}

void AudioEQ::onTotalCurveReady(QVector<QPointF> points) {
    if (m_renderer) {
        m_renderer->setTotalCurve(points);
    }
}

void AudioEQ::onBandCurveReady(int index, QVector<QPointF> points) {
    if (m_renderer) {
        m_renderer->setBandCurve(index, points);
    }
}

void AudioEQ::requestCurveUpdate() {
    if (!m_engine || !m_model) return;

    CurveRequest req;
    for (int i = 0; i < m_model->bandCount(); ++i) {
        req.bands.append(m_model->band(i));
    }
    req.lpf = m_model->lpf();
    req.hpf = m_model->hpf();
    req.sampleRate = m_model->sampleRate();
    req.freqMin = m_mapper->freqMin();
    req.freqMax = m_mapper->freqMax();
    req.pointCount = m_pointCount;

    m_engine->requestTotalCurve(req);
}

void AudioEQ::syncRendererBands() {
    if (!m_renderer || !m_model) return;
    QVector<BandRenderData> bands;
    for (int i = 0; i < m_model->bandCount(); ++i) {
        EQBand b = m_model->band(i);
        BandRenderData d;
        d.freqHz  = b.freqHz;
        d.gainDb  = b.gainDb;
        d.index   = i;
        d.focused = (i == m_model->focusedBandIndex());
        d.bypass  = b.bypass;
        bands.append(d);
    }
    m_renderer->setBands(bands);

    ShelfBand lpf = m_model->lpf();
    m_renderer->setLpf({lpf.freqHz, lpf.enabled});
    ShelfBand hpf = m_model->hpf();
    m_renderer->setHpf({hpf.freqHz, hpf.enabled});
}
