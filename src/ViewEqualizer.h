#pragma once
#include <QWidget>
#include "AudioEQTypes.h"

class VulkanRenderer;

class AUDIOEQ_EXPORT ViewEqualizer : public QWidget {
    Q_OBJECT
public:
    explicit ViewEqualizer(QWidget* parent = nullptr);

private:
    VulkanRenderer* m_renderer = nullptr;
};
