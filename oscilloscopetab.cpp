#include "oscilloscopetab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPainter>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

OscilloscopePlot::OscilloscopePlot(QWidget *parent) : QWidget(parent) {
    setMinimumSize(400, 300);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void OscilloscopePlot::setParameters(int fx, int fy, float phase) {
    m_fx = fx;
    m_fy = fy;
    m_phase = phase;
    update();
}

void OscilloscopePlot::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::black);
    painter.setPen(QPen(QColor(50, 255, 50), 2));

    int width = this->width();
    int height = this->height();
    int cx = width / 2;
    int cy = height / 2;
    int radius = std::min(width, height) / 2 - 10;

    QPointF lastPoint;
    bool first = true;
    int numVisualPoints = 2000;
    float t_max = 2.0f * M_PI * std::max(m_fx, m_fy);

    for (int i = 0; i <= numVisualPoints; ++i) {
        float t = (static_cast<float>(i) / numVisualPoints) * t_max;
        float x = std::sin(m_fx * t);
        float y = std::sin(m_fy * t + m_phase);
        QPointF currentPoint(cx + x * radius, cy - y * radius);

        if (!first) {
            painter.drawLine(lastPoint, currentPoint);
        }
        lastPoint = currentPoint;
        first = false;
    }
}


OscilloscopeTab::OscilloscopeTab(QWidget *parent) : QWidget(parent) {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);


    plotWidget = new OscilloscopePlot(this);
    mainLayout->addWidget(plotWidget, 2);


    QVBoxLayout* controlLayout = new QVBoxLayout();

    presetCombo = new QComboBox(this);
    presetCombo->addItems({"Custom / Manual", "Perfect Circle (Unison)", "Diagonal Line (Unison)",
                           "Parabola (Octave)", "Figure-Eight (Octave)",
                           "3-Lobe Interlace (Perfect 5th)", "4-Lobe Interlace (Perfect 4th)"});
    controlLayout->addWidget(presetCombo);

    QFormLayout* formLayout = new QFormLayout();

    waveformCombo = new QComboBox(this);
    waveformCombo->addItems({"sinew", "trianglew", "saww", "squarew"});
    formLayout->addRow("Waveform:", waveformCombo);

    freqXSpin = new QSpinBox(this);
    freqXSpin->setRange(1, 20); freqXSpin->setValue(3);

    freqYSpin = new QSpinBox(this);
    freqYSpin->setRange(1, 20); freqYSpin->setValue(2);

    phaseStartSlider = new QSlider(Qt::Horizontal, this);
    phaseStartSlider->setRange(0, 360); phaseStartSlider->setValue(0);

    phaseEndSlider = new QSlider(Qt::Horizontal, this);
    phaseEndSlider->setRange(0, 360); phaseEndSlider->setValue(180);

    timeSpin = new QDoubleSpinBox(this);
    timeSpin->setRange(0.1, 20.0); timeSpin->setValue(4.0); timeSpin->setSuffix(" s");

    pingPongCheck = new QCheckBox("Tempo Sync Ping-Pong", this);

    formLayout->addRow("Freq Ratio X:", freqXSpin);
    formLayout->addRow("Freq Ratio Y:", freqYSpin);
    formLayout->addRow("Phase Start (°):", phaseStartSlider);
    formLayout->addRow("Phase End (°):", phaseEndSlider);
    formLayout->addRow("Sweep Time:", timeSpin);
    formLayout->addRow("Animation:", pingPongCheck);

    controlLayout->addLayout(formLayout);

    generateBtn = new QPushButton("Generate Stereo PCM", this);
    controlLayout->addWidget(generateBtn);

    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    controlLayout->addWidget(line);

    generateStringBtn = new QPushButton("Generate Xpressive String", this);
    controlLayout->addWidget(generateStringBtn);

    stringOutput = new QTextEdit(this);
    stringOutput->setReadOnly(true);
    stringOutput->setMinimumHeight(150);
    controlLayout->addWidget(stringOutput);

    mainLayout->addLayout(controlLayout, 1);

    // Connections
    connect(presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OscilloscopeTab::loadPreset);
    connect(freqXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &OscilloscopeTab::updateParameters);
    connect(freqYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &OscilloscopeTab::updateParameters);
    connect(phaseStartSlider, &QSlider::valueChanged, this, &OscilloscopeTab::updateParameters);
    connect(generateBtn, &QPushButton::clicked, this, &OscilloscopeTab::generateTimbre);
    connect(generateStringBtn, &QPushButton::clicked, this, &OscilloscopeTab::generateString);

    updateParameters();
}

