#include "vocaltransformtab.h"
#include "synthengine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QPainter>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QMouseEvent>
#include <QApplication>
#include <cmath>
#include <algorithm>
#include <QClipboard>

VocalTransformTab::VocalTransformTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth)
{
    setupUI();

    connect(btnRecord, &QPushButton::clicked, this, &VocalTransformTab::toggleRecording);
    connect(btnProcess, &QPushButton::clicked, this, &VocalTransformTab::processAudio);
    connect(btnPlay, &QPushButton::clicked, this, &VocalTransformTab::playAudio);
    connect(btnTrim, &QPushButton::clicked, this, &VocalTransformTab::trimSelection);
    connect(btnVocalMask, &QPushButton::clicked, this, &VocalTransformTab::applyVocalMask);
    connect(btnShiftBright, &QPushButton::clicked, this, &VocalTransformTab::applyBrightShift);
    connect(btnShiftDeep, &QPushButton::clicked, this, &VocalTransformTab::applyDeepShift);
}

VocalTransformTab::~VocalTransformTab()
{
    if (audioInput) {
        audioInput->stop();
        delete audioInput;
        audioInput = nullptr;
    }
    if (audioOutput) {
        audioOutput->stop();
        delete audioOutput;
        audioOutput = nullptr;
    }
    if (playbackBuffer) {
        delete playbackBuffer;
        playbackBuffer = nullptr;
    }
}

void VocalTransformTab::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);


    QHBoxLayout *qualityLayout = new QHBoxLayout();
    qualityLayout->addWidget(new QLabel("Sample Rate:"));
    sampleRateCombo = new QComboBox(this);
    sampleRateCombo->addItem("48000 Hz", 48000);
    sampleRateCombo->addItem("44100 Hz", 44100);
    sampleRateCombo->addItem("16000 Hz", 16000);
    sampleRateCombo->setCurrentIndex(1); // Default 44.1k
    qualityLayout->addWidget(sampleRateCombo);

    qualityLayout->addWidget(new QLabel("Bit Depth:"));
    bitDepthCombo = new QComboBox(this);
    bitDepthCombo->addItem("16-bit Integer", QAudioFormat::Int16);
    bitDepthCombo->addItem("32-bit Float (Hi-Res)", QAudioFormat::Float);
    qualityLayout->addWidget(bitDepthCombo);
    qualityLayout->addStretch();
    mainLayout->addLayout(qualityLayout);


    QHBoxLayout *controlLayout = new QHBoxLayout();
    btnRecord = new QPushButton("🎤 Start Recording", this);
    btnRecord->setCheckable(true);
    btnRecord->setStyleSheet("font-weight: bold; padding: 8px;");

    btnPlay = new QPushButton("▶ Play Audio", this);
    btnPlay->setStyleSheet("font-weight: bold; padding: 8px; background-color: #004488; color: white;");

    btnTrim = new QPushButton("✂ Trim Selection", this);
    btnTrim->setStyleSheet("font-weight: bold; padding: 8px; background-color: #884400; color: white;");

    btnProcess = new QPushButton("⚡ Process to Xpressive", this);
    btnProcess->setStyleSheet("font-weight: bold; padding: 8px; background-color: #006600; color: white;");

    controlLayout->addWidget(btnRecord);
    controlLayout->addWidget(btnPlay);
    controlLayout->addWidget(btnTrim);
    controlLayout->addWidget(btnProcess);
    mainLayout->addLayout(controlLayout);


    QHBoxLayout *presetLayout = new QHBoxLayout();
    btnShiftDeep = new QPushButton("🌘 Shift Timbre: Deep", this);
    btnShiftDeep->setStyleSheet("font-weight: bold; padding: 8px; background-color: #333366; color: white;");

    btnShiftBright = new QPushButton("🌟 Shift Timbre: Bright", this);
    btnShiftBright->setStyleSheet("font-weight: bold; padding: 8px; background-color: #663333; color: white;");

    btnVocalMask = new QPushButton("🎭 Apply Identity Mask (Non-Human)", this);
    btnVocalMask->setStyleSheet("font-weight: bold; padding: 8px; background-color: #550088; color: white;");

    presetLayout->addWidget(btnShiftDeep);
    presetLayout->addWidget(btnShiftBright);
    presetLayout->addWidget(btnVocalMask);
    mainLayout->addLayout(presetLayout);


    QFormLayout *sliderLayout = new QFormLayout();
    pitchSlider = new QSlider(Qt::Horizontal, this);
    pitchSlider->setRange(50, 200);
    pitchSlider->setValue(100);
    sliderLayout->addRow("Pitch Shift (%):", pitchSlider);

    formantSlider = new QSlider(Qt::Horizontal, this);
    formantSlider->setRange(50, 200);
    formantSlider->setValue(100);
    sliderLayout->addRow("Formant Shift (%):", formantSlider);

    roboticSlider = new QSlider(Qt::Horizontal, this);
    roboticSlider->setRange(0, 100);
    roboticSlider->setValue(0);
    sliderLayout->addRow("Robotic Effect (%):", roboticSlider);

    mainLayout->addLayout(sliderLayout);


    mainLayout->addWidget(new QLabel("Waveform (Click + Drag to select region):"));

    setMinimumHeight(550);


    QHBoxLayout *outputModeLayout = new QHBoxLayout();
    outputModeLayout->addWidget(new QLabel("Output Format:"));
    buildModeCombo = new QComboBox(this);
    buildModeCombo->addItem("Nightly / 1.3 Xpressive");
    buildModeCombo->addItem("Legacy (1.2) Xpressive");
    outputModeLayout->addWidget(buildModeCombo);
    outputModeLayout->addStretch();
    mainLayout->addLayout(outputModeLayout);

    mainLayout->addWidget(new QLabel("Xpressive Output:"));
    xpressiveOutput = new QTextEdit(this);
    xpressiveOutput->setReadOnly(true);
    xpressiveOutput->setMinimumHeight(140);
    xpressiveOutput->setFontFamily("Consolas");
    mainLayout->addWidget(xpressiveOutput);

    QLabel *status = new QLabel("Ready — Select Quality, click 'Start Recording', and speak into your microphone", this);
    status->setWordWrap(true);
    mainLayout->addWidget(status);
}

