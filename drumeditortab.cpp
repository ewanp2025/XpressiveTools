#include "drumeditortab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QRadioButton>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QTimer>
#include <cmath>
#include <algorithm>
#include <QStackedWidget>
#include "mainwindow.h"
#include "synthengine.h" 

DrumEditorTab::DrumEditorTab(SynthEngine* synthEngine, QWidget *parent) 
    : QWidget(parent), m_ghostSynth(synthEngine) 
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* modeLayout = new QHBoxLayout();
    m_radioManual = new QRadioButton("Manual Drum Designer", this);
    m_radioAnalyze = new QRadioButton("Analyze Sample Mode", this);
    m_radioManual->setChecked(true); // Default to your manual designer
    modeLayout->addWidget(m_radioManual);
    modeLayout->addWidget(m_radioAnalyze);
    mainLayout->addLayout(modeLayout);

    connect(m_radioManual, &QRadioButton::toggled, this, &DrumEditorTab::onModeSwitched);
    connect(m_radioAnalyze, &QRadioButton::toggled, this, &DrumEditorTab::onModeSwitched);


    m_manualContainer = new QWidget(this);
    QVBoxLayout* drumLayout = new QVBoxLayout(m_manualContainer);

    drumScope = new UniversalScope();
    drumScope->setMinimumHeight(150);
    drumLayout->addWidget(drumScope);

    drumDisclaimer = new QLabel("⚠️ NOTICE: Panning must be set manually. Filters are simulated math approximations.");
    drumDisclaimer->setStyleSheet("color: red; font-weight: bold; border: 1px solid red; padding: 5px; background-color: #ffeeee;");
    drumDisclaimer->setAlignment(Qt::AlignCenter);
    drumLayout->addWidget(drumDisclaimer);

    drumTypeCombo = new QComboBox();
    drumTypeCombo->addItems({"Kick (LPF)", "Snare (BPF)", "Hi-Hat (HPF)", "Tom (LPF)", "Cowbell (BPF)", "Rimshot (HPF)", "Clap (BPF)"});

    drumWaveCombo = new QComboBox();
    drumWaveCombo->addItems({"Sine", "Triangle", "Square", "Sawtooth"});

    QFormLayout *fLayout = new QFormLayout();

    // Initialise Sliders
    drumPitchSlider = new QSlider(Qt::Horizontal); drumPitchSlider->setRange(20, 150); drumPitchSlider->setValue(40);
    drumDecaySlider = new QSlider(Qt::Horizontal); drumDecaySlider->setRange(1, 200); drumDecaySlider->setValue(40);
    drumPitchDropSlider = new QSlider(Qt::Horizontal); drumPitchDropSlider->setRange(0, 500); drumPitchDropSlider->setValue(350);
    drumToneSlider = new QSlider(Qt::Horizontal);  drumToneSlider->setRange(100, 14000); drumToneSlider->setValue(1000);
    drumSnapSlider = new QSlider(Qt::Horizontal);  drumSnapSlider->setRange(10, 100); drumSnapSlider->setValue(50);
    drumNoiseSlider = new QSlider(Qt::Horizontal); drumNoiseSlider->setRange(0, 100); drumNoiseSlider->setValue(0);
    drumPWMSlider = new QSlider(Qt::Horizontal);   drumPWMSlider->setRange(0, 100); drumPWMSlider->setValue(50);
    drumExpSlider = new QSlider(Qt::Horizontal);   drumExpSlider->setRange(1, 10); drumExpSlider->setValue(2);

    fLayout->addRow("Body Waveform:", drumWaveCombo);
    fLayout->addRow("Base Pitch:", drumPitchSlider);
    fLayout->addRow("Decay Speed:", drumDecaySlider);
    fLayout->addRow("Exponential Curve:", drumExpSlider);
    fLayout->addRow("Pitch Punch (Drop):", drumPitchDropSlider);
    fLayout->addRow("Filter Cutoff (Sim):", drumToneSlider);
    fLayout->addRow("Filter Res (Snap):", drumSnapSlider);
    fLayout->addRow("Noise Mix:", drumNoiseSlider);
    fLayout->addRow("Pulse Width:", drumPWMSlider);

    drumLayout->addWidget(new QLabel("<b>Internal Filter Drum Designer</b>"));
    drumLayout->addLayout(fLayout);
    drumLayout->addWidget(drumTypeCombo);

    auto *dBtnLay = new QHBoxLayout();
    btnPlayDrum = new QPushButton("▶ Play Drum Loop");
    btnPlayDrum->setCheckable(true);
    btnPlayDrum->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");

    btnGenerateDrum = new QPushButton("Copy Formula to Clipboard");
    btnSaveDrumXpf = new QPushButton("Save Drum as .XPF File");

    dBtnLay->addWidget(btnPlayDrum);
    dBtnLay->addWidget(btnGenerateDrum);
    dBtnLay->addWidget(btnSaveDrumXpf);
    drumLayout->addLayout(dBtnLay);

    m_manualOutputBox = new QTextEdit(this);
    m_manualOutputBox->setPlaceholderText("Generated XPF code will appear here...");
    m_manualOutputBox->setMaximumHeight(100);
    drumLayout->addWidget(m_manualOutputBox);

    m_analyzeContainer = new QWidget(this);
    QVBoxLayout* analyzeLayout = new QVBoxLayout(m_analyzeContainer);

    m_btnLoadSample = new QPushButton("Load Drum Sample (.wav)...", this);
    m_analyzeScope = new UniversalScope(this);
    m_analyzeScope->setMinimumHeight(150);
    m_lblAnalysisResults = new QLabel("Analysis: Waiting for sample...", this);
    m_lblFilterRecommendation = new QLabel("Filter: N/A", this);
    m_outputExpression = new QTextEdit(this);
    m_outputExpression->setPlaceholderText("Generated mathematical expression will appear here...");

    m_lblTrimDetails = new QLabel("Trim: 0.0% to 100.0%", this);

    QHBoxLayout* trimLayout = new QHBoxLayout();
    m_trimStartSlider = new QSlider(Qt::Horizontal, this);
    m_trimStartSlider->setRange(0, 1000); // 1000 steps for 0.1% precision
    m_trimStartSlider->setValue(0);

    m_trimEndSlider = new QSlider(Qt::Horizontal, this);
    m_trimEndSlider->setRange(0, 1000);
    m_trimEndSlider->setValue(1000);

    trimLayout->addWidget(new QLabel("Start:"));
    trimLayout->addWidget(m_trimStartSlider);
    trimLayout->addWidget(new QLabel("End:"));
    trimLayout->addWidget(m_trimEndSlider);

    analyzeLayout->addWidget(m_btnLoadSample);
    analyzeLayout->addWidget(m_analyzeScope);
    analyzeLayout->addWidget(m_lblTrimDetails);
    analyzeLayout->addLayout(trimLayout);
    analyzeLayout->addWidget(m_lblAnalysisResults);
    analyzeLayout->addWidget(m_lblFilterRecommendation);
    analyzeLayout->addWidget(m_outputExpression);

    connect(m_trimStartSlider, &QSlider::valueChanged, this, &DrumEditorTab::onTrimChanged);
    connect(m_trimEndSlider, &QSlider::valueChanged, this, &DrumEditorTab::onTrimChanged);

    m_modeStack = new QStackedWidget(this);
    m_modeStack->addWidget(m_manualContainer);   // Index 0
    m_modeStack->addWidget(m_analyzeContainer);  // Index 1

    mainLayout->addWidget(m_modeStack);

    connect(drumPitchSlider, &QSlider::valueChanged, this, &DrumEditorTab::updateDrum);
    connect(drumDecaySlider, &QSlider::valueChanged, this, &DrumEditorTab::updateDrum);
    connect(drumPitchDropSlider, &QSlider::valueChanged, this, &DrumEditorTab::updateDrum);
    connect(drumExpSlider, &QSlider::valueChanged, this, &DrumEditorTab::updateDrum);
    connect(drumNoiseSlider, &QSlider::valueChanged, this, &DrumEditorTab::updateDrum);
    connect(drumWaveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DrumEditorTab::updateDrum);

    connect(btnGenerateDrum, &QPushButton::clicked, this, &DrumEditorTab::generateDrumXpf);
    connect(btnSaveDrumXpf, &QPushButton::clicked, this, &DrumEditorTab::generateDrumXpf);
    connect(m_btnLoadSample, &QPushButton::clicked, this, &DrumEditorTab::onLoadSample);

    connect(drumTypeCombo, &QComboBox::currentIndexChanged, [=](int idx){
        bool wasPlaying = btnPlayDrum->isChecked();
        if(wasPlaying && m_ghostSynth) m_ghostSynth->stop();

        drumPitchSlider->blockSignals(true);
        drumDecaySlider->blockSignals(true);



        drumPitchSlider->blockSignals(false);
        drumDecaySlider->blockSignals(false);

        updateDrum();
        if(wasPlaying && m_ghostSynth) m_ghostSynth->start();
    });


    connect(btnPlayDrum, &QPushButton::toggled, [=](bool checked){
        if(!checked) {
            if(m_ghostSynth) {
                m_ghostSynth->setAudioSource([](double){ return 0.0; });
                m_ghostSynth->stop();
            }
            btnPlayDrum->setText("▶ Play Drum Loop");
            btnPlayDrum->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 40px;");
        } else {
            if(m_ghostSynth) m_ghostSynth->start();
            btnPlayDrum->setText("⏹ Stop");
            btnPlayDrum->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 40px;");
            updateDrum();
        }
    });

    QTimer::singleShot(200, this, &DrumEditorTab::updateDrum);
}

