#pragma once
#include <QObject>
#include "AudioEQTypes.h"

class EqualizerModel;
class CurveEngine;
class CoordinateMapper;

class AUDIOEQ_EXPORT SampleRateManager : public QObject {
	Q_OBJECT
public:
	explicit SampleRateManager(EqualizerModel* model, CurveEngine* engine,
	                            CoordinateMapper* mapper, QObject* parent = nullptr);

	SampleRate sampleRate() const;
	void setSampleRate(SampleRate rate);
	double nyquistFrequency() const;

signals:
	void sampleRateChanged(SampleRate rate);
	void nyquistChanged(double nyquistHz);

private:
	EqualizerModel* m_model;
	CurveEngine* m_engine;
	CoordinateMapper* m_mapper;
};
