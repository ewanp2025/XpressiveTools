#include "vocaltransformtab.h"
#include "synthengine.h"
#include <QPainter>
#include <QDebug>

VocalTransformTab::VocalTransformTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth) {
    
    // Set up standard 44.1kHz mono audio format for recording
    audioFormat.setSampleRate(44100);
    audioFormat.setChannelCount(1);
    audioFormat.setSampleFormat(QAudioFormat::Float);

    QAudioDevice info = QMediaDevices::defaultAudioInput();
    if (!info.isFormatSupported(audioFormat)) {
        qWarning() << "Default format not supported, trying to use nearest.";
        audioFormat = info.preferredFormat();
    }

    setupUI();
}

VocalTransformTab::~VocalTransformTab() {
    if (audioInput) {
        audioInput->stop();
        delete audioInput;
    }
}

void VocalTransformTab::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Top Controls
    QHBoxLayout *topLayout = new QHBoxLayout();
    btnRecord = new QPushButton("🔴 Start Recording");
    btnRecord->setStyleSheet("background-color: #550000; color: white; font-weight: bold; padding: 10px;");
    
    btnProcess = new QPushButton("Apply Formant/Pitch Shift");
    
    topLayout->addWidget(btnRecord);
    topLayout->addWidget(btnProcess);
    mainLayout->addLayout(topLayout);

    // Waveform Canvas Area
    QWidget* canvasPlaceholder = new QWidget();
    canvasPlaceholder->setObjectName("waveArea");
    canvasPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    canvasPlaceholder->setMinimumHeight(150);
    mainLayout->addWidget(new QLabel("Raw Audio (Click & Drag to Trim):"));
    mainLayout->addWidget(canvasPlaceholder);

    // Processing Sliders
    QHBoxLayout *slidersLayout = new QHBoxLayout();
    
    QVBoxLayout *pitchLayout = new QVBoxLayout();
    pitchSlider = new QSlider(Qt::Horizontal);
    pitchSlider->setRange(-12, 12); pitchSlider->setValue(0);
    pitchLayout->addWidget(new QLabel("Pitch Shift (Semitones)"));
    pitchLayout->addWidget(pitchSlider);
    
    QVBoxLayout *formantLayout = new QVBoxLayout();
    formantSlider = new QSlider(Qt::Horizontal);
    formantSlider->setRange(-100, 100); formantSlider->setValue(0);
    formantLayout->addWidget(new QLabel("Formant Character (Throat Size)"));
    formantLayout->addWidget(formantSlider);

    slidersLayout->addLayout(pitchLayout);
    slidersLayout->addLayout(formantLayout);
    mainLayout->addLayout(slidersLayout);

    // Output
    xpressiveOutput = new QTextEdit();
    xpressiveOutput->setReadOnly(true);
    mainLayout->addWidget(new QLabel("Xpressive Math Output:"));
    mainLayout->addWidget(xpressiveOutput);

    // Connections
    connect(btnRecord, &QPushButton::clicked, this, &VocalTransformTab::toggleRecording);
    connect(btnProcess, &QPushButton::clicked, this, &VocalTransformTab::processAudio);
}

void VocalTransformTab::toggleRecording() {
    if (!audioInput) {
        audioInput = new QAudioSource(QMediaDevices::defaultAudioInput(), audioFormat, this);
    }

    if (audioInput->state() == QAudio::StoppedState || audioInput->state() == QAudio::IdleState) {
        recordedBuffer.clear();
        processedBuffer.clear();
        audioIODevice = audioInput->start();
        connect(audioIODevice, &QIODevice::readyRead, this, &VocalTransformTab::handleAudioData);
        
        btnRecord->setText("⏹ Stop Recording");
        btnRecord->setStyleSheet("background-color: #880000; color: white;");
    } else {
        audioInput->stop();
        btnRecord->setText("🔴 Start Recording");
        btnRecord->setStyleSheet("background-color: #550000; color: white;");
        update(); // Force a redraw to show the recorded waveform
    }
}

void VocalTransformTab::handleAudioData() {
    if (!audioIODevice) return;
    
    QByteArray data = audioIODevice->readAll();
    int samples = data.size() / sizeof(float);
    const float* rawSamples = reinterpret_cast<const float*>(data.constData());
    
    for (int i = 0; i < samples; ++i) {
        recordedBuffer.push_back(rawSamples[i]);
    }
    
    // Keep UI responsive and updating while recording
    update();
}

void VocalTransformTab::processAudio() {
    // This is where we will write the kiss_fft Phase Vocoder!
    // For now, let's just copy the recorded buffer so we don't crash.
    processedBuffer = recordedBuffer;
    
    generateXpressive();
}

void VocalTransformTab::generateXpressive() {
    // We will port over your binary tree generator from PCMEditorTab here.
    if (processedBuffer.empty()) return;
    xpressiveOutput->setText("Expression generating...");
}

// ---- You can paste the paintEvent, mousePressEvent, and mouseMoveEvent 
// ---- directly from your PCMEditorTab here so the waveform drawing works immediately!
void VocalTransformTab::paintEvent(QPaintEvent *event) {
    // Copy your painting logic from PCMEditorTab here
}
void VocalTransformTab::mousePressEvent(QMouseEvent *event) {}
void VocalTransformTab::mouseMoveEvent(QMouseEvent *event) {}