void DrumEditorTab::onModeSwitched() {
    if (m_radioManual->isChecked()) {
        m_modeStack->setCurrentWidget(m_manualContainer);
    } else {
        m_modeStack->setCurrentWidget(m_analyzeContainer);


        if (btnPlayDrum->isChecked()) {
            btnPlayDrum->setChecked(false);
        }
    }
}

void DrumEditorTab::updateDrum() {
    int waveIdx = drumWaveCombo->currentIndex();
    double baseFreq = drumPitchSlider->value();
    double decayFactor = drumDecaySlider->value();
    double pitchDrop = drumPitchDropSlider->value();
    double noiseMix = drumNoiseSlider->value() / 100.0;
    double expCurve = drumExpSlider->value();

    double loopLen = 0.5 + (200.0 / (decayFactor > 0 ? decayFactor : 1.0));

    std::function<double(double)> drumAlgo = [=](double t) {
        double localT = std::fmod(t, loopLen);
        if(localT < 0) return 0.0;

        double instFreq = baseFreq + (pitchDrop * std::exp(-localT * (decayFactor / 2.0)));
        double phase = localT * instFreq * 6.283185;
        double osc = 0.0;

        if (waveIdx == 0) osc = std::sin(phase);
        else if (waveIdx == 1) osc = (2.0/3.14159) * std::asin(std::sin(phase));
        else if (waveIdx == 2) osc = (std::sin(phase) > 0 ? 1.0 : -1.0);
        else osc = 2.0 * (std::fmod(localT * instFreq, 1.0)) - 1.0;

        double noise = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        double signal = (osc * (1.0 - noiseMix)) + (noise * noiseMix);
        double env = std::exp(-localT * decayFactor * expCurve);

        return signal * env;
    };

    drumScope->updateScope(drumAlgo, 0.2, 1.0);

    if (btnPlayDrum->isChecked() && m_ghostSynth) {
        m_ghostSynth->setAudioSource(drumAlgo);
    }


    QString waveFunc = drumWaveCombo->currentText().toLower() + "w";
    if(waveFunc == "sinew") waveFunc = "sinew";

    QString pitch = QString("(f + (%1 * exp(-t * %2)))").arg(pitchDrop).arg(decayFactor / 2.0);
    QString source = QString("((%1(integrate(%2)) * %3) + (randv(t*10000) * %4))")
                         .arg(waveFunc).arg(pitch).arg(1.0 - noiseMix).arg(noiseMix);

    QString formula = QString("(%1 * exp(-t * %2))").arg(source).arg(decayFactor * expCurve);
    QString displayFormula = QString("clamp(-1, %1, 1)").arg(formula);


    m_manualOutputBox->setText(displayFormula);
}
void DrumEditorTab::generateDrumXpf() {

    QString waveFunc = drumWaveCombo->currentText().toLower() + "w";
    if(waveFunc == "sinew") waveFunc = "sinew";

    double decayBase = drumDecaySlider->value();
    double expFactor = drumExpSlider->value();
    double pitchDrop = drumPitchDropSlider->value();
    double noiseMix = drumNoiseSlider->value() / 100.0;

    QString pitch = QString("(f + (%1 * exp(-t * %2)))").arg(pitchDrop).arg(decayBase / 2.0);
    QString source = QString("((%1(integrate(%2)) * %3) + (randv(t*10000) * %4))")
                         .arg(waveFunc).arg(pitch).arg(1.0 - noiseMix).arg(noiseMix);

    QString formula = QString("(%1 * exp(-t * %2))").arg(source).arg(decayBase * expFactor);
    QString displayFormula = QString("clamp(-1, %1, 1)").arg(formula);


    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());

    if (clickedButton == btnSaveDrumXpf) {

        QString xmlFormula = formula;
        xmlFormula = xmlFormula.replace("\"", "&quot;");

        int typeIndex = drumTypeCombo->currentIndex();
        int filterType = 0;
        if (typeIndex == 1 || typeIndex == 4 || typeIndex == 6) filterType = 2;
        if (typeIndex == 2 || typeIndex == 5) filterType = 1;

        QString xpf = getXpfTemplate().arg(
            drumTypeCombo->currentText(),
            QString::number(drumPitchSlider->value()),
            xmlFormula,
            QString::number(drumToneSlider->value()),
            QString::number(drumSnapSlider->value() / 100.0),
            QString::number(filterType),
            "0.1",
            "0.5"
            );

        QString fileName = QFileDialog::getSaveFileName(this, "Save Drum", "", "LMMS Preset (*.xpf)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << xpf;
                file.close();
                emit statusMessage("Drum saved: " + fileName);
            }
        }
    } else {

        QApplication::clipboard()->setText(displayFormula);
        emit statusMessage("Drum Formula copied to clipboard!");
    }
}