void VocalTransformTab::applyVocalMask()
{
    pitchSlider->setValue(60);
    formantSlider->setValue(155);
    roboticSlider->setValue(85);
    QMessageBox::information(this, "Identity Mask Applied",
                             "Vocal parameters have been heavily scrambled to create a non-human, synthetic output.");
}

void VocalTransformTab::applyBrightShift()
{
    pitchSlider->setValue(135);
    formantSlider->setValue(125);
    roboticSlider->setValue(0);
    QMessageBox::information(this, "Timbre Shift Applied",
                             "Vocal resonance has been shifted upward for a brighter acoustic profile.");
}

void VocalTransformTab::applyDeepShift()
{
    pitchSlider->setValue(75);
    formantSlider->setValue(85);
    roboticSlider->setValue(0);
    QMessageBox::information(this, "Timbre Shift Applied",
                             "Vocal resonance has been shifted downward for a deeper acoustic profile.");
}

void VocalTransformTab::toggleRecording()
{
    if (btnRecord->isChecked()) {
        btnRecord->setText("⏹ Stop Recording");
        btnRecord->setStyleSheet("background-color: #aa0000; color: white; font-weight: bold;");

        recordedBuffer.clear();
        processedBuffer.clear();

        selectionStartPixel = -1;
        selectionEndPixel = -1;


        audioFormat.setSampleRate(sampleRateCombo->currentData().toInt());
        audioFormat.setChannelCount(1);
        audioFormat.setSampleFormat(static_cast<QAudioFormat::SampleFormat>(bitDepthCombo->currentData().toInt()));

        QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
        if (!inputDevice.isNull()) {
            if (!inputDevice.isFormatSupported(audioFormat)) {
                audioFormat = inputDevice.preferredFormat();
            }

            audioInput = new QAudioSource(inputDevice, audioFormat, this);
            audioIODevice = audioInput->start();
            if (audioIODevice) {
                connect(audioIODevice, &QIODevice::readyRead, this, &VocalTransformTab::handleAudioData);
            }
        } else {
            QMessageBox::warning(this, "Error", "No microphone detected!");
            btnRecord->setChecked(false);
            btnRecord->setText("🎤 Start Recording");
        }
    } else {
        if (audioInput) {
            audioInput->stop();
            delete audioInput;
            audioInput = nullptr;
        }
        btnRecord->setText("🎤 Start Recording");
        btnRecord->setStyleSheet("");
        update();
    }
}