void OscilloscopeTab::loadPreset(int index) {
    freqXSpin->blockSignals(true); freqYSpin->blockSignals(true); phaseStartSlider->blockSignals(true);

    switch (index) {
    case 1: freqXSpin->setValue(1); freqYSpin->setValue(1); phaseStartSlider->setValue(90); break;
    case 2: freqXSpin->setValue(1); freqYSpin->setValue(1); phaseStartSlider->setValue(0); break;
    case 3: freqXSpin->setValue(1); freqYSpin->setValue(2); phaseStartSlider->setValue(90); break;
    case 4: freqXSpin->setValue(1); freqYSpin->setValue(2); phaseStartSlider->setValue(0); break;
    case 5: freqXSpin->setValue(3); freqYSpin->setValue(2); phaseStartSlider->setValue(90); break;
    case 6: freqXSpin->setValue(3); freqYSpin->setValue(4); phaseStartSlider->setValue(90); break;
    }

    freqXSpin->blockSignals(false); freqYSpin->blockSignals(false); phaseStartSlider->blockSignals(false);
    updateParameters();
}

void OscilloscopeTab::updateParameters() {

    int fx = freqXSpin->value();
    int fy = freqYSpin->value();
    float phase = phaseStartSlider->value() * M_PI / 180.0f;
    plotWidget->setParameters(fx, fy, phase);
}

void OscilloscopeTab::generateTimbre() {
    int sampleRate = 44100;
    std::vector<float> bufferX(sampleRate);
    std::vector<float> bufferY(sampleRate);

    int fx = freqXSpin->value();
    int fy = freqYSpin->value();
    float phase = phaseStartSlider->value() * M_PI / 180.0f; // Uses Start Phase for now

    float baseFreq = 55.0f;
    float phaseIncX = 2.0f * M_PI * (baseFreq * fx) / sampleRate;
    float phaseIncY = 2.0f * M_PI * (baseFreq * fy) / sampleRate;

    float currentPhaseX = 0.0f;
    float currentPhaseY = 0.0f;

    for (int i = 0; i < sampleRate; ++i) {
        bufferX[i] = std::sin(currentPhaseX);
        bufferY[i] = std::sin(currentPhaseY + phase);
        currentPhaseX += phaseIncX;
        currentPhaseY += phaseIncY;
        if (currentPhaseX >= 2.0f * M_PI) currentPhaseX -= 2.0f * M_PI;
        if (currentPhaseY >= 2.0f * M_PI) currentPhaseY -= 2.0f * M_PI;
    }
    emit timbreGenerated(bufferX, bufferY);
}

void OscilloscopeTab::generateString() {
    QString wave = waveformCombo->currentText();
    int fx = freqXSpin->value();
    int fy = freqYSpin->value();


    float pStart = phaseStartSlider->value() / 360.0f;
    float pEnd = phaseEndSlider->value() / 360.0f;
    float time = timeSpin->value();

    QString exprX, exprY;

    if (pingPongCheck->isChecked()) {
        exprX = QString("%1(integrate(f * %2.0) + (%3 + (%4 - %3) * ((trianglew(t * (tempo / 120.0)) + 1.0) / 2.0)))")
        .arg(wave).arg(fx).arg(pStart).arg(pEnd);
    } else {
        exprX = QString("%1(integrate(f * %2.0) + (%3 + (%4 - %3) * min(t / %5, 1.0)))")
        .arg(wave).arg(fx).arg(pStart).arg(pEnd).arg(time);
    }


    exprY = QString("%1(integrate(f * %2.0))").arg(wave).arg(fy);


    QString finalText = QString("--- XPRESSIVE SETUP NOTES ---\n"
                                "PN1 -> Pan Hard Left (100% L)\n"
                                "PN2 -> Pan Hard Right (100% R)\n"
                                "-----------------------------\n\n"
                                "O1 (X-Axis Left):\n%1\n\n"
                                "O2 (Y-Axis Right):\n%2")
                            .arg(exprX).arg(exprY);

    stringOutput->setText(finalText);
}