QString DrumEditorTab::getXpfTemplate() {
    QStringList lines;
    lines << "<?xml version=\"1.0\"?>"
          << "<!DOCTYPE lmms-project>"
          << "<lmms-project version=\"20\" creator=\"WaveConv\" type=\"instrumenttracksettings\">"
          << "<head/>"
          << "<instrumenttracksettings name=\"%1\" muted=\"0\" solo=\"0\">"
          << "<instrumenttrack vol=\"100\" pan=\"0\" basenote=\"%2\" pitchrange=\"1\">"
          << "<instrument name=\"xpressive\">"
          << "<xpressive version=\"0.1\" O1=\"%3\" O2=\"0\" bin=\"\">"
          << "<key/></xpressive></instrument>"
          << "<eldata fcut=\"%4\" fres=\"%5\" ftype=\"%6\" fwet=\"1\">"
          << "<elvol rel=\"%7\" dec=\"%8\" sustain=\"0\" amt=\"0\"/>"
          << "</eldata></instrumenttrack></instrumenttracksettings></lmms-project>";
    return lines.join("\n");
}

void DrumEditorTab::onLoadSample() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open Drum Sample", "", "Audio Files (*.wav)");
    if (filePath.isEmpty()) return;

    m_rawAudioData = loadWavFile(filePath, m_rawSampleRate);

    if (m_rawAudioData.empty()) {
        QMessageBox::warning(this, "Error", "Failed to load WAV file or file is empty.");
        return;
    }

    m_trimStartSlider->blockSignals(true);
    m_trimStartSlider->setValue(0);
    m_trimStartSlider->blockSignals(false);

    m_trimEndSlider->setValue(1000);
}

