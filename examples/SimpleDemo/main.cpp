#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QPushButton>
#include <QDebug>
#include "AudioEQ.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    if (!AudioEQ::isVulkanSupported()) {
        qCritical("Vulkan is not supported on this device.");
        return 1;
    }

    QMainWindow window;
    window.setWindowTitle("AudioEQ SimpleDemo");
    window.resize(900, 500);

    auto* eq = new AudioEQ(&window);
    window.setCentralWidget(eq);

    // Toolbar
    auto* toolbar = window.addToolBar("Controls");

    // Sample rate buttons
    auto* btn44k = new QPushButton("44.1kHz");
    auto* btn48k = new QPushButton("48kHz");
    auto* btn96k = new QPushButton("96kHz");
    QObject::connect(btn44k, &QPushButton::clicked, [eq]{ eq->setSampleRate(SampleRate::SR_44100); });
    QObject::connect(btn48k, &QPushButton::clicked, [eq]{ eq->setSampleRate(SampleRate::SR_48000); });
    QObject::connect(btn96k, &QPushButton::clicked, [eq]{ eq->setSampleRate(SampleRate::SR_96000); });
    toolbar->addWidget(btn44k);
    toolbar->addWidget(btn48k);
    toolbar->addWidget(btn96k);
    toolbar->addSeparator();

    // Band count buttons
    auto* btn3 = new QPushButton("3 Bands");
    auto* btn5 = new QPushButton("5 Bands");
    auto* btn8 = new QPushButton("8 Bands");
    QObject::connect(btn3, &QPushButton::clicked, [eq]{ eq->setBandCount(3); });
    QObject::connect(btn5, &QPushButton::clicked, [eq]{ eq->setBandCount(5); });
    QObject::connect(btn8, &QPushButton::clicked, [eq]{ eq->setBandCount(8); });
    toolbar->addWidget(btn3);
    toolbar->addWidget(btn5);
    toolbar->addWidget(btn8);
    toolbar->addSeparator();

    // LPF / HPF toggle
    auto* btnLpf = new QPushButton("LPF");
    btnLpf->setCheckable(true);
    QObject::connect(btnLpf, &QPushButton::toggled, [eq](bool on){ eq->setLpfEnabled(on); });
    auto* btnHpf = new QPushButton("HPF");
    btnHpf->setCheckable(true);
    QObject::connect(btnHpf, &QPushButton::toggled, [eq](bool on){ eq->setHpfEnabled(on); });
    toolbar->addWidget(btnLpf);
    toolbar->addWidget(btnHpf);

    window.show();
    return app.exec();
}
