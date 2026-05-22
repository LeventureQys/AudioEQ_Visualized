#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDebug>
#include "AudioEQ.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    QMainWindow window;
    window.setWindowTitle("AudioEQ Simple Demo");
    window.resize(800, 500);
    
    QWidget* central = new QWidget(&window);
    QVBoxLayout* layout = new QVBoxLayout(central);
    
    // 检查Vulkan
    QLabel* statusLabel = new QLabel(central);
    if (AudioEQ::isVulkanSupported()) {
        statusLabel->setText("Vulkan: Available");
    } else {
        statusLabel->setText("Vulkan: NOT Available (fallback)");
    }
    layout->addWidget(statusLabel);
    
    // 创建 AudioEQ
    AudioEQ* eq = new AudioEQ(central);
    eq->setMinimumHeight(300);
    layout->addWidget(eq);
    
    // 控制按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    QPushButton* addBandBtn = new QPushButton("Add Band");
    QPushButton* removeBandBtn = new QPushButton("Remove Band");
    QPushButton* sr441Btn = new QPushButton("SR 44100");
    QPushButton* sr96kBtn = new QPushButton("SR 96000");
    
    btnLayout->addWidget(addBandBtn);
    btnLayout->addWidget(removeBandBtn);
    btnLayout->addWidget(sr441Btn);
    btnLayout->addWidget(sr96kBtn);
    layout->addLayout(btnLayout);
    
    // 连接信号
    QObject::connect(eq, &AudioEQ::bandChanged, [](int idx) {
        qDebug() << "Band changed:" << idx;
    });
    
    QObject::connect(addBandBtn, &QPushButton::clicked, [eq]() {
        EQBand band;
        band.frequency = 2000.0;
        band.gain = 3.0;
        band.q = 1.5;
        int outIdx;
        ResultCode rc = eq->addBand(band, &outIdx);
        qDebug() << "Add band result:" << static_cast<int>(rc) << "index:" << outIdx;
    });
    
    QObject::connect(removeBandBtn, &QPushButton::clicked, [eq]() {
        int count = eq->bandCount();
        if (count > 0) {
            ResultCode rc = eq->removeBand(count - 1);
            qDebug() << "Remove band result:" << static_cast<int>(rc);
        }
    });
    
    QObject::connect(sr441Btn, &QPushButton::clicked, [eq]() {
        eq->setSampleRate(SampleRate::SR_44100);
        qDebug() << "Sample rate: 44100";
    });
    
    QObject::connect(sr96kBtn, &QPushButton::clicked, [eq]() {
        eq->setSampleRate(SampleRate::SR_96000);
        qDebug() << "Sample rate: 96000";
    });
    
    window.setCentralWidget(central);
    window.show();
    
    // 初始设置
    EQBand params;
    params.frequency = 1000.0;
    params.gain = 6.0;
    params.q = 2.0;
    params.type = FilterType::Peak;
    eq->setBandParams(0, params);
    
    return app.exec();
}