std::vector<float> DrumEditorTab::loadWavFile(const QString& filePath, int& outSampleRate) {
    std::vector<float> audioData;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return audioData;

    if (file.size() < 44) return audioData;

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    char riff[5] = {0};
    in.readRawData(riff, 4);
    if (QString(riff) != "RIFF") return audioData;

    quint32 fileSize;
    in >> fileSize;

    char wave[5] = {0};
    in.readRawData(wave, 4);
    if (QString(wave) != "WAVE") return audioData;

    int numChannels = 1;
    int bitsPerSample = 16;
    bool foundData = false;

    while (!in.atEnd() && !foundData) {
        char chunkId[5] = {0};
        if (in.readRawData(chunkId, 4) != 4) break;

        quint32 chunkSize;
        in >> chunkSize;

        if (QString(chunkId) == "fmt ") {
            quint16 audioFormat, channels, blockAlign, bps;
            quint32 sampleRate, byteRate;

            in >> audioFormat >> channels >> sampleRate >> byteRate >> blockAlign >> bps;

            outSampleRate = sampleRate;
            numChannels = channels;
            bitsPerSample = bps;

            if (chunkSize > 16) file.seek(file.pos() + (chunkSize - 16));
        } else if (QString(chunkId) == "data") {
            foundData = true;

            if (chunkSize > 50000000) {
                QMessageBox::warning(this, "File Too Large", "This sample is too large. Please use a single drum hit.");
                return audioData;
            }

            int bytesPerSample = bitsPerSample / 8;
            int numSamples = chunkSize / bytesPerSample;

            audioData.reserve(numSamples / numChannels);

            for (int i = 0; i < numSamples; i += numChannels) {
                float sampleVal = 0.0f;

                if (bitsPerSample == 16) {
                    qint16 val;
                    in >> val;
                    sampleVal = val / 32768.0f;
                    for(int c = 1; c < numChannels; ++c) { qint16 skip; in >> skip; }
                } else if (bitsPerSample == 24) {
                    char buf[3];
                    in.readRawData(buf, 3);
                    qint32 val = (buf[0] & 0xFF) | ((buf[1] & 0xFF) << 8) | ((buf[2] & 0xFF) << 16);
                    if (val & 0x800000) val |= 0xFF000000;
                    sampleVal = val / 8388608.0f;
                    for(int c = 1; c < numChannels; ++c) { in.readRawData(buf, 3); }
                } else {
                    in.skipRawData(bytesPerSample);
                    for(int c = 1; c < numChannels; ++c) { in.skipRawData(bytesPerSample); }
                }
                audioData.push_back(sampleVal);
            }
        } else {
            file.seek(file.pos() + chunkSize);
        }
    }
    return audioData;
}

