#include "vectorprocessingtab.h"
#include "synthengine.h"
#include "mainwindow.h" // For UniversalScope
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <cmath>

VectorProcessingTab::VectorProcessingTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth)
{
    m_kernel.resize(16, 0.0);
    m_kernel[0] = 1.0;

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

    QHBoxLayout* mainSplit = new QHBoxLayout(this);


    QVBoxLayout* leftPane = new QVBoxLayout();

    m_scope = new UniversalScope();
    m_scope->setMinimumHeight(150);
    leftPane->addWidget(m_scope);

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
    leftPane->addWidget(kernelGroup);

    m_btnNormalize = new QPushButton("Normalise Kernel / Prevent Clipping");
    leftPane->addWidget(m_btnNormalize);

    mainSplit->addLayout(leftPane, 1);


    QVBoxLayout* rightPane = new QVBoxLayout();

    QGroupBox* routingGroup = new QGroupBox("Processor Routing");
    QFormLayout* routingForm = new QFormLayout(routingGroup);

    m_cmbInputSource = new QComboBox();
    m_cmbInputSource->addItems({
        "Sawtooth (Harmonic Rich)",
        "White Noise (Impulse/Exciter)",
        "Square Pulse",
        "Custom (Use Text Boxes Below)"
    });
    m_cmbInputSource->setCurrentIndex(3); // Default to custom

    m_cmbExperiment = new QComboBox();
    m_cmbExperiment->addItems({
        "0. Standard Vector Matrix (Manual Sliders)",
        "1. Morphing Kernel (Crossfades Sliders via LFO)",
        "2. Vector Modal Synthesis (Fixed Resonators)",
        "3. Low-Pass Filter (Moving Average)",
        "4. High-Pass Filter (Difference)",
        "5. Band-Pass Filter (Sinc Curve Approximation)",
        "6. Slapback Echo (Macro Delay)",
        "7. Room Reverb (Exponential Decay)",
        "8. Reverse Reverb (Swell)",
        "9. Metallic Comb Filter / Flanger",
        "10. Multi-Tap Rhythmic Delay (Syncopated)",
        "11. Early Reflections (Reverb Diffuser)",
        "12. Vocal Formant 'Ah' (Complex EQ)",
        "13. Karplus-Strong Resonator (Plucked String)",
        "14. Phase Smear / Lo-Fi Tape",
        "15. Guitar Cabinet IR (4x12 Speaker Sim)",
        "16. Spring Reverb (Boing/Scatter)",
        "17. Wah Pedal (Resonant Bandpass)",
        "18. Dense Chorus / Doubler"
    });

    m_cmbTopology = new QComboBox();
    m_cmbTopology->addItems({
        "IIR Feedback Matrix (Uses last(n) buffer)",
        "FIR Convolution (True Time-Shifting)"
    });

    m_spinTapSpacing = new QSpinBox();
    m_spinTapSpacing->setRange(1, 44100);
    m_spinTapSpacing->setValue(1);
    m_spinTapSpacing->setToolTip("1 = Micro Filters/EQ. 1000+ = Macro Echo/Reverb");

    m_cmbSyntax = new QComboBox();
    m_cmbSyntax->addItems({"Nightly (Variables)", "Legacy (Inline Additive)"});

    m_lblCaution = new QLabel("️ CAUTION: FIR mode duplicates the expression 16 times. This may cause high CPU load or crash LMMS.");
    m_lblCaution->setStyleSheet("color: red; font-weight: bold; padding: 5px; border: 1px solid red; background-color: #330000;");
    m_lblCaution->setVisible(false);
    m_lblCaution->setWordWrap(true);

    routingForm->addRow("Signal Source:", m_cmbInputSource);
    routingForm->addRow("Effect / Experiment:", m_cmbExperiment);
    routingForm->addRow("Architecture:", m_cmbTopology);
    routingForm->addRow("Tap Spacing (Samples):", m_spinTapSpacing);
    routingForm->addRow("Syntax:", m_cmbSyntax);
    routingForm->addRow("", m_lblCaution);
    rightPane->addWidget(routingGroup);


    m_lblIntegrateNote = new QLabel("NOTE: For FIR time-shifting to work, use explicit phase (e.g., 't * f') instead of 'integrate(f)'.");
    m_lblIntegrateNote->setStyleSheet("color: #00ffff; font-style: italic;");
    m_lblIntegrateNote->setWordWrap(true);
    rightPane->addWidget(m_lblIntegrateNote);

    m_lblDspNote1 = new QLabel("This matrix processes the entire waveform. It filters all upper harmonics, not just the fundamental pitch!");
    m_lblDspNote1->setStyleSheet("color: #ffaa00; font-style: italic;");
    m_lblDspNote1->setWordWrap(true);
    rightPane->addWidget(m_lblDspNote1);

    m_lblDspNote2 = new QLabel("Increasing Tap Spacing mathematically lowers the filter frequency and creates repeating reflections (Comb Filtering).");
    m_lblDspNote2->setStyleSheet("color: #ffaa00; font-style: italic;");
    m_lblDspNote2->setWordWrap(true);
    rightPane->addWidget(m_lblDspNote2);

    rightPane->addWidget(new QLabel("Input O1 Custom Expression (Dry):"));
    m_txtInputO1 = new QTextEdit();
    m_txtInputO1->setPlaceholderText("e.g., saww(t * f) * exp(-t * 5)");
    m_txtInputO1->setMaximumHeight(50);
    rightPane->addWidget(m_txtInputO1);

    rightPane->addWidget(new QLabel("Input O2 Custom Expression (Dry):"));
    m_txtInputO2 = new QTextEdit();
    m_txtInputO2->setPlaceholderText("Leave blank if mono...");
    m_txtInputO2->setMaximumHeight(50);
    rightPane->addWidget(m_txtInputO2);


    rightPane->addWidget(new QLabel("Generated O1 Output:"));
    m_txtOutputO1 = new QTextEdit();
    m_txtOutputO1->setReadOnly(true);
    rightPane->addWidget(m_txtOutputO1);

    rightPane->addWidget(new QLabel("Generated O2 Output:"));
    m_txtOutputO2 = new QTextEdit();
    m_txtOutputO2->setReadOnly(true);
    rightPane->addWidget(m_txtOutputO2);


    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_btnGenerate = new QPushButton("🚀 Compile Vector Math");
    m_btnGenerate->setStyleSheet("background-color: #00aa00; border: 1px solid #00ff00; color: white; height: 40px;");

    m_btnPlay = new QPushButton("▶ Preview Audio");
    m_btnPlay->setCheckable(true);
    m_btnPlay->setStyleSheet("height: 40px;");

    btnLayout->addWidget(m_btnPlay);
    btnLayout->addWidget(m_btnGenerate);
    rightPane->addLayout(btnLayout);

    mainSplit->addLayout(rightPane, 1);


    connect(m_cmbExperiment, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VectorProcessingTab::onExperimentChanged);
    connect(m_cmbTopology, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VectorProcessingTab::onTopologyChanged);
    connect(m_spinTapSpacing, QOverload<int>::of(&QSpinBox::valueChanged), this, &VectorProcessingTab::updateUI);
    connect(m_btnGenerate, &QPushButton::clicked, this, &VectorProcessingTab::onGenerate);
    connect(m_btnPlay, &QPushButton::toggled, this, &VectorProcessingTab::togglePlay);
    connect(m_btnNormalize, &QPushButton::clicked, this, &VectorProcessingTab::onNormalizeKernel);
}

void VectorProcessingTab::onTopologyChanged(int index) {
    m_lblCaution->setVisible(index == 1);
}

void VectorProcessingTab::onExperimentChanged(int index) {

    bool custom = (index == 0 || index == 1);


    for(int i=0; i<16; ++i) m_kernelSliders[i]->blockSignals(true);
    m_spinTapSpacing->blockSignals(true);

    for (int i = 0; i < 16; ++i) {
        m_kernelSliders[i]->setEnabled(custom);
    }
    m_btnNormalize->setEnabled(custom);


    if (index == 3) { //
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(100/16);
        m_spinTapSpacing->setValue(1);
    }
    else if (index == 4) {
        m_kernelSliders[0]->setValue(100);
        m_kernelSliders[1]->setValue(-100);
        for(int i=2; i<16; ++i) m_kernelSliders[i]->setValue(0);
        m_spinTapSpacing->setValue(1);
    }
    else if (index == 5) {
        int sinc[] = {0, -10, -20, 30, 100, 30, -20, -10, 0, 0, 0, 0, 0, 0, 0, 0};
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(sinc[i]);
        m_spinTapSpacing->setValue(1);
    }
    else if (index == 6) {
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(0);
        m_kernelSliders[0]->setValue(100);
        m_kernelSliders[15]->setValue(60);
        m_spinTapSpacing->setValue(4000);
    }
    else if (index == 7) {
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(100 * std::pow(0.8, i));
        m_spinTapSpacing->setValue(1500);
    }
    else if (index == 8) {
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(100 * std::pow(0.8, 15 - i));
        m_spinTapSpacing->setValue(1500);
    }
    else if (index == 9) {
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(0);
        m_kernelSliders[0]->setValue(100);
        m_kernelSliders[8]->setValue(80);
        m_spinTapSpacing->setValue(150);
    }
    else if (index == 10) {
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(0);
        m_kernelSliders[0]->setValue(100);
        m_kernelSliders[3]->setValue(75);
        m_kernelSliders[7]->setValue(50);
        m_kernelSliders[11]->setValue(25);
        m_kernelSliders[15]->setValue(10);
        m_spinTapSpacing->setValue(4000);
    }
    else if (index == 11) {
        int diffuse[] = {100, 60, -50, 40, 30, -25, 20, 15, -10, 8, -6, 5, -3, 2, 0, 0};
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(diffuse[i]);
        m_spinTapSpacing->setValue(220);
    }
    else if (index == 12) {
        int formant[] = {0, 20, 50, 80, 100, 80, 20, -40, -80, -100, -80, -40, 20, 50, 80, 100};
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(formant[i]);
        m_spinTapSpacing->setValue(1);
    }
    else if (index == 13) {
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(0);
        m_kernelSliders[0]->setValue(100);
        m_kernelSliders[1]->setValue(98);
        m_spinTapSpacing->setValue(400);
    }
    else if (index == 14) {
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(0);
        m_kernelSliders[0]->setValue(50);
        m_kernelSliders[1]->setValue(-100);
        m_kernelSliders[2]->setValue(50);
        m_spinTapSpacing->setValue(1);
    }
    else if (index == 15) {
        int cab[] = {100, -80, 50, -30, 20, 10, -5, -10, 8, -5, 2, 0, 0, 0, 0, 0};
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(cab[i]);
        m_spinTapSpacing->setValue(2);
    }
    else if (index == 16) {
        int spring[] = {100, -60, 40, -50, 70, -30, 20, 40, -20, 10, -10, 5, -5, 2, 0, 0};
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(spring[i]);
        m_spinTapSpacing->setValue(300);
    }
    else if (index == 17) {
        int wah[] = {10, -30, 100, -30, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(wah[i]);
        m_spinTapSpacing->setValue(5);
    }
    else if (index == 18) {
        for(int i=0; i<16; ++i) m_kernelSliders[i]->setValue(0);
        m_kernelSliders[0]->setValue(100);
        m_kernelSliders[8]->setValue(80);
        m_kernelSliders[15]->setValue(60);
        m_spinTapSpacing->setValue(1200);
    }

    for(int i=0; i<16; ++i) m_kernelSliders[i]->blockSignals(false);
    m_spinTapSpacing->blockSignals(false);
    updateUI();
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
    int inputIdx = m_cmbInputSource->currentIndex();
    int experiment = m_cmbExperiment->currentIndex();
    bool isIIR = (m_cmbTopology->currentIndex() == 0);
    bool isNightly = (m_cmbSyntax->currentIndex() == 0);


    QString baseO1;
    if (inputIdx == 0) baseO1 = "saww(t * f)";
    else if (inputIdx == 1) baseO1 = "randv(t * srate)";
    else if (inputIdx == 2) baseO1 = "squarew(t * f)";
    else {
        baseO1 = m_txtInputO1->toPlainText().trimmed();
        if (baseO1.isEmpty()) baseO1 = "0";
    }


    QString baseO2 = m_txtInputO2->toPlainText().trimmed();


    QString outO1 = generateProcessor(baseO1, experiment, isIIR, isNightly);
    m_txtOutputO1->setText(outO1);
    QApplication::clipboard()->setText(outO1);

    if (inputIdx == 3 && !baseO2.isEmpty()) {
        QString outO2 = generateProcessor(baseO2, experiment, isIIR, isNightly);
        m_txtOutputO2->setText(outO2);
    } else {
        m_txtOutputO2->clear();
    }
}

QString VectorProcessingTab::generateProcessor(QString baseCode, int experiment, bool isIIR, bool isNightly) {


    if (experiment == 2) {
        QString modesMath = "";
        double freqs[6] = {1.0, 2.04, 3.12, 4.5, 5.8, 7.2};
        double decays[6] = {10.0, 15.0, 25.0, 40.0, 50.0, 80.0};
        double amps[6] = {1.0, 0.7, 0.4, 0.2, 0.1, 0.05};

        for(int i = 0; i < 6; ++i) {
            modesMath += QString(" + (%1 * sinew(t * f * %2) * exp(-t * %3))")
            .arg(amps[i]).arg(freqs[i]).arg(decays[i]);
        }

        if (isNightly) {
            return QString("var exciter := (%1) * (t < 0.05);\nvar modes := 0.0%2;\nclamp(-1.0, exciter + modes, 1.0)").arg(baseCode, modesMath);
        }
        return QString("clamp(-1.0, ((%1) * (t < 0.05))%2, 1.0)").arg(baseCode, modesMath);
    }


    QStringList taps;
    QRegularExpression re("\\bt\\b");
    int spacing = m_spinTapSpacing->value();

    for (int i = 0; i < 16; ++i) {
        double valA = m_kernel[i];
        QString tapMultiplier;


        if (experiment == 1) {
            double valB = m_kernel[15 - i];
            if (std::abs(valA) < 0.001 && std::abs(valB) < 0.001) continue;

            if (isNightly) {
                tapMultiplier = QString("((%1 * (1.0 - morph)) + (%2 * morph))").arg(valA, 0, 'f', 3).arg(valB, 0, 'f', 3);
            } else {
                QString inlineMorph = "((sinew(t * 1.5) + 1.0) * 0.5)";
                tapMultiplier = QString("((%1 * (1.0 - %3)) + (%2 * %3))").arg(valA, 0, 'f', 3).arg(valB, 0, 'f', 3).arg(inlineMorph);
            }
        }

        else {
            if (std::abs(valA) < 0.001) continue;
            tapMultiplier = QString::number(valA, 'f', 3);
        }

        int currentDelay = i * spacing;


        if (isIIR) {
            if (i == 0) {
                if (isNightly) taps << QString("(in * %1)").arg(tapMultiplier);
                else taps << QString("((%1) * %2)").arg(baseCode, tapMultiplier);
            } else {
                taps << QString("(last(%1) * %2)").arg(currentDelay).arg(tapMultiplier);
            }
        } else {
            QString shiftedCode = baseCode;
            if (i > 0) {
                QString tShift = QString("max(0, t - (%1/srate))").arg(currentDelay);
                shiftedCode.replace(re, tShift);
            }
            taps << QString("(%1) * %2").arg(shiftedCode, tapMultiplier);
        }
    }

    if (taps.isEmpty()) return "0";
    QString finalMath = taps.join(" + ");


    if (isNightly) {
        QString prefix = QString("var in := %1;\n").arg(baseCode);
        if (experiment == 1) prefix += "var morph := ((sinew(t * 1.5) + 1.0) * 0.5);\n";
        return QString("%1var out := %2;\nclamp(-1.0, out, 1.0)").arg(prefix, finalMath);
    }
    return QString("clamp(-1.0, %1, 1.0)").arg(finalMath);
}

void VectorProcessingTab::togglePlay(bool checked) {
    if (!m_ghostSynth) return;

    if (checked) {
        m_btnPlay->setText("⏹ Stop Preview");
        m_btnPlay->setStyleSheet("background-color: #ff00ff; color: black; height: 40px;");

        int inputIdx = m_cmbInputSource->currentIndex();
        int topology = m_cmbTopology->currentIndex();
        int experiment = m_cmbExperiment->currentIndex();
        int spacing = m_spinTapSpacing->value();

        std::vector<double> currentKernel = m_kernel;


        int bufferSize = (16 * spacing) + 1;

        auto audioAlgo = [=, delayLine = std::vector<double>(bufferSize, 0.0), ptr = 0](double t) mutable -> double {
            double f = 110.0;
            double input = 0.0;

            if (inputIdx == 0 || inputIdx == 3) input = 2.0 * std::fmod(t * f, 1.0) - 1.0;
            else if (inputIdx == 1) input = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            else input = (std::fmod(t * f, 1.0) > 0.5) ? 1.0 : -1.0;

            double output = 0.0;

            if (experiment == 2) {
                double freqs[6] = {1.0, 2.04, 3.12, 4.5, 5.8, 7.2};
                double decays[6] = {10.0, 15.0, 25.0, 40.0, 50.0, 80.0};
                double amps[6] = {1.0, 0.7, 0.4, 0.2, 0.1, 0.05};
                double exciter = (std::fmod(t, 2.0) < 0.05) ? input : 0.0;

                for (int i = 0; i < 6; ++i) {
                    double localTime = std::fmod(t, 2.0);
                    output += amps[i] * std::sin(localTime * f * freqs[i] * 6.28318) * std::exp(-localTime * decays[i]);
                }
                output += exciter;
                return std::clamp(output, -1.0, 1.0);
            }


            if (topology == 0) {
                output = input;
                double morph = (std::sin(t * 1.5 * 6.28318) + 1.0) * 0.5;

                for (int i = 0; i < 16; ++i) {
                    double h = currentKernel[i];
                    if (experiment == 1) {
                        h = (currentKernel[i] * (1.0 - morph)) + (currentKernel[15 - i] * morph);
                    }

                    if (i == 0) output = input * h;
                    else {
                        int currentDelay = i * spacing;
                        int readPtr = (ptr - currentDelay + bufferSize) % bufferSize;
                        output += h * delayLine[readPtr];
                    }
                }
                output = std::clamp(output, -1.0, 1.0);
                delayLine[ptr] = output;
                ptr = (ptr + 1) % bufferSize;
            }
            else if (topology == 1) {
                delayLine[ptr] = input;
                output = 0.0;
                double morph = (std::sin(t * 1.5 * 6.28318) + 1.0) * 0.5;

                for (int i = 0; i < 16; ++i) {
                    double h = currentKernel[i];
                    if (experiment == 1) {
                        h = (currentKernel[i] * (1.0 - morph)) + (currentKernel[15 - i] * morph);
                    }
                    int currentDelay = i * spacing;
                    int readPtr = (ptr - currentDelay + bufferSize) % bufferSize;
                    output += h * delayLine[readPtr];
                }
                output = std::clamp(output, -1.0, 1.0);
                ptr = (ptr + 1) % bufferSize;
            }

            return output;
        };


        double scopeView = (spacing > 1000) ? 0.5 : 0.05;
        m_scope->updateScope(audioAlgo, scopeView, 1.0);

        m_ghostSynth->setAudioSource(audioAlgo);
        m_ghostSynth->start();

    } else {
        m_btnPlay->setText("▶ Preview Audio");
        m_btnPlay->setStyleSheet("height: 40px;");
        m_ghostSynth->stop();
    }
}