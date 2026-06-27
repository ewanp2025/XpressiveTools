#include "vectorprocessingtab.h"
#include "synthengine.h"
#include "mainwindow.h" // For UniversalScope
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <cmath>

VectorProcessingTab::VectorProcessingTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth)
{
    m_kernel.resize(16, 0.0);
    m_kernel[0] = 1.0; // Default to pass-through

    setupUI();
    updateUI();
}

VectorProcessingTab::~VectorProcessingTab() {}

void VectorProcessingTab::setupUI() {
    this->setStyleSheet(R"(
        QWidget { background-color: #0d0d12; color: #ff00ff; font-family: "Consolas", monospace; }
        QGroupBox { border: 1px solid #aa00ff; margin-top: 10px; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; color: #ff55ff; }
        QPushButton { background-color: #330055; border: 1px solid #ff00ff; padding: 8px; font-weight: bold; }
        QPushButton:hover { background-color: #550088; color: #ffffff; }
        QPushButton:checked { background-color: #ff00ff; color: #000000; }
        QSlider::groove:vertical { border: 1px solid #aa00ff; width: 6px; background: #110022; }
        QSlider::handle:vertical { background: #ff00ff; height: 14px; margin: 0 -4px; border-radius: 7px; }
        QTextEdit { background-color: #050508; border: 1px solid #aa00ff; color: #ffaa00; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);


    m_scope = new UniversalScope();
    m_scope->setMinimumHeight(150);
    mainLayout->addWidget(m_scope);


    QHBoxLayout* controlsLayout = new QHBoxLayout();

    QGroupBox* expGroup = new QGroupBox("Unrolled Vector Engine");
    QFormLayout* formLayout = new QFormLayout(expGroup);

    m_cmbExperiment = new QComboBox();
    m_cmbExperiment->addItems({
        "16-Tap IIR Resonator (Feedback Delay Matrix)",
        "Vector Modal Synthesis (Additive)",
        "Morphing IIR Matrix (Crossfading Kernels)"
    });

    m_cmbInputSource = new QComboBox();
    m_cmbInputSource->addItems({"Sawtooth (Harmonic Rich)", "White Noise (Impulse/Exciter)", "Square Pulse"});

    formLayout->addRow("Experiment Type:", m_cmbExperiment);
    formLayout->addRow("Input Signal:", m_cmbInputSource);

    m_btnNormalize = new QPushButton("Normalize Kernel");
    formLayout->addRow("", m_btnNormalize);

    controlsLayout->addWidget(expGroup, 1);


    QGroupBox* kernelGroup = new QGroupBox("16-Point Impulse Response (Kernel)");
    QHBoxLayout* slidersLayout = new QHBoxLayout(kernelGroup);

    for (int i = 0; i < 16; ++i) {
        QVBoxLayout* sl = new QVBoxLayout();
        m_kernelSliders[i] = new QSlider(Qt::Vertical);
        m_kernelSliders[i]->setRange(-100, 100);
        m_kernelSliders[i]->setValue(i == 0 ? 100 : 0);

        m_kernelLabels[i] = new QLabel(QString("h[%1]").arg(i));
        m_kernelLabels[i]->setAlignment(Qt::AlignCenter);

        sl->addWidget(m_kernelSliders[i]);
        sl->addWidget(m_kernelLabels[i]);
        slidersLayout->addLayout(sl);

        connect(m_kernelSliders[i], &QSlider::valueChanged, this, &VectorProcessingTab::updateUI);
    }
    controlsLayout->addWidget(kernelGroup, 2);
    mainLayout->addLayout(controlsLayout);


    QHBoxLayout* outRow = new QHBoxLayout();
    m_btnGenerate = new QPushButton("🚀 Compile Unrolled Math");
    m_btnGenerate->setStyleSheet("background-color: #00aa00; border: 1px solid #00ff00; color: white;");

    m_btnPlay = new QPushButton("▶ Test Vector Matrix");
    m_btnPlay->setCheckable(true);

    outRow->addWidget(m_btnGenerate);
    outRow->addWidget(m_btnPlay);
    mainLayout->addLayout(outRow);

    m_txtOutput = new QTextEdit();
    m_txtOutput->setReadOnly(true);
    m_txtOutput->setMinimumHeight(120);
    mainLayout->addWidget(m_txtOutput);


    connect(m_cmbExperiment, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VectorProcessingTab::onExperimentChanged);
    connect(m_btnGenerate, &QPushButton::clicked, this, &VectorProcessingTab::onGenerate);
    connect(m_btnPlay, &QPushButton::toggled, this, &VectorProcessingTab::togglePlay);
    connect(m_btnNormalize, &QPushButton::clicked, this, &VectorProcessingTab::onNormalizeKernel);
}

void VectorProcessingTab::onExperimentChanged(int index) {
    bool enableSliders = (index == 0 || index == 2);
    for (int i = 0; i < 16; ++i) {
        m_kernelSliders[i]->setEnabled(enableSliders);
    }
    m_btnNormalize->setEnabled(enableSliders);
}

void VectorProcessingTab::onNormalizeKernel() {
    double sum = 0;
    for (int i = 0; i < 16; ++i) sum += std::abs(m_kernelSliders[i]->value());
    if (sum == 0) return;

    for (int i = 0; i < 16; ++i) {
        double val = (m_kernelSliders[i]->value() / sum) * 100.0;
        m_kernelSliders[i]->setValue(static_cast<int>(val));
    }
    updateUI();
}

void VectorProcessingTab::updateUI() {
    for (int i = 0; i < 16; ++i) {
        m_kernel[i] = m_kernelSliders[i]->value() / 100.0;
    }
    if (m_btnPlay->isChecked()) togglePlay(true);
}

void VectorProcessingTab::onGenerate() {
    int experiment = m_cmbExperiment->currentIndex();
    QString code;

    if (experiment == 0) code = generateFIRConvolution();
    else if (experiment == 1) code = generateModalSynthesis();
    else if (experiment == 2) code = generateMorphingKernel();

    m_txtOutput->setText(code);
    QApplication::clipboard()->setText(code);
}

QString VectorProcessingTab::generateFIRConvolution() {
    QString inputSig = (m_cmbInputSource->currentIndex() == 0) ? "saww(integrate(f))" :
                           (m_cmbInputSource->currentIndex() == 1) ? "randv(t*srate)" : "squarew(integrate(f))";

    QString convMath = "";
    for (int i = 0; i < 16; ++i) {
        if (std::abs(m_kernel[i]) > 0.001) {
            convMath += QString(" + (%1 * last(%2))").arg(m_kernel[i], 0, 'f', 3).arg(i + 1);
        }
    }

    return QString(
               "//remember to delete comments when pasting as lmms does not like comments\n"
               "var input := %1;\n"
               "var out := input%2;\n"
               "clamp(-1.0, out, 1.0)" // Notice: No trailing semicolon here!
               ).arg(inputSig, convMath);
}

QString VectorProcessingTab::generateModalSynthesis() {
    // Treat the input source as a percussive exciter by multiplying it by a short decay envelope
    QString exciterSig = (m_cmbInputSource->currentIndex() == 0) ? "saww(integrate(f))" :
                             (m_cmbInputSource->currentIndex() == 1) ? "randv(t*srate)" : "squarew(integrate(f))";

    double freqs[6] = {1.0, 2.04, 3.12, 4.5, 5.8, 7.2};
    double decays[6] = {10.0, 15.0, 25.0, 40.0, 50.0, 80.0};
    double amps[6] = {1.0, 0.7, 0.4, 0.2, 0.1, 0.05};

    QString modesMath = "";
    for(int i = 0; i < 6; ++i) {
        modesMath += QString(" + (%1 * sinew(integrate(f * %2)) * exp(-t * %3))")
        .arg(amps[i]).arg(freqs[i]).arg(decays[i]);
    }

    return QString(
               "//remember to delete comments when pasting as lmms does not like comments\n"
               "var exciter := %1 * (t < 0.05);\n"
               "var modes := 0.0%2;\n"
               "clamp(-1.0, exciter + modes, 1.0)"
               ).arg(exciterSig, modesMath);
}

QString VectorProcessingTab::generateMorphingKernel() {
    QString inputSig = (m_cmbInputSource->currentIndex() == 0) ? "saww(integrate(f))" :
                           (m_cmbInputSource->currentIndex() == 1) ? "randv(t*srate)" : "squarew(integrate(f))";

    QString convMath = "";
    for (int i = 0; i < 16; ++i) {
        double valA = m_kernel[i];
        double valB = m_kernel[15 - i];

        if (std::abs(valA) > 0.001 || std::abs(valB) > 0.001) {
            convMath += QString(" + (((%1 * (1.0 - morph)) + (%2 * morph)) * last(%3))")
            .arg(valA, 0, 'f', 3).arg(valB, 0, 'f', 3).arg(i + 1);
        }
    }

    return QString(
               "//remember to delete comments when pasting as lmms does not like comments\n"
               "var input := %1;\n"
               "var morph := (sinew(t * 1.5) + 1.0) * 0.5;\n"
               "var out := input%2;\n"
               "clamp(-1.0, out, 1.0)"
               ).arg(inputSig, convMath);
}

void VectorProcessingTab::togglePlay(bool checked) {
    if (!m_ghostSynth) return;

    if (checked) {
        m_btnPlay->setText("⏹ Stop");
        m_btnPlay->setStyleSheet("background-color: #ff00ff; color: black;");

        int inputType = m_cmbInputSource->currentIndex();
        int expType = m_cmbExperiment->currentIndex();
        std::vector<double> currentKernel = m_kernel;

        auto audioAlgo = [=, delayLine = std::vector<double>(16, 0.0), ptr = 0](double t) mutable -> double {
            double input = 0.0;
            double f = 110.0;

            if (inputType == 0) input = 2.0 * std::fmod(t * f, 1.0) - 1.0;
            else if (inputType == 1) input = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            else input = (std::fmod(t * f, 1.0) > 0.5) ? 1.0 : -1.0;

            double output = 0.0;

            if (expType == 0) {
                output = input;
                for (int i = 0; i < 16; ++i) {
                    int readPtr = (ptr - i - 1 + 16) % 16;
                    output += currentKernel[i] * delayLine[readPtr];
                }
                output = std::clamp(output, -1.0, 1.0);
                delayLine[ptr] = output;
                ptr = (ptr + 1) % 16;
            }
            else if (expType == 1) {
                double freqs[6] = {1.0, 2.04, 3.12, 4.5, 5.8, 7.2};
                double decays[6] = {10.0, 15.0, 25.0, 40.0, 50.0, 80.0};
                double amps[6] = {1.0, 0.7, 0.4, 0.2, 0.1, 0.05};
                double exciter = (std::fmod(t, 2.0) < 0.05) ? input : 0.0;

                for (int i = 0; i < 6; ++i) {
                    double localTime = std::fmod(t, 2.0);
                    output += amps[i] * std::sin(localTime * f * freqs[i] * 6.28318) * std::exp(-localTime * decays[i]);
                }
                output += exciter;
                output = std::clamp(output, -1.0, 1.0);
            }
            else if (expType == 2) {
                output = input;
                double morph = (std::sin(t * 1.5 * 6.28318) + 1.0) * 0.5;
                for (int i = 0; i < 16; ++i) {
                    double h = (currentKernel[i] * (1.0 - morph)) + (currentKernel[15 - i] * morph);
                    int readPtr = (ptr - i - 1 + 16) % 16;
                    output += h * delayLine[readPtr];
                }
                output = std::clamp(output, -1.0, 1.0);
                delayLine[ptr] = output;
                ptr = (ptr + 1) % 16;
            }

            return output;
        };

        m_scope->updateScope(audioAlgo, 0.05, 1.0);
        m_ghostSynth->setAudioSource(audioAlgo);
        m_ghostSynth->start();

    } else {
        m_btnPlay->setText("▶ Test Vector Matrix");
        m_btnPlay->setStyleSheet("");
        m_ghostSynth->stop();
    }
}