void VocalTransformTab::handleAudioData()
{
    if (!audioIODevice) return;

    QByteArray data = audioIODevice->readAll();

    if (audioFormat.sampleFormat() == QAudioFormat::Int16) {
        const int16_t* samples = reinterpret_cast<const int16_t*>(data.constData());
        int numSamples = data.size() / sizeof(int16_t);
        for (int i = 0; i < numSamples; ++i) {
            recordedBuffer.push_back(samples[i] / 32768.0);
        }
    } else if (audioFormat.sampleFormat() == QAudioFormat::Float) {
        const float* samples = reinterpret_cast<const float*>(data.constData());
        int numSamples = data.size() / sizeof(float);
        for (int i = 0; i < numSamples; ++i) {
            recordedBuffer.push_back(samples[i]);
        }
    }

    update();
}

void VocalTransformTab::playAudio()
{
    if (recordedBuffer.empty()) {
        QMessageBox::warning(this, "No Audio", "Record something to play back first!");
        return;
    }

    if (audioOutput) {
        audioOutput->stop();
        delete audioOutput;
        audioOutput = nullptr;
    }
    if (playbackBuffer) {
        delete playbackBuffer;
        playbackBuffer = nullptr;
    }

    int startIdx = 0;
    int endIdx = recordedBuffer.size();

    if (selectionStartPixel != -1 && selectionEndPixel != -1 && selectionStartPixel != selectionEndPixel) {
        int widgetWidth = width();
        startIdx = static_cast<int>((std::min(selectionStartPixel, selectionEndPixel) / (float)widgetWidth) * recordedBuffer.size());
        endIdx   = static_cast<int>((std::max(selectionStartPixel, selectionEndPixel) / (float)widgetWidth) * recordedBuffer.size());

        startIdx = std::max(0, std::min(startIdx, (int)recordedBuffer.size() - 1));
        endIdx   = std::max(startIdx + 1, std::min(endIdx, (int)recordedBuffer.size()));
    }

    QByteArray playData;
    playData.resize((endIdx - startIdx) * sizeof(int16_t));
    int16_t* ptr = reinterpret_cast<int16_t*>(playData.data());

    for (int i = startIdx; i < endIdx; ++i) {
        ptr[i - startIdx] = static_cast<int16_t>(std::clamp(recordedBuffer[i], -1.0, 1.0) * 32767.0);
    }

    playbackBuffer = new QBuffer(this);
    playbackBuffer->setData(playData);
    playbackBuffer->open(QIODevice::ReadOnly);

    QAudioFormat outputFormat = audioFormat;
    outputFormat.setSampleFormat(QAudioFormat::Int16);
    outputFormat.setChannelCount(1);

    QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
    audioOutput = new QAudioSink(outputDevice, outputFormat, this);
    audioOutput->start(playbackBuffer);
}

void VocalTransformTab::trimSelection()
{
    if (recordedBuffer.empty()) return;

    if (selectionStartPixel == -1 || selectionEndPixel == -1 || selectionStartPixel == selectionEndPixel) {
        QMessageBox::information(this, "Select Area", "Click and drag on the waveform to select an area to trim.");
        return;
    }

    int widgetWidth = width();
    int startIdx = static_cast<int>((std::min(selectionStartPixel, selectionEndPixel) / (float)widgetWidth) * recordedBuffer.size());
    int endIdx   = static_cast<int>((std::max(selectionStartPixel, selectionEndPixel) / (float)widgetWidth) * recordedBuffer.size());

    startIdx = std::max(0, std::min(startIdx, (int)recordedBuffer.size() - 1));
    endIdx   = std::max(startIdx + 1, std::min(endIdx, (int)recordedBuffer.size()));

    std::vector<double> trimmed;
    trimmed.assign(recordedBuffer.begin() + startIdx, recordedBuffer.begin() + endIdx);
    recordedBuffer = trimmed;

    selectionStartPixel = -1;
    selectionEndPixel = -1;
    update();
}

void VocalTransformTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(10, 10, 15));

    if (recordedBuffer.empty()) return;

    QRect waveRect = rect().adjusted(20, 180, -20, -280);
    int w = waveRect.width();
    int h = waveRect.height();
    int midY = waveRect.top() + h / 2;

    painter.setPen(QPen(QColor(0, 255, 100), 1.5));

    float samplesPerPixel = static_cast<float>(recordedBuffer.size()) / w;

    for (int x = 0; x < w; ++x) {
        int startIdx = static_cast<int>(x * samplesPerPixel);
        int endIdx = std::min(static_cast<int>((x + 1) * samplesPerPixel), (int)recordedBuffer.size() - 1);

        float minVal = 0.0f, maxVal = 0.0f;
        for (int i = startIdx; i <= endIdx; ++i) {
            float val = recordedBuffer[i];
            if (val < minVal) minVal = val;
            if (val > maxVal) maxVal = val;
        }

        int y1 = midY - static_cast<int>(maxVal * (h / 2.0f) * 0.95f);
        int y2 = midY - static_cast<int>(minVal * (h / 2.0f) * 0.95f);
        if (y1 == y2) y2 += 1;

        painter.drawLine(waveRect.left() + x, y1, waveRect.left() + x, y2);
    }

    if (selectionStartPixel != -1 && selectionEndPixel != -1) {
        int left = std::min(selectionStartPixel, selectionEndPixel);
        int right = std::max(selectionStartPixel, selectionEndPixel);
        painter.fillRect(left, waveRect.top(), right - left, waveRect.height(), QColor(0, 255, 120, 60));
        painter.setPen(QPen(QColor(0, 255, 120), 2, Qt::DashLine));
        painter.drawRect(left, waveRect.top(), right - left, waveRect.height());
    }
}

void VocalTransformTab::mousePressEvent(QMouseEvent *event)
{
    selectionStartPixel = event->pos().x();
    selectionEndPixel = event->pos().x();
    update();
}

void VocalTransformTab::mouseMoveEvent(QMouseEvent *event)
{
    if (selectionStartPixel != -1) {
        selectionEndPixel = event->pos().x();
        update();
    }
}

