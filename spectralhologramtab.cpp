#include "spectralhologramtab.h"
#include "synthengine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QDataStream>
#include <cmath>
#include <algorithm>
#include <functional>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QSpinBox>
#include <QPainterPath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpectralHologramTab::SpectralHologramTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth) 
{
    setupUI();
    

    m_harmonicWeights.resize(32, 0.0);
    m_masterEnv.resize(m_envResolution, 0.0);
}

SpectralHologramTab::~SpectralHologramTab() {}

void SpectralHologramTab::setupUI() {
    this->setStyleSheet(R"(
        QWidget { background-color: #0a0a0f; color: #00ffff; font-family: "Consolas", monospace; }
        QGroupBox { border: 1px solid #005555; margin-top: 10px; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; color: #00aaaa; }
        QPushButton { background-color: #003333; border: 1px solid #00ffff; padding: 6px; font-weight: bold; }
        QPushButton:hover { background-color: #005555; color: #ffffff; }
        QPushButton:checked { background-color: #aa0000; color: white; border: 1px solid #ff0000; }
        QSlider::groove:horizontal { border: 1px solid #005555; height: 6px; background: #001111; }
        QSlider::handle:horizontal { background: #00ffff; width: 14px; margin: -4px 0; border-radius: 7px; }
        QTextEdit { background-color: #050505; border: 1px solid #004444; color: #00ffaa; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);


    QHBoxLayout* topLayout = new QHBoxLayout();
    btnLoad = new QPushButton("1. Load Source WAV");
    btnAnalyze = new QPushButton("2. Extract Spectral DNA");
    btnAnalyze->setEnabled(false);
    lblStatus = new QLabel("Ready. Load a sustained vocal, synth, or string sample.");
    
    topLayout->addWidget(btnLoad);
    topLayout->addWidget(btnAnalyze);
    topLayout->addWidget(lblStatus, 1);
    mainLayout->addLayout(topLayout);


    QWidget* canvasPlaceholder = new QWidget();
    canvasPlaceholder->setMinimumHeight(180);
    canvasPlaceholder->setAttribute(Qt::WA_TransparentForMouseEvents);
    mainLayout->addWidget(canvasPlaceholder); // Paint event draws over this


    QGroupBox* controlGroup = new QGroupBox("Holographic Resynthesis Engine");
    QFormLayout* formLayout = new QFormLayout(controlGroup);

    spinPartials = new QSpinBox();
    spinPartials->setRange(1, 32);
    spinPartials->setValue(16);
    
    sldStretch = new QSlider(Qt::Horizontal);
    sldStretch->setRange(1, 1000);
    sldStretch->setValue(100); 
    
    sldInharmonicity = new QSlider(Qt::Horizontal);
    sldInharmonicity->setRange(0, 200);
    sldInharmonicity->setValue(100);

    sldFormantShift = new QSlider(Qt::Horizontal);
    sldFormantShift->setRange(50, 200);
    sldFormantShift->setValue(100);

    cmbMaterial = new QComboBox();
    cmbMaterial->addItems({"Original Acoustic Extraction", "Force: Glass / Chimes", "Force: Metallic Bell", "Force: Hollow Wood", "Force: Pure Sawtooth"});
    
    formLayout->addRow("Additive Partials (Oscillators):", spinPartials);
    formLayout->addRow("Time Stretch (Infinite Res):", sldStretch);
    formLayout->addRow("Harmonic -> Inharmonic Shift:", sldInharmonicity);
    formLayout->addRow("Formant Shift (Pitch Indep.):", sldFormantShift);
    formLayout->addRow("Material Mutation:", cmbMaterial);
    
    mainLayout->addWidget(controlGroup);

    QHBoxLayout* outRow = new QHBoxLayout();
    cmbBuildMode = new QComboBox();
    cmbBuildMode->addItems({"Nightly (Optimized Additive Variables)", "Legacy (Not Recommended for >4 Partials)"});
    
    btnGenerate = new QPushButton("3. Compile to Math Expression");
    btnGenerate->setStyleSheet("background-color: #006600;");
    
    btnPlay = new QPushButton("▶ Play Hologram");
    btnPlay->setCheckable(true);

    outRow->addWidget(new QLabel("Syntax:"));
    outRow->addWidget(cmbBuildMode);
    outRow->addWidget(btnGenerate);
    outRow->addWidget(btnPlay);
    mainLayout->addLayout(outRow);

    txtOutput = new QTextEdit();
    txtOutput->setReadOnly(true);
    txtOutput->setMinimumHeight(120);
    mainLayout->addWidget(txtOutput);


    connect(btnLoad, &QPushButton::clicked, this, &SpectralHologramTab::onLoadWav);
    connect(btnAnalyze, &QPushButton::clicked, this, &SpectralHologramTab::onAnalyze);
    connect(btnGenerate, &QPushButton::clicked, this, &SpectralHologramTab::onGenerate);
    connect(btnPlay, &QPushButton::toggled, this, &SpectralHologramTab::togglePlay);
    connect(cmbMaterial, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SpectralHologramTab::onApplyMaterial);
    

    connect(spinPartials, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](){ update(); });
    connect(sldInharmonicity, &QSlider::valueChanged, this, [this](){ update(); });
}

bool SpectralHologramTab::loadWavToMemory(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    char riff[4], wave[4], fmt[4], data[4];
    uint32_t chunkSize, subChunk1Size, sampleRate, byteRate, subChunk2Size;
    uint16_t audioFormat, numChannels, blockAlign, bitsPerSample;

    file.read(riff, 4); stream >> chunkSize; file.read(wave, 4);
    if (strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) return false;

    file.read(fmt, 4); stream >> subChunk1Size;
    stream >> audioFormat >> numChannels >> sampleRate >> byteRate >> blockAlign >> bitsPerSample;

    if (audioFormat != 1 && audioFormat != 3) return false; // Only PCM or Float


    while (!file.atEnd()) {
        file.read(data, 4);
        stream >> subChunk2Size;
        if (strncmp(data, "data", 4) == 0) break;
        file.seek(file.pos() + subChunk2Size);
    }

    m_sampleRate = sampleRate;
    m_audioData.clear();
    QByteArray raw = file.read(subChunk2Size);
    

    int bytesPerSample = bitsPerSample / 8;
    int numSamples = raw.size() / (bytesPerSample * numChannels);
    m_audioData.reserve(numSamples);

    double maxVal = 0.001;

    for (int i = 0; i < numSamples; ++i) {
        double val = 0.0;
        int offset = i * numChannels * bytesPerSample;
        
        if (bitsPerSample == 16) {
            const int16_t* ptr = reinterpret_cast<const int16_t*>(raw.data() + offset);
            val = *ptr / 32768.0;
            if (numChannels > 1) val = (val + (*(ptr+1) / 32768.0)) * 0.5;
        } else if (bitsPerSample == 32 && audioFormat == 3) {
            const float* ptr = reinterpret_cast<const float*>(raw.data() + offset);
            val = *ptr;
            if (numChannels > 1) val = (val + *(ptr+1)) * 0.5;
        }
        
        m_audioData.push_back(val);
        if (std::abs(val) > maxVal) maxVal = std::abs(val);
    }


    for (double& d : m_audioData) d /= maxVal;
    
    m_duration = m_audioData.size() / m_sampleRate;
    return true;
}

void SpectralHologramTab::onLoadWav() {
    QString path = QFileDialog::getOpenFileName(this, "Load Source Audio", "", "WAV Files (*.wav)");
    if (!path.isEmpty()) {
        if (loadWavToMemory(path)) {
            lblStatus->setText(QString("Loaded: %1s at %2Hz. Ready for Extraction.").arg(m_duration, 0, 'f', 2).arg(m_sampleRate));
            btnAnalyze->setEnabled(true);
            update();
        } else {
            QMessageBox::warning(this, "Error", "Could not parse WAV file. Must be 16-bit or 32-bit float.");
        }
    }
}

void SpectralHologramTab::onAnalyze() {
    if (m_audioData.empty()) return;
    
    lblStatus->setText("Extracting Amplitude and Harmonic DNA...");
    QApplication::processEvents();

    m_masterEnv.assign(m_envResolution, 0.0);
    int step = m_audioData.size() / m_envResolution;
    
    for (int i = 0; i < m_envResolution; ++i) {
        double sumSquares = 0.0;
        int startIdx = i * step;
        int endIdx = std::min(startIdx + step, (int)m_audioData.size());
        
        for (int j = startIdx; j < endIdx; ++j) {
            sumSquares += m_audioData[j] * m_audioData[j];
        }
        m_masterEnv[i] = std::sqrt(sumSquares / (endIdx - startIdx));
    }

    double maxEnv = *std::max_element(m_masterEnv.begin(), m_masterEnv.end());
    if (maxEnv > 0) {
        for(double& v : m_masterEnv) v /= maxEnv;
    }


    double f0 = 220.0; 
    m_harmonicWeights.assign(32, 0.0);
    

    for (int h = 1; h <= 32; ++h) {
        double targetFreq = f0 * h;
        if (targetFreq > m_sampleRate / 2.0) break;
        
        double sinSum = 0.0, cosSum = 0.0;
        for (size_t i = 0; i < m_audioData.size(); i += 4) {
            double t = (double)i / m_sampleRate;
            double phase = 2.0 * M_PI * targetFreq * t;
            sinSum += m_audioData[i] * std::sin(phase);
            cosSum += m_audioData[i] * std::cos(phase);
        }
        m_harmonicWeights[h-1] = std::sqrt(sinSum*sinSum + cosSum*cosSum);
    }


    double maxHarm = *std::max_element(m_harmonicWeights.begin(), m_harmonicWeights.end());
    if (maxHarm > 0) {
        for(double& w : m_harmonicWeights) w /= maxHarm;
    }
    
    cmbMaterial->setCurrentIndex(0);
    lblStatus->setText("Extraction Complete! isolated into 32 partials.");
    update();
}

void SpectralHologramTab::onApplyMaterial(int index) {
    if (m_harmonicWeights.empty()) return;
    
    if (index == 0) {
        onAnalyze();
        return;
    }
    

    for (int h = 1; h <= 32; ++h) {
        double weight = 0.0;
        if (index == 1) {
            if (h == 1 || h == 5 || h == 11 || h == 19 || h == 28) weight = 1.0 / std::sqrt(h);
            sldInharmonicity->setValue(135);
        } 
        else if (index == 2) {
            if (h == 1) weight = 1.0;
            if (h == 2) weight = 0.6;
            if (h == 3) weight = 0.4;
            if (h == 4) weight = 0.8;
            if (h == 8) weight = 0.5;
            sldInharmonicity->setValue(115);
        }
        else if (index == 3) {
            if (h % 2 != 0) weight = 1.0 / h;
            sldInharmonicity->setValue(100);
        }
        else if (index == 4) {
            weight = 1.0 / h;
            sldInharmonicity->setValue(100);
        }
        m_harmonicWeights[h-1] = weight;
    }
    update();
}

QString SpectralHologramTab::buildTree(int start, int end, const std::vector<double>& arr) {
    if (start == end) return QString::number(arr[start], 'f', 4);
    int mid = start + (end - start) / 2;
    return QString("((s <= %1) ? %2 : %3)").arg(mid)
           .arg(buildTree(start, mid, arr))
           .arg(buildTree(mid + 1, end, arr));
}

void SpectralHologramTab::onGenerate() {
    if (m_masterEnv.empty() || m_masterEnv[0] == 0 && m_masterEnv[m_envResolution-1] == 0) {
        QMessageBox::warning(this, "Wait", "Please analyze an audio file first.");
        return;
    }

    int activePartials = spinPartials->value();
    double speed = sldStretch->value() / 100.0;
    double inharmonicity = sldInharmonicity->value() / 100.0;
    double formant = sldFormantShift->value() / 100.0;
    bool isNightly = (cmbBuildMode->currentIndex() == 0);

    QString envTree = buildTree(0, m_envResolution - 1, m_masterEnv);
    QString outCode;

    if (isNightly) {
        outCode += QString("// --- SPECTRAL HOLOGRAM ENGINE ---\n");
        outCode += QString("var speed := %1;\n").arg(speed, 0, 'f', 2);
        outCode += QString("var duration := %1;\n").arg(m_duration, 0, 'f', 2);
        outCode += QString("var seq_t := mod(t * speed, duration);\n");
        outCode += QString("var s := floor((seq_t / duration) * %1);\n").arg(m_envResolution - 1);
        outCode += QString("var env := %1;\n\n").arg(envTree);
        

        outCode += QString("var f0 := f * (1.0 + 0.005 * sin(seq_t * 6.0));\n\n");
        
        QString partialSum = "";
        for (int h = 1; h <= activePartials; ++h) {
            double amp = m_harmonicWeights[h-1];
            if (amp < 0.001) continue;
            

            double freqMult = h * std::pow(inharmonicity, h - 1);

            double actualAmp = amp * (1.0 - std::abs(1.0 - formant)*0.5); 
            
            QString pName = QString("p%1").arg(h);
            outCode += QString("var %1 := %2 * sin(integrate(f0 * %3));\n")
                        .arg(pName).arg(actualAmp, 0, 'f', 4).arg(freqMult, 0, 'f', 4);
            
            if (!partialSum.isEmpty()) partialSum += " + ";
            partialSum += pName;
        }
        
        if (partialSum.isEmpty()) partialSum = "0";
        outCode += QString("\nclamp(-1.0, (%1) * env, 1.0);").arg(partialSum);
        
    } else {

        if (activePartials > 8) {
            QMessageBox::warning(this, "Legacy Mode", "Generating >8 partials in Legacy mode creates expressions too large for LMMS 1.2. Clamping to 8.");
            activePartials = 8;
        }
        
        QString t_var = QString("mod(t * %1, %2)").arg(speed).arg(m_duration);
        QString s_var = QString("floor((%1 / %2) * %3)").arg(t_var).arg(m_duration).arg(m_envResolution - 1);
        
        QString rawEnv = envTree;
        rawEnv.replace("s", s_var);
        
        QString partialSum = "";
        for (int h = 1; h <= activePartials; ++h) {
            double amp = m_harmonicWeights[h-1];
            if (amp < 0.001) continue;
            double freqMult = h * std::pow(inharmonicity, h - 1);
            
            QString osc = QString("(%1 * sinew(integrate(f * %2)))").arg(amp).arg(freqMult);
            if (!partialSum.isEmpty()) partialSum += " + ";
            partialSum += osc;
        }
        
        outCode = QString("clamp(-1.0, (%1) * (%2), 1.0)").arg(partialSum).arg(rawEnv);
    }

    txtOutput->setText(outCode);
    QApplication::clipboard()->setText(outCode);
    lblStatus->setText("Compiled! Copied to clipboard.");
    
    if (btnPlay->isChecked()) {
        togglePlay(true);
    }
}

void SpectralHologramTab::togglePlay(bool checked) {
    if (!m_ghostSynth) return;

    if (checked) {
        btnPlay->setText("⏹ Stop Hologram");
        btnPlay->setStyleSheet("background-color: #aa0000; color: white;");


        int activePartials = spinPartials->value();
        double speed = sldStretch->value() / 100.0;
        double inharmonicity = sldInharmonicity->value() / 100.0;
        double duration = m_duration > 0 ? m_duration : 1.0;
        std::vector<double> localEnv = m_masterEnv;
        std::vector<double> localHarmonics = m_harmonicWeights;
        
        auto algo = [=, phase_accum = 0.0, last_t = -1.0](double t) mutable {
            double dt = (last_t < 0.0) ? 0.0000226 : (t - last_t);
            last_t = t;
            
            double seq_t = std::fmod(t * speed, duration);
            int s = (int)((seq_t / duration) * (localEnv.size() - 1));
            s = std::clamp(s, 0, (int)localEnv.size() - 1);
            
            double env = localEnv[s];
            

            double f0 = 220.0 * (1.0 + 0.005 * std::sin(seq_t * 6.0));
            phase_accum += f0 * dt;
            
            double out = 0.0;
            for (int h = 1; h <= activePartials; ++h) {
                double amp = localHarmonics[h-1];
                if (amp < 0.001) continue;
                double freqMult = h * std::pow(inharmonicity, h - 1);
                out += amp * std::sin(2.0 * M_PI * phase_accum * freqMult);
            }
            
            return std::clamp(out * env, -1.0, 1.0);
        };
        
        m_ghostSynth->setAudioSource(algo);
        m_ghostSynth->start();

    } else {
        btnPlay->setText("▶ Play Hologram");
        btnPlay->setStyleSheet("");
        m_ghostSynth->stop();
    }
}

void SpectralHologramTab::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    

    QRect canvasBox = QRect(20, 70, width() - 40, 180);
    painter.fillRect(canvasBox, QColor(0, 15, 15));
    painter.setPen(QPen(QColor(0, 60, 60), 1, Qt::DotLine));
    for(int i=1; i<10; i++) {
        painter.drawLine(canvasBox.left(), canvasBox.top() + (canvasBox.height()*i/10), canvasBox.right(), canvasBox.top() + (canvasBox.height()*i/10));
    }

    if (m_masterEnv.empty() || m_harmonicWeights.empty()) return;


    painter.setPen(QPen(QColor(0, 100, 255, 100), 2));
    painter.setBrush(QColor(0, 100, 255, 30));
    QPainterPath envPath;
    envPath.moveTo(canvasBox.bottomLeft());
    
    float wStep = (float)canvasBox.width() / (m_masterEnv.size() - 1);
    for (size_t i = 0; i < m_masterEnv.size(); ++i) {
        float x = canvasBox.left() + i * wStep;
        float y = canvasBox.bottom() - (m_masterEnv[i] * canvasBox.height());
        envPath.lineTo(x, y);
    }
    envPath.lineTo(canvasBox.bottomRight());
    painter.drawPath(envPath);


    int activePartials = spinPartials->value();
    float barWidth = canvasBox.width() / 32.0f;
    float inharmonicShift = sldInharmonicity->value() / 100.0;
    
    for (int h = 1; h <= 32; ++h) {
        if (h > activePartials) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(50, 50, 50, 80));
        } else {
            painter.setPen(QPen(QColor(0, 255, 255), 1));

            int r = std::min(255, (int)(std::abs(inharmonicShift - 1.0) * 500));
            painter.setBrush(QColor(r, 255 - r, 200, 200)); 
        }

        double amp = m_harmonicWeights[h-1];
        float shiftOffset = (std::pow(inharmonicShift, h-1) - 1.0) * 10.0f;
        float x = canvasBox.left() + ((h-1) * barWidth) + shiftOffset;
        

        if (x < canvasBox.right() && x >= canvasBox.left()) {
            float hPx = amp * canvasBox.height() * 0.9f;
            painter.drawRect(QRectF(x + 2, canvasBox.bottom() - hPx, barWidth - 4, hPx));
        }
    }
}
