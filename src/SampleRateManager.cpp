#include "SampleRateManager.h"
#include "EqualizerModel.h"
#include "CurveEngine.h"
#include "CoordinateMapper.h"

SampleRateManager::SampleRateManager(EqualizerModel* model, CurveEngine* engine,
                                     CoordinateMapper* mapper, QObject* parent)
	: QObject(parent), m_model(model), m_engine(engine), m_mapper(mapper)
{
}

SampleRate SampleRateManager::sampleRate() const
{
	return m_model->sampleRate();
}

void SampleRateManager::setSampleRate(SampleRate rate)
{
	if (m_model->sampleRate() == rate)
		return;

	m_model->setSampleRate(rate);

	double nyquist = m_model->nyquistFrequency();
	m_mapper->setFreqRange(m_mapper->freqMin(), nyquist);
	m_engine->setFreqRange(m_mapper->freqMin(), nyquist);

	emit sampleRateChanged(rate);
	emit nyquistChanged(nyquist);

	m_engine->requestTotalCurve(*m_model);
}

double SampleRateManager::nyquistFrequency() const
{
	return m_model->nyquistFrequency();
}