void DrumEditorTab::analyzeAudioData(const std::vector<float>& audioData, int sampleRate) {
    int totalSamples = audioData.size();
    if (totalSamples == 0) return;

    float maxAmp = 0.0f;
    int peakIndex = 0;

    for (int i = 0; i < totalSamples; ++i) {
        float absVal = std::abs(audioData[i]);
        if (absVal > maxAmp) {
            maxAmp = absVal;
            peakIndex = i;
        }
    }
    float attackTime = static_cast<float>(peakIndex) / sampleRate;

    int decayIndex = totalSamples - 1;
    while (decayIndex > peakIndex && std::abs(audioData[decayIndex]) < maxAmp * 0.1f) {
        decayIndex--;
    }
    float decayTime = static_cast<float>(decayIndex - peakIndex) / sampleRate;

    int zeroCrossings = 0;
    int initialZeroCrossings = 0;
    int initialWindow = std::min(totalSamples, (int)(sampleRate * 0.02f));

    for (int i = 1; i < totalSamples; ++i) {
        if ((audioData[i] >= 0 && audioData[i - 1] < 0) || (audioData[i] < 0 && audioData[i - 1] >= 0)) {
            zeroCrossings++;
            if (i < initialWindow) initialZeroCrossings++;
        }
    }

    float baseFreq = (initialZeroCrossings / 2.0f) / 0.02f;
    float overallZCR = static_cast<float>(zeroCrossings) / totalSamples;

    QString drumType;
    QString expression;
    QString filterRec;

    if (overallZCR > 0.2f) {
        if (decayTime < 0.15f) {
            drumType = "Hi-Hat / Cymbal";
            expression = QString("(noise() * exp(-t * %1))").arg(5.0f / (decayTime == 0 ? 0.01 : decayTime), 0, 'f', 2);
            filterRec = "High-Pass Filter (HPF) at ~5000Hz";
        } else {
            drumType = "Snare Drum";
            // FIXED: Changed (f * %1) to (f + %1)
            expression = QString("(sinew(integrate(f + %1)) * exp(-t * %2)) + (noise() * exp(-t * %3) * 0.8)")
                             .arg(baseFreq < 100 ? 200 : baseFreq, 0, 'f', 1)
                             .arg(10.0f)
                             .arg(3.0f / decayTime, 0, 'f', 2);
            filterRec = "Band-Pass Filter (BPF) at ~2000Hz";
        }
    } else {
        drumType = "Kick Drum";
        float pitchDrop = baseFreq - 50.0f;
        if (pitchDrop < 0) pitchDrop = 100.0f;

        // FIXED: Changed f * (50 + ...) to f + ...
        expression = QString("sinew(integrate(f + %1 * exp(-t * 30))) * exp(-t * %2)")
                         .arg(pitchDrop, 0, 'f', 1)
                         .arg(5.0f / (decayTime == 0 ? 0.1 : decayTime), 0, 'f', 2);
        filterRec = "Low-Pass Filter (LPF) at ~200Hz to 500Hz";
    }

    m_lblAnalysisResults->setText(QString("Type: %1\nAttack: %2s | Decay: %3s\nInitial Pitch: ~%4 Hz")
                                      .arg(drumType).arg(attackTime, 0, 'f', 3)
                                      .arg(decayTime, 0, 'f', 3).arg(baseFreq, 0, 'f', 1));
    m_lblFilterRecommendation->setText(QString("Recommended External Filter: %1").arg(filterRec));

    m_outputExpression->setText(expression);
    emit expressionGenerated(expression);
}