void VocalTransformTab::processAudio()
{
    if (recordedBuffer.empty()) {
        QMessageBox::warning(this, "No Audio", "Record something first!");
        return;
    }

    std::vector<double> inputBuffer = recordedBuffer;

    if (selectionStartPixel != -1 && selectionEndPixel != -1 && selectionStartPixel != selectionEndPixel) {
        int widgetWidth = width();
        int startIdx = static_cast<int>((std::min(selectionStartPixel, selectionEndPixel) / (float)widgetWidth) * recordedBuffer.size());
        int endIdx   = static_cast<int>((std::max(selectionStartPixel, selectionEndPixel) / (float)widgetWidth) * recordedBuffer.size());

        startIdx = std::max(0, std::min(startIdx, (int)recordedBuffer.size()-1));
        endIdx   = std::max(startIdx + 1, std::min(endIdx, (int)recordedBuffer.size()));

        inputBuffer.assign(recordedBuffer.begin() + startIdx, recordedBuffer.begin() + endIdx);
    }

    double pitchFactor = pitchSlider->value() / 100.0;
    double formantFactor = formantSlider->value() / 100.0;
    double robotic = roboticSlider->value() / 100.0;

    processedBuffer.clear();
    processedBuffer.reserve(inputBuffer.size());

    for (double sample : inputBuffer) {
        double processed = sample;
        if (robotic > 0.01) {
            int bits = 8 + static_cast<int>((1.0 - robotic) * 8);
            processed = std::floor(processed * (1 << bits)) / (1 << bits);
        }
        processedBuffer.push_back(processed);
    }

    QString expr;
    if (buildModeCombo->currentIndex() == 0) {
        expr = generateNightlyVocalExpression(processedBuffer, pitchFactor, formantFactor);
    } else {
        expr = generateLegacyVocalExpression(processedBuffer, pitchFactor, formantFactor);
    }

    xpressiveOutput->setPlainText(expr);
    QApplication::clipboard()->setText(expr);

    QMessageBox::information(this, "Success",
                             "Xpressive expression generated!\n\n"
                             "It has been copied to the clipboard.\n"
                             "Paste it into the O1 field in LMMS Xpressive.");
}

QString VocalTransformTab::generateNightlyVocalExpression(const std::vector<double>& buffer,
                                                          double pitchMult,
                                                          double formantMult)
{
    if (buffer.empty()) return "0";

    int N = buffer.size();
    double sampleRate = audioFormat.sampleRate() > 0 ? audioFormat.sampleRate() : 44100.0;

    std::function<QString(int, int)> buildTree = [&](int start, int end) -> QString {
        if (start == end) {
            return QString::number(buffer[start], 'f', 4);
        }
        int mid = start + (end - start) / 2;
        return QString("((s <= %1) ? %2 : %3)")
            .arg(mid)
            .arg(buildTree(start, mid))
            .arg(buildTree(mid + 1, end));
    };

    QString tree = buildTree(0, N - 1);

    QString expr = QString(
                       "var s := floor(mod(t * %1 * %2, %3));\n"
                       "var raw := %4;\n"
                       "clamp(-1.0, raw * %5, 1.0)")
                       .arg(sampleRate, 0, 'f', 1)
                       .arg(pitchMult, 0, 'f', 3)
                       .arg(N)
                       .arg(tree)
                       .arg(formantMult, 0, 'f', 3);

    return expr;
}

QString VocalTransformTab::generateLegacyVocalExpression(const std::vector<double>& buffer,
                                                         double pitchMult,
                                                         double formantMult)
{
    if (buffer.empty()) return "0";

    int N = buffer.size();
    double sampleRate = audioFormat.sampleRate() > 0 ? audioFormat.sampleRate() : 44100.0;

    std::function<QString(int, int)> buildTree = [&](int start, int end) -> QString {
        if (start == end) {
            return QString::number(buffer[start], 'f', 4);
        }
        int mid = start + (end - start) / 2;
        return QString("((s <= %1) ? %2 : %3)")
            .arg(mid)
            .arg(buildTree(start, mid))
            .arg(buildTree(mid + 1, end));
    };

    QString tree = buildTree(0, N - 1);

    QString expr = QString(
                       "var s := floor(mod(t * %1 * %2, %3));\n"
                       "var raw := %4;\n"
                       "max(-1.0, min(raw * %5, 1.0))")
                       .arg(sampleRate, 0, 'f', 1)
                       .arg(pitchMult, 0, 'f', 3)
                       .arg(N)
                       .arg(tree)
                       .arg(formantMult, 0, 'f', 3);

    return expr;
}

void VocalTransformTab::generateXpressive()
{
    processAudio();
}
