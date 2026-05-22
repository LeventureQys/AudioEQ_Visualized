#include "AudioEQ.h"
#include "EqualizerModel.h"
#include "CurveEngine.h"
#include "CoordinateMapper.h"
#include "ViewEqualizer.h"
#include "filter/FilterAlgorithmFactory.h"
#include "filter/ButterworthIIR.h"

#ifdef AUDIOEQ_WITH_VULKAN
#include "vulkan/VulkanQtIntegration.h"
#include "vulkan/VulkanContext.h"
#endif

static int s_filterAlgorithmRegistry = []() {
    FilterAlgorithmFactory::registerAlgorithm(FilterAlgorithmType::ButterworthIIR,
        []() -> std::unique_ptr<FilterAlgorithm> { return std::make_unique<ButterworthIIR>(); });
    return 0;
}();

AudioEQ::AudioEQ(QWidget* parent)
    : QWidget(parent)
{
    initModel();
    initEngine();
    connectSignals();
}

AudioEQ::~AudioEQ() = default;

bool AudioEQ::isVulkanSupported()
{
#ifdef AUDIOEQ_WITH_VULKAN
    return VulkanContext::isVulkanSupported();
#else
    return false;
#endif
}

int AudioEQ::bandCount() const
{
    return m_model->bandCount();
}

ResultCode AudioEQ::setBandCount(int count)
{
    return m_model->setBandCount(count);
}

ResultCode AudioEQ::addBand(const EQBand& band, int* outIndex)
{
    return m_model->addBand(band, outIndex);
}

ResultCode AudioEQ::removeBand(int index)
{
    return m_model->removeBand(index);
}

EQBand AudioEQ::bandParams(int index) const
{
    return m_model->bandAt(index);
}

ResultCode AudioEQ::setBandParams(int index, const EQBand& params)
{
    return m_model->setBandParams(index, params);
}

int AudioEQ::focusedBandIndex() const
{
    return m_model->focusedBandIndex();
}

void AudioEQ::setFocusedBandIndex(int index)
{
    m_model->setFocusedBandIndex(index);
}

SampleRate AudioEQ::sampleRate() const
{
    return m_model->sampleRate();
}

ResultCode AudioEQ::setSampleRate(SampleRate rate)
{
    return m_model->setSampleRate(rate);
}

ResultCode AudioEQ::setLpfEnabled(bool enabled)
{
    return m_model->setLpfEnabled(enabled);
}

bool AudioEQ::isLpfEnabled() const
{
    return m_model->lpf().enabled;
}

ResultCode AudioEQ::setHpfEnabled(bool enabled)
{
    return m_model->setHpfEnabled(enabled);
}

bool AudioEQ::isHpfEnabled() const
{
    return m_model->hpf().enabled;
}

ResultCode AudioEQ::setCurvePointCount(int count)
{
    m_curveEngine->setPointCount(count);
    return ResultCode::OK;
}

void AudioEQ::initModel()
{
    m_model = std::make_unique<EqualizerModel>();
    m_model->setBandCount(5);
}

void AudioEQ::initEngine()
{
    m_filterAlgo = FilterAlgorithmFactory::create(FilterAlgorithmType::ButterworthIIR);
    m_curveEngine = std::make_unique<CurveEngine>(m_filterAlgo.get());
    m_mapper = std::make_unique<CoordinateMapper>(QRect(0, 0, 800, 600), 10.0, 24000.0, -48.0, 48.0);

#ifdef AUDIOEQ_WITH_VULKAN
    if (VulkanContext::isVulkanSupported()) {
        m_vulkan = std::make_unique<VulkanQtIntegration>();
        m_vulkan->initialize(this);
    }
#endif
}

void AudioEQ::initView()
{
    m_view = std::make_unique<ViewEqualizer>(this);
    m_view->setVisible(false);
}

void AudioEQ::connectSignals()
{
    QObject::connect(m_model.get(), &EqualizerModel::bandChanged, this, [this](int index) {
        m_curveEngine->requestTotalCurve(*m_model);
        emit bandChanged(index);
    });

    QObject::connect(m_model.get(), &EqualizerModel::bandAdded, this, [this](int index) {
        m_curveEngine->requestTotalCurve(*m_model);
        emit bandAdded(index);
    });

    QObject::connect(m_model.get(), &EqualizerModel::bandRemoved, this, [this](int index) {
        m_curveEngine->requestTotalCurve(*m_model);
        emit bandRemoved(index);
    });

    QObject::connect(m_model.get(), &EqualizerModel::sampleRateChanged, this, [this](SampleRate) {
        m_curveEngine->requestTotalCurve(*m_model);
    });

    QObject::connect(m_model.get(), &EqualizerModel::lpfChanged, this, [this]() {
        m_curveEngine->requestTotalCurve(*m_model);
    });

    QObject::connect(m_model.get(), &EqualizerModel::hpfChanged, this, [this]() {
        m_curveEngine->requestTotalCurve(*m_model);
    });

    QObject::connect(m_curveEngine.get(), &CurveEngine::totalCurveReady, this,
                     [](const QVector<QPointF>&) {
    });
}
