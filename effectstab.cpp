#include "effectstab.h"
#include <math.h>

EffectsTab::EffectsTab(QWidget *parent) : QWidget(parent) {
    setupUI();

    connect(effectSelector, &QComboBox::currentIndexChanged, this, &EffectsTab::updateLabels);
    connect(param1Slider, &QSlider::valueChanged, this, &EffectsTab::updateLabels);
    connect(param2Slider, &QSlider::valueChanged, this, &EffectsTab::updateLabels);
    connect(param3Slider, &QSlider::valueChanged, this, &EffectsTab::updateLabels);
    connect(generateButton, &QPushButton::clicked, this, &EffectsTab::generateExpression);

    updateLabels();
}

void EffectsTab::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel("Paste Base Expression Here:"));
    inputExpression = new QTextEdit(this);
    inputExpression->setMaximumHeight(60);
    mainLayout->addWidget(inputExpression);

    QHBoxLayout *controlsLayout = new QHBoxLayout();
    effectSelector = new QComboBox(this);
    effectSelector->addItem("Single-Tap Delay");
    effectSelector->addItem("Recursive Echo (Pseudo-Reverb)");
    effectSelector->addItem("Chorus (Modulated Delay)");
    effectSelector->addItem("Saturation (Soft Clip / Tanh)");
    effectSelector->addItem("Hard Clipping (Overdrive)");
    effectSelector->addItem("Bitcrusher (Quantization)");
    effectSelector->addItem("Low-Pass Filter (1-Pole IIR)");
    effectSelector->addItem("Tremolo (AM)");

    controlsLayout->addWidget(new QLabel("Effect:"));
    controlsLayout->addWidget(effectSelector);

    nightlyCheckBox = new QCheckBox("Nightly Build (ExprTk)", this);
    nightlyCheckBox->setChecked(true);
    controlsLayout->addWidget(nightlyCheckBox);
    mainLayout->addLayout(controlsLayout);


    QHBoxLayout *p1Layout = new QHBoxLayout();
    param1Label = new QLabel("Param 1:", this);
    param1Slider = new QSlider(Qt::Horizontal, this);
    param1Slider->setRange(1, 100);
    p1Layout->addWidget(param1Label);
    p1Layout->addWidget(param1Slider);
    mainLayout->addLayout(p1Layout);


    QHBoxLayout *p2Layout = new QHBoxLayout();
    param2Label = new QLabel("Param 2:", this);
    param2Slider = new QSlider(Qt::Horizontal, this);
    param2Slider->setRange(0, 100);
    p2Layout->addWidget(param2Label);
    p2Layout->addWidget(param2Slider);
    mainLayout->addLayout(p2Layout);


    QHBoxLayout *p3Layout = new QHBoxLayout();
    param3Label = new QLabel("Param 3:", this);
    param3Slider = new QSlider(Qt::Horizontal, this);
    param3Slider->setRange(0, 100);
    p3Layout->addWidget(param3Label);
    p3Layout->addWidget(param3Slider);
    mainLayout->addLayout(p3Layout);

    generateButton = new QPushButton("Generate Effect Expression", this);
    mainLayout->addWidget(generateButton);

    mainLayout->addWidget(new QLabel("Output Expression:"));
    outputExpression = new QTextEdit(this);
    outputExpression->setReadOnly(true);
    mainLayout->addWidget(outputExpression);
}

