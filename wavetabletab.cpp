#include "wavetabletab.h"
#include "synthengine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QClipboard>
#include <cmath>
#include <algorithm>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QSpinBox>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WavetableTab::WavetableTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth) 
{
    setupUI();
    onParametersChanged(); // Initial render
}

WavetableTab::~WavetableTab() {}

void WavetableTab::setupUI() {
    this->setStyleSheet(R"(
        QWidget { background-color: #121218; color: #ff007f; font-family: "Consolas", monospace; }
        QGroupBox { border: 1px solid #7f00ff; margin-top: 10px; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; color: #aa55ff; }
        QPushButton { background-color: #2a0055; border: 1px solid #ff007f; padding: 8px; font-weight: bold; }
        QPushButton:hover { background-color: #5500aa; color: #ffffff; }
        QPushButton:checked { background-color: #aa0000; color: white; border: 1px solid #ffaa00; }
        QSlider::groove:horizontal { border: 1px solid #7f00ff; height: 6px; background: #221133; }
        QSlider::handle:horizontal { background: #ff007f; width: 14px; margin: -4px 0; border-radius: 7px; }
        QTextEdit { background-color: #08080a; border: 1px solid #440088; color: #ffaa00; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);


    QWidget* canvasPlaceholder = new QWidget();
    canvasPlaceholder->setMinimumHeight(220);
    canvasPlaceholder->setAttribute(Qt::WA_TransparentForMouseEvents);
    mainLayout->addWidget(canvasPlaceholder);


    QHBoxLayout* controlsRow = new QHBoxLayout();


    QGroupBox* oscGroup = new QGroupBox("Wavetable Core (Procedural)");
    QFormLayout* oscLayout = new QFormLayout(oscGroup);
    
    sldWtPos = new QSlider(Qt::Horizontal);
    sldWtPos->setRange(0, 300); // 0.0 to 3.0
    sldWtPos->setValue(0);
    lblWtPos = new QLabel("WT Pos: 0.00");

    cmbWarpMode = new QComboBox();
    cmbWarpMode->addItems({"None (Linear Phase)", "Bend (+/- Curve)", "Sync (Windowed Multiplier)", "Squeeze (Push Center)"});
    
    sldWarpAmt = new QSlider(Qt::Horizontal);
    sldWarpAmt->setRange(0, 100);
    sldWarpAmt->setValue(0);
    lblWarpAmt = new QLabel("Warp: 0.00");

    oscLayout->addRow(lblWtPos, sldWtPos);
    oscLayout->addRow("Warp Mode:", cmbWarpMode);
    oscLayout->addRow(lblWarpAmt, sldWarpAmt);
    controlsRow->addWidget(oscGroup);


    QGroupBox* modGroup = new QGroupBox("WT Pos Modulation");
    QFormLayout* modLayout = new QFormLayout(modGroup);

    cmbModSource = new QComboBox();
    cmbModSource->addItems({"None", "LFO (Sine)", "Envelope (Pluck)", "Envelope (Sweep Up)"});
    
    sldModDepth = new QSlider(Qt::Horizontal);
    sldModDepth->setRange(-100, 100);
    sldModDepth->setValue(50);
    
    sldModRate = new QSlider(Qt::Horizontal);
    sldModRate->setRange(1, 100); // 0.1Hz to 10Hz or Decay multiplier
    sldModRate->setValue(20);

    modLayout->addRow("Source:", cmbModSource);
    modLayout->addRow("Depth:", sldModDepth);
    modLayout->addRow("Rate/Time:", sldModRate);
    controlsRow->addWidget(modGroup);


    QGroupBox* uniGroup = new QGroupBox("Hyper-Dimension");
    QFormLayout* uniLayout = new QFormLayout(uniGroup);
    
    spinUnisonVoices = new QSpinBox();
    spinUnisonVoices->setRange(1, 3);
    spinUnisonVoices->setValue(1);
    
    sldUnisonDetune = new QSlider(Qt::Horizontal);
    sldUnisonDetune->setRange(0, 50); // 0.0 to 0.05 multiplier
    sldUnisonDetune->setValue(10);
    
    uniLayout->addRow("Voices:", spinUnisonVoices);
    uniLayout->addRow("Detune Spread:", sldUnisonDetune);
    controlsRow->addWidget(uniGroup);

    mainLayout->addLayout(controlsRow);


    QHBoxLayout* outRow = new QHBoxLayout();
    btnGenerate = new QPushButton("⚡ Compile to Xpressive Math");
    btnPlay = new QPushButton("▶ Audition Wavetable");
    btnPlay->setCheckable(true);

    outRow->addWidget(btnGenerate);
    outRow->addWidget(btnPlay);
    mainLayout->addLayout(outRow);

    txtOutput = new QTextEdit();
    txtOutput->setReadOnly(true);
    txtOutput->setMinimumHeight(120);
    mainLayout->addWidget(txtOutput);


    connect(sldWtPos, &QSlider::valueChanged, this, &WavetableTab::onParametersChanged);
    connect(sldWarpAmt, &QSlider::valueChanged, this, &WavetableTab::onParametersChanged);
    connect(cmbWarpMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WavetableTab::onParametersChanged);
    
    connect(btnGenerate, &QPushButton::clicked, this, &WavetableTab::onGenerate);
    connect(btnPlay, &QPushButton::toggled, this, &WavetableTab::togglePlay);
}


double WavetableTab::calculateWaveform(double phase, double wtPos, int warpMode, double warpAmt) {

    double p = std::fmod(phase, 1.0);
    if (p < 0) p += 1.0;

    if (warpMode == 1) { // Bend
        double powAmt = (warpAmt < 0) ? (1.0 / (1.0 - warpAmt)) : (1.0 + warpAmt * 4.0);
        p = std::pow(p, powAmt);
    } else if (warpMode == 2) { // Sync
        p = std::fmod(p * (1.0 + warpAmt * 3.0), 1.0);
    } else if (warpMode == 3) { // Squeeze
        if (p < 0.5) p = std::pow(p * 2.0, 1.0 + warpAmt * 3.0) * 0.5;
        else p = 1.0 - (std::pow((1.0 - p) * 2.0, 1.0 + warpAmt * 3.0) * 0.5);
    }


    double w0 = std::sin(p * 2.0 * M_PI);               // Sine
    double w1 = (p * 2.0) - 1.0;                        // Saw
    double w2 = (p < 0.5) ? 1.0 : -1.0;                 // Square
    double w3 = std::sin(p * 2.0 * M_PI) * std::cos(p * 8.0 * M_PI); // Complex Harmonic
    

    double wt = std::clamp(wtPos, 0.0, 3.0);
    if (wt < 1.0) return w0 * (1.0 - wt) + w1 * wt;
    if (wt < 2.0) return w1 * (2.0 - wt) + w2 * (wt - 1.0);
    return w2 * (3.0 - wt) + w3 * (wt - 2.0);
}

void WavetableTab::onParametersChanged() {
    lblWtPos->setText(QString("WT Pos: %1").arg(sldWtPos->value() / 100.0, 0, 'f', 2));
    

    double wVal = sldWarpAmt->value() / 100.0;
    if (cmbWarpMode->currentIndex() == 1) { 
        wVal = (sldWarpAmt->value() - 50) / 50.0; // Bend goes -1 to 1
    }
    lblWarpAmt->setText(QString("Warp: %1").arg(wVal, 0, 'f', 2));
    
    update();
}

void WavetableTab::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect canvas = QRect(20, 20, width() - 40, 200);
    painter.fillRect(canvas, QColor(8, 8, 12));

    int frames = 24;
    int res = 200;
    
    double activeWtPos = sldWtPos->value() / 100.0;
    int warpMode = cmbWarpMode->currentIndex();
    double warpAmt = sldWarpAmt->value() / 100.0;
    if (warpMode == 1) warpAmt = (sldWarpAmt->value() - 50) / 50.0;


    for (int i = frames; i >= 0; --i) {
        double frameWtPos = (double)i / frames * 3.0;
        
        bool isActiveFrame = (std::abs(frameWtPos - activeWtPos) < (3.0 / frames));
        
        float zOffset_Y = i * 4.0f; 
        float zOffset_X = i * -3.0f;
        
        QPainterPath path;
        for (int x = 0; x <= res; ++x) {
            double phase = (double)x / res;
            double sample = calculateWaveform(phase, frameWtPos, warpMode, warpAmt);
            
            float px = canvas.right() - 50 + zOffset_X - (x * (canvas.width() - 100.0f) / res);
            float py = canvas.center().y() - 20 - zOffset_Y - (sample * 35.0f);
            
            if (x == 0) path.moveTo(px, py);
            else path.lineTo(px, py);
        }
        
        if (isActiveFrame) {
            painter.setPen(QPen(QColor(255, 0, 127), 3));
            painter.setBrush(QColor(255, 0, 127, 40));
        } else {
            painter.setPen(QPen(QColor(127, 0, 255, 100), 1));
            painter.setBrush(QColor(20, 10, 40, 150));
        }
        

        QPainterPath fillPath = path;
        fillPath.lineTo(canvas.left() - 50 + zOffset_X, canvas.bottom() - zOffset_Y);
        fillPath.lineTo(canvas.right() - 50 + zOffset_X, canvas.bottom() - zOffset_Y);
        painter.drawPath(fillPath);
        painter.drawPath(path);
    }
}

void WavetableTab::onGenerate() {
    double wtBase = sldWtPos->value() / 100.0;
    int warpMode = cmbWarpMode->currentIndex();
    double warpAmt = sldWarpAmt->value() / 100.0;
    if (warpMode == 1) warpAmt = (sldWarpAmt->value() - 50) / 50.0;
    
    int modSource = cmbModSource->currentIndex();
    double modDepth = sldModDepth->value() / 100.0;
    double modRate = sldModRate->value() / 10.0;

    int voices = spinUnisonVoices->value();
    double detune = sldUnisonDetune->value() / 1000.0;

    QString outCode = "// --- PROCEDURAL WAVETABLE ENGINE (Mega WT) ---\n";
    

    outCode += QString("var wt_base := %1;\n").arg(wtBase, 0, 'f', 2);
    if (modSource == 1) { // LFO
        outCode += QString("var mod_sig := sin(t * 6.28318 * %1);\n").arg(modRate);
    } else if (modSource == 2) { // Pluck Env
        outCode += QString("var mod_sig := exp(-t * %1);\n").arg(modRate * 2.0);
    } else if (modSource == 3) { // Sweep up
        outCode += QString("var mod_sig := 1.0 - exp(-t * %1);\n").arg(modRate * 2.0);
    } else {
        outCode += "var mod_sig := 0.0;\n";
    }
    
    outCode += QString("var wt_pos := clamp(0.0, wt_base + (mod_sig * %1), 3.0);\n\n").arg(modDepth * 3.0, 0, 'f', 2);


    auto buildOsc = [&](const QString& freqVar) -> QString {
        QString block = QString("  var p := mod(integrate(%1), 1.0);\n").arg(freqVar);
        

        if (warpMode == 1) { // Bend
            double powAmt = (warpAmt < 0) ? (1.0 / (1.0 - warpAmt)) : (1.0 + warpAmt * 4.0);
            block += QString("  p := pow(p, %1);\n").arg(powAmt, 0, 'f', 3);
        } else if (warpMode == 2) { // Sync
            block += QString("  p := mod(p * %1, 1.0);\n").arg(1.0 + warpAmt * 3.0, 0, 'f', 3);
        } else if (warpMode == 3) { // Squeeze
            block += QString("  p := (p < 0.5) ? (pow(p*2.0, %1)*0.5) : (1.0 - pow((1.0-p)*2.0, %1)*0.5);\n").arg(1.0 + warpAmt * 3.0, 0, 'f', 3);
        }


        block += "  var w0 := sin(p * 6.28318);\n";
        block += "  var w1 := (p * 2.0) - 1.0;\n";
        block += "  var w2 := (p < 0.5) ? 1.0 : -1.0;\n";
        block += "  var w3 := sin(p * 6.28318) * cos(p * 25.1327);\n";


        block += "  var out := (wt_pos < 1.0) ? (w0*(1.0-wt_pos) + w1*wt_pos) :\n"
                 "             (wt_pos < 2.0) ? (w1*(2.0-wt_pos) + w2*(wt_pos-1.0)) :\n"
                 "                              (w2*(3.0-wt_pos) + w3*(wt_pos-2.0));\n";
        return block;
    };


    if (voices == 1) {
        outCode += "var osc1 := {\n" + buildOsc("f") + "  out;\n};\n";
        outCode += "clamp(-1.0, osc1, 1.0);\n";
    } else if (voices == 2) {
        outCode += "var oscL := {\n" + buildOsc(QString("f * %1").arg(1.0 - detune)) + "  out;\n};\n";
        outCode += "var oscR := {\n" + buildOsc(QString("f * %1").arg(1.0 + detune)) + "  out;\n};\n";
        outCode += "clamp(-1.0, (oscL + oscR) * 0.7, 1.0);\n";
    } else { // 3 Voices (Center, L, R)
        outCode += "var oscC := {\n" + buildOsc("f") + "  out;\n};\n";
        outCode += "var oscL := {\n" + buildOsc(QString("f * %1").arg(1.0 - detune)) + "  out;\n};\n";
        outCode += "var oscR := {\n" + buildOsc(QString("f * %1").arg(1.0 + detune)) + "  out;\n};\n";
        outCode += "clamp(-1.0, (oscC + oscL + oscR) * 0.5, 1.0);\n";
    }

    txtOutput->setText(outCode);
    QApplication::clipboard()->setText(outCode);
    
    if (btnPlay->isChecked()) {
        togglePlay(true); // Restart audio engine with new math
    }
}

void WavetableTab::togglePlay(bool checked) {
    if (!m_ghostSynth) return;

    if (checked) {
        btnPlay->setText("⏹ Stop");
        btnPlay->setStyleSheet("background-color: #aa0000; color: white;");


        double wtBase = sldWtPos->value() / 100.0;
        int warpMode = cmbWarpMode->currentIndex();
        double warpAmt = sldWarpAmt->value() / 100.0;
        if (warpMode == 1) warpAmt = (sldWarpAmt->value() - 50) / 50.0;
        
        int modSource = cmbModSource->currentIndex();
        double modDepth = sldModDepth->value() / 100.0;
        double modRate = sldModRate->value() / 10.0;
        int voices = spinUnisonVoices->value();
        double detune = sldUnisonDetune->value() / 1000.0;

        auto audioAlgo = [=](double t) mutable -> double {

            double modSig = 0.0;
            if (modSource == 1) modSig = std::sin(t * 2.0 * M_PI * modRate);
            else if (modSource == 2) modSig = std::exp(-t * modRate * 2.0);
            else if (modSource == 3) modSig = 1.0 - std::exp(-t * modRate * 2.0);
            
            double wtPos = std::clamp(wtBase + (modSig * modDepth * 3.0), 0.0, 3.0);
            double baseF = 110.0;

            double out = 0.0;
            if (voices == 1) {
                out = calculateWaveform(t * baseF, wtPos, warpMode, warpAmt);
            } else if (voices == 2) {
                out += calculateWaveform(t * baseF * (1.0 - detune), wtPos, warpMode, warpAmt);
                out += calculateWaveform(t * baseF * (1.0 + detune), wtPos, warpMode, warpAmt);
                out *= 0.7;
            } else {
                out += calculateWaveform(t * baseF, wtPos, warpMode, warpAmt);
                out += calculateWaveform(t * baseF * (1.0 - detune), wtPos, warpMode, warpAmt);
                out += calculateWaveform(t * baseF * (1.0 + detune), wtPos, warpMode, warpAmt);
                out *= 0.5;
            }
            return std::clamp(out, -1.0, 1.0);
        };
        
        m_ghostSynth->setAudioSource(audioAlgo);
        m_ghostSynth->start();

    } else {
        btnPlay->setText("▶ Audition Wavetable");
        btnPlay->setStyleSheet("");
        m_ghostSynth->stop();
    }
}