void DrumEditorTab::onTrimChanged() {
    if (m_rawAudioData.empty()) return;

    int startVal = m_trimStartSlider->value();
    int endVal = m_trimEndSlider->value();

    if (startVal >= endVal) {
        m_trimStartSlider->blockSignals(true);
        m_trimStartSlider->setValue(endVal - 1);
        m_trimStartSlider->blockSignals(false);
        startVal = endVal - 1;
    }

    size_t startIndex = (startVal / 1000.0) * m_rawAudioData.size();
    size_t endIndex = (endVal / 1000.0) * m_rawAudioData.size();
    if (endIndex > m_rawAudioData.size()) endIndex = m_rawAudioData.size();


    m_lblTrimDetails->setText(QString("Trim: %1% to %2%")
                                  .arg(startVal / 10.0, 0, 'f', 1)
                                  .arg(endVal / 10.0, 0, 'f', 1));

    double totalDur = (double)m_rawAudioData.size() / m_rawSampleRate;
    double startSec = (double)startIndex / m_rawSampleRate;
    double endSec = (double)endIndex / m_rawSampleRate;

    if (totalDur > 0.0) {
        std::function<double(double)> waveFunc = [this](double t) {
            size_t idx = static_cast<size_t>(t * m_rawSampleRate);
            if (idx < m_rawAudioData.size()) {
                return static_cast<double>(m_rawAudioData[idx]);
            }
            return 0.0;
        };

        m_analyzeScope->updateScope(waveFunc, totalDur, 1.0);

        m_analyzeScope->setHighlight(startSec, endSec);
    }


    std::vector<float> slicedAudio(m_rawAudioData.begin() + startIndex, m_rawAudioData.begin() + endIndex);
    analyzeAudioData(slicedAudio, m_rawSampleRate);
}