void EffectsTab::updateLabels() {
    QString effect = effectSelector->currentText();

    param1Slider->setEnabled(true);
    param2Slider->setEnabled(true);
    param3Slider->setEnabled(true);

    if (effect == "Single-Tap Delay") {
        param1Label->setText(QString("Delay Time (%1 ms):").arg(param1Slider->value() * 10));
        param2Label->setText(QString("Wet Mix (%1%):").arg(param2Slider->value()));
        param3Label->setText("N/A"); param3Slider->setEnabled(false);
    }
    else if (effect == "Recursive Echo (Pseudo-Reverb)") {
        param1Label->setText(QString("Room Size / Time (%1 ms):").arg(param1Slider->value() * 5));
        param2Label->setText(QString("Feedback Decay (%1%):").arg(param2Slider->value()));
        param3Label->setText("N/A"); param3Slider->setEnabled(false);
    }
    else if (effect == "Chorus (Modulated Delay)") {
        param1Label->setText(QString("Rate (%1 Hz):").arg(param1Slider->value() / 10.0));
        param2Label->setText(QString("Depth (%1%):").arg(param2Slider->value()));
        param3Label->setText(QString("Wet Mix (%1%):").arg(param3Slider->value()));
    }
    else if (effect == "Saturation (Soft Clip / Tanh)") {
        param1Label->setText(QString("Drive (x%1):").arg(param1Slider->value() / 5.0));
        param2Label->setText("N/A"); param2Slider->setEnabled(false);
        param3Label->setText("N/A"); param3Slider->setEnabled(false);
    }
    else if (effect == "Hard Clipping (Overdrive)") {
        param1Label->setText(QString("Gain Multiplier (x%1):").arg(param1Slider->value()));
        param2Label->setText("N/A"); param2Slider->setEnabled(false);
        param3Label->setText("N/A"); param3Slider->setEnabled(false);
    }
    else if (effect == "Bitcrusher (Quantization)") {
        int bits = 1 + (param1Slider->value() / 7);
        param1Label->setText(QString("Resolution (%1-bit):").arg(bits));
        param2Label->setText("N/A"); param2Slider->setEnabled(false);
        param3Label->setText("N/A"); param3Slider->setEnabled(false);
    }
    else if (effect == "Low-Pass Filter (1-Pole IIR)") {
        param1Label->setText(QString("Cutoff Muffle (%1%):").arg(param1Slider->value()));
        param2Label->setText("N/A"); param2Slider->setEnabled(false);
        param3Label->setText("N/A"); param3Slider->setEnabled(false);
    }
    else if (effect == "Tremolo (AM)") {
        param1Label->setText(QString("Rate (%1 Hz):").arg(param1Slider->value() / 5.0));
        param2Label->setText(QString("Depth (%1%):").arg(param2Slider->value()));
        param3Label->setText("N/A"); param3Slider->setEnabled(false);
    }
}

void EffectsTab::generateExpression() {
    QString baseExpr = inputExpression->toPlainText().trimmed();
    if (baseExpr.isEmpty()) {
        outputExpression->setText("Please enter a base expression first.");
        return;
    }

    baseExpr = QString("(%1)").arg(baseExpr);
    QString result = "";
    QString effect = effectSelector->currentText();

    if (effect == "Single-Tap Delay") {
        int ms = param1Slider->value() * 10;
        int sampleOffset = ms * 44.1;
        double wetMix = param2Slider->value() / 100.0;
        double initTime = ms / 1000.0;
        result = QString("%1 + %2 * last(%3) * (t > %4)").arg(baseExpr).arg(wetMix).arg(sampleOffset).arg(initTime);
    }
    else if (effect == "Recursive Echo (Pseudo-Reverb)") {
        int ms = param1Slider->value() * 5; // 5ms to 500ms
        int sampleOffset = ms * 44.1;
        double feedback = (param2Slider->value() / 100.0) * 0.95;
        double initTime = ms / 1000.0;
        result = QString("%1 + %2 * last(%3) * (t > %4)").arg(baseExpr).arg(feedback).arg(sampleOffset).arg(initTime);
    }
    else if (effect == "Chorus (Modulated Delay)") {
        double rate = param1Slider->value() / 10.0;
        double depth = param2Slider->value() / 100.0;
        double wetMix = param3Slider->value() / 100.0;
        int baseDelay = 882; // ~20ms base delay for chorus
        int modMagnitude = 441 * depth; // Up to 10ms swing
        result = QString("%1 + %2 * last(%3 + %4 * sin(t * 6.28318 * %5)) * (t > 0.05)")
                     .arg(baseExpr).arg(wetMix).arg(baseDelay).arg(modMagnitude).arg(rate);
    }
    else if (effect == "Saturation (Soft Clip / Tanh)") {
        double drive = param1Slider->value() / 5.0; // 0.2 to 20.0
        result = QString("tanh(%1 * %2)").arg(baseExpr).arg(drive);
    }
    else if (effect == "Hard Clipping (Overdrive)") {
        int gain = param1Slider->value();
        result = QString("clamp(-1.0, %1 * %2, 1.0)").arg(baseExpr).arg(gain);
    }
    else if (effect == "Bitcrusher (Quantization)") {
        int bits = 1 + (param1Slider->value() / 7);
        long states = pow(2, bits);
        result = QString("(floor(%1 * %2) / %2)").arg(baseExpr).arg(states);
    }
    else if (effect == "Low-Pass Filter (1-Pole IIR)") {
        double alpha = param1Slider->value() / 100.0;
        if (alpha > 0.99) alpha = 0.99; // Prevent absolute silence
        double currentWeight = 1.0 - alpha;
        result = QString("(%1 * %2) + (%3 * last(1))").arg(baseExpr).arg(currentWeight).arg(alpha);
    }
    else if (effect == "Tremolo (AM)") {
        double rate = param1Slider->value() / 5.0;
        double depth = param2Slider->value() / 100.0;
        result = QString("%1 * (1.0 - %2 + %2 * 0.5 * (1.0 + sin(t * 6.28318 * %3)))")
                     .arg(baseExpr).arg(depth).arg(rate);
    }

    outputExpression->setText(result);
}
