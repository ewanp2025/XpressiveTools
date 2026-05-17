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
#include <QLinearGradient>
#include <QEvent>
#include <QDialog>
#include <QTextEdit>
#include <QCheckBox>
#include <cmath>
#include <algorithm>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WavetableTab::WavetableTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth)
{
    setupUI();
    onParametersChanged();
}

WavetableTab::~WavetableTab() {}

void WavetableTab::setupUI() {
    this->setStyleSheet(R"(
        QWidget { background-color: #121218; color: #00ffff; font-family: "Consolas", monospace; }
        QGroupBox { border: 1px solid #00aaaa; margin-top: 5px; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; color: #00ffff; }
        QPushButton { background-color: #004444; border: 1px solid #00ffff; padding: 8px; font-weight: bold; }
        QPushButton:hover { background-color: #008888; color: #ffffff; }
        QPushButton:checked { background-color: #ff007f; color: white; border: 1px solid #ffffff; }
        QSlider::groove:horizontal { border: 1px solid #00aaaa; height: 6px; background: #112222; }
        QSlider::handle:horizontal { background: #00ffff; width: 14px; margin: -4px 0; border-radius: 7px; }
        QComboBox { background-color: #112222; border: 1px solid #00aaaa; color: #00ffff; padding: 2px; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    canvasPlaceholder = new QWidget(this);
    canvasPlaceholder->setMinimumHeight(220);
    canvasPlaceholder->installEventFilter(this);
    mainLayout->addWidget(canvasPlaceholder);

    QHBoxLayout* controlsRow = new QHBoxLayout();


    QGroupBox* oscGroup = new QGroupBox("Wavetable Core");
    QFormLayout* oscLayout = new QFormLayout(oscGroup);

    cmbWtBank = new QComboBox();
    cmbWtBank->addItems({"Basic Shapes", "Harmonic Sweeps", "Analog PWM"});

    sldWtPos = new QSlider(Qt::Horizontal);
    sldWtPos->setRange(0, 300); // 0.0 to 3.0
    sldWtPos->setValue(0);
    lblWtPos = new QLabel("WT Pos: 0.00");

    cmbWarpMode = new QComboBox();
    cmbWarpMode->addItems({"None (Linear Phase)", "Bend (+/- Curve)", "Sync (Windowed)", "Squeeze (Push Center)", "Quantize (Steps)", "Mirror (Fold)"});

    sldWarpAmt = new QSlider(Qt::Horizontal);
    sldWarpAmt->setRange(0, 100);
    sldWarpAmt->setValue(0);
    lblWarpAmt = new QLabel("Warp: 0.00");

    cmbInterpolation = new QComboBox();
    cmbInterpolation->addItems({"Linear (Classic)", "Cosine (Smooth)", "Step (Lo-Fi)"});

    oscLayout->addRow("Table Bank:", cmbWtBank);
    oscLayout->addRow(lblWtPos, sldWtPos);
    oscLayout->addRow("Interpolation:", cmbInterpolation);
    oscLayout->addRow("Warp Mode:", cmbWarpMode);
    oscLayout->addRow(lblWarpAmt, sldWarpAmt);
    controlsRow->addWidget(oscGroup);


    QVBoxLayout* midColLayout = new QVBoxLayout();

    QGroupBox* modGroup = new QGroupBox("WT Modulation");
    QFormLayout* modLayout = new QFormLayout(modGroup);
    cmbModSource = new QComboBox();
    cmbModSource->addItems({"None", "LFO (Sine)", "Envelope (Pluck)"});
    sldModDepth = new QSlider(Qt::Horizontal); sldModDepth->setRange(-100, 100); sldModDepth->setValue(50);
    sldModRate = new QSlider(Qt::Horizontal); sldModRate->setRange(1, 100); sldModRate->setValue(20);
    modLayout->addRow("Source:", cmbModSource);
    modLayout->addRow("Depth:", sldModDepth);
    modLayout->addRow("Rate:", sldModRate);
    midColLayout->addWidget(modGroup);

    QGroupBox* subGroup = new QGroupBox("Sub Oscillator");
    QFormLayout* subLayout = new QFormLayout(subGroup);
    chkSubOsc = new QCheckBox("Enable Sub");
    cmbSubShape = new QComboBox(); cmbSubShape->addItems({"Sine", "Triangle", "Square"});
    cmbSubOctave = new QComboBox(); cmbSubOctave->addItems({"-1 Octave", "-2 Octaves", "-3 Octaves"});
    sldSubVol = new QSlider(Qt::Horizontal); sldSubVol->setRange(0, 100); sldSubVol->setValue(50);
    subLayout->addRow(chkSubOsc);
    subLayout->addRow("Shape:", cmbSubShape);
    subLayout->addRow("Octave:", cmbSubOctave);
    subLayout->addRow("Volume:", sldSubVol);
    midColLayout->addWidget(subGroup);

    controlsRow->addLayout(midColLayout);


    QGroupBox* uniGroup = new QGroupBox("Hyper-Dimension");
    QFormLayout* uniLayout = new QFormLayout(uniGroup);

    spinUnisonVoices = new QSpinBox();
    spinUnisonVoices->setRange(1, 3);
    spinUnisonVoices->setValue(1);

    sldUnisonDetune = new QSlider(Qt::Horizontal);
    sldUnisonDetune->setRange(0, 50);
    sldUnisonDetune->setValue(10);

    sldUnisonWidth = new QSlider(Qt::Horizontal);
    sldUnisonWidth->setRange(0, 100);
    sldUnisonWidth->setValue(100);

    uniLayout->addRow("Voices:", spinUnisonVoices);
    uniLayout->addRow("Detune:", sldUnisonDetune);
    uniLayout->addRow("Stereo Width:", sldUnisonWidth);
    controlsRow->addWidget(uniGroup);

    mainLayout->addLayout(controlsRow);


    QHBoxLayout* outRow = new QHBoxLayout();
    cmbParserMode = new QComboBox();
    cmbParserMode->addItems({"Nightly (O1 Variables)", "Legacy (Inline Math)"});
    btnGenerate = new QPushButton("⚡ Compile Math");
    btnPlay = new QPushButton("▶ Audition Wavetable");
    btnPlay->setCheckable(true);

    outRow->addWidget(new QLabel("Parser Mode:"));
    outRow->addWidget(cmbParserMode);
    outRow->addWidget(btnGenerate);
    outRow->addWidget(btnPlay);
    mainLayout->addLayout(outRow);


    connect(cmbWtBank, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WavetableTab::onParametersChanged);
    connect(sldWtPos, &QSlider::valueChanged, this, &WavetableTab::onParametersChanged);
    connect(sldWarpAmt, &QSlider::valueChanged, this, &WavetableTab::onParametersChanged);
    connect(cmbWarpMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WavetableTab::onParametersChanged);
    connect(cmbInterpolation, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WavetableTab::onParametersChanged);
    connect(btnGenerate, &QPushButton::clicked, this, &WavetableTab::onGenerate);
    connect(btnPlay, &QPushButton::toggled, this, &WavetableTab::togglePlay);
}

double WavetableTab::calculateWaveform(double phase, double wtPos, int warpMode, double warpAmt, int interpMode, int wtBank) {
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
    } else if (warpMode == 4) { // Quantize (Stepped Phase)
        double steps = std::max(2.0, 32.0 * (1.0 - warpAmt));
        p = std::floor(p * steps) / steps;
    } else if (warpMode == 5) { // Mirror
        double foldAmt = warpAmt * 2.0;
        p = std::fmod(p * (1.0 + foldAmt), 1.0);
        if (p > 0.5) p = 1.0 - p;
        p *= 2.0;
    }

    double w0, w1, w2, w3;


    if (wtBank == 0) {

        w0 = std::sin(p * 2.0 * M_PI);
        w1 = (p * 2.0) - 1.0;
        w2 = (p < 0.5) ? 1.0 : -1.0;
        w3 = std::sin(p * 2.0 * M_PI) * std::cos(p * 8.0 * M_PI);
    } else if (wtBank == 1) {

        w0 = std::sin(p * 2.0 * M_PI);
        w1 = (std::sin(p * 2.0 * M_PI) + 0.5 * std::sin(p * 6.0 * M_PI)) * 0.66;
        w2 = (std::sin(p * 2.0 * M_PI) + 0.5 * std::sin(p * 6.0 * M_PI) + 0.25 * std::sin(p * 10.0 * M_PI)) * 0.57;
        w3 = 0; for(int i=1; i<=11; i+=2) w3 += (1.0/i) * std::sin(p * 2.0 * M_PI * i); w3 *= 0.5;
    } else {

        w0 = std::sin(p * 2.0 * M_PI);
        w1 = (p < 0.5) ? 1.0 : -1.0; // 50% square
        w2 = (p < 0.25) ? 1.0 : -1.0; // 25% pulse
        w3 = (p < 0.1) ? 1.0 : -1.0; // 10% pulse
    }


    double wt = std::clamp(wtPos, 0.0, 3.0);
    int wt_idx = std::floor(wt);
    double wt_frac = wt - wt_idx;

    if (interpMode == 1) {
        wt_frac = 0.5 - 0.5 * std::cos(wt_frac * M_PI);
    } else if (interpMode == 2) {
        wt_frac = (wt_frac >= 0.5) ? 1.0 : 0.0;
    }

    if (wt_idx == 0) return w0 * (1.0 - wt_frac) + w1 * wt_frac;
    if (wt_idx == 1) return w1 * (1.0 - wt_frac) + w2 * wt_frac;
    if (wt_idx == 2) return w2 * (1.0 - wt_frac) + w3 * wt_frac;
    return w3;
}

void WavetableTab::onParametersChanged() {
    lblWtPos->setText(QString("WT Pos: %1").arg(sldWtPos->value() / 100.0, 0, 'f', 2));

    double wVal = sldWarpAmt->value() / 100.0;
    if (cmbWarpMode->currentIndex() == 1 || cmbWarpMode->currentIndex() == 4) {
        wVal = (sldWarpAmt->value() - 50) / 50.0;
    }
    lblWarpAmt->setText(QString("Warp: %1").arg(wVal, 0, 'f', 2));

    canvasPlaceholder->update();
}

bool WavetableTab::eventFilter(QObject *obj, QEvent *event) {
    if (obj == canvasPlaceholder && event->type() == QEvent::Paint) {
        QPainter painter(canvasPlaceholder);
        painter.setRenderHint(QPainter::Antialiasing);

        QRect canvas = canvasPlaceholder->rect();
        painter.fillRect(canvas, QColor(8, 8, 12));

        int frames = 24;
        int res = 250;

        double activeWtPos = sldWtPos->value() / 100.0;
        int warpMode = cmbWarpMode->currentIndex();
        int interpMode = cmbInterpolation->currentIndex();
        int wtBank = cmbWtBank->currentIndex();
        double warpAmt = sldWarpAmt->value() / 100.0;
        if (warpMode == 1 || warpMode == 4) warpAmt = (sldWarpAmt->value() - 50) / 50.0;

        for (int i = frames; i >= 0; --i) {
            double frameWtPos = (double)i / frames * 3.0;
            bool isActiveFrame = (std::abs(frameWtPos - activeWtPos) < (3.0 / frames));

            float zOffset_Y = i * 4.5f;
            float zOffset_X = i * -3.5f;

            QPainterPath path;
            for (int x = 0; x <= res; ++x) {
                double phase = (double)x / res;
                double sample = calculateWaveform(phase, frameWtPos, warpMode, warpAmt, interpMode, wtBank);

                float px = canvas.right() - 60 + zOffset_X - (x * (canvas.width() - 120.0f) / res);
                float py = canvas.center().y() - 10 - zOffset_Y - (sample * 40.0f);

                if (x == 0) path.moveTo(px, py);
                else path.lineTo(px, py);
            }

            QPainterPath fillPath = path;
            fillPath.lineTo(canvas.left() - 60 + zOffset_X, canvas.bottom() - zOffset_Y + 20);
            fillPath.lineTo(canvas.right() - 60 + zOffset_X, canvas.bottom() - zOffset_Y + 20);

            if (isActiveFrame) {
                painter.setPen(QPen(QColor(0, 255, 255), 3));
                QLinearGradient grad(0, canvas.top(), 0, canvas.bottom());
                grad.setColorAt(0, QColor(0, 255, 255, 80));
                grad.setColorAt(1, QColor(0, 255, 255, 10));
                painter.setBrush(grad);
            } else {
                painter.setPen(QPen(QColor(0, 170, 170, 100), 1));
                painter.setBrush(QColor(10, 20, 20, 200));
            }

            painter.drawPath(fillPath);
            painter.drawPath(path);
        }
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void WavetableTab::onGenerate() {
    bool isLegacy = (cmbParserMode->currentIndex() == 1);

    double wtBase = sldWtPos->value() / 100.0;
    int warpMode = cmbWarpMode->currentIndex();
    int interpMode = cmbInterpolation->currentIndex();
    int wtBank = cmbWtBank->currentIndex();
    double warpAmt = sldWarpAmt->value() / 100.0;
    if (warpMode == 1 || warpMode == 4) warpAmt = (sldWarpAmt->value() - 50) / 50.0;

    int modSource = cmbModSource->currentIndex();
    double modDepth = sldModDepth->value() / 100.0;
    double modRate = sldModRate->value() / 10.0;

    int voices = spinUnisonVoices->value();
    double detune = sldUnisonDetune->value() / 1000.0;
    double width = sldUnisonWidth->value() / 100.0;

    bool hasSub = chkSubOsc->isChecked();
    int subShape = cmbSubShape->currentIndex();
    double subVol = sldSubVol->value() / 100.0;
    double subMult = (cmbSubOctave->currentIndex() == 0) ? 0.5 : (cmbSubOctave->currentIndex() == 1) ? 0.25 : 0.125;

    QString outCode = "";

    if (isLegacy) {
        outCode += "// --- PROCEDURAL WAVETABLE (LEGACY INLINE MODE) ---\n";
        outCode += "// Note: Legacy mode does not support full features.\n";
        outCode += "sin(mod(integrate(f), 1.0) * 6.28318)";
    } else {
        outCode += "// --- PROCEDURAL WAVETABLE ENGINE (Mega WT - Nightly) ---\n";

        outCode += QString("var wt_base := %1;\n").arg(wtBase, 0, 'f', 2);
        if (modSource == 1) outCode += QString("var mod_sig := sin(t * 6.28318 * %1);\n").arg(modRate);
        else if (modSource == 2) outCode += QString("var mod_sig := exp(-t * %1);\n").arg(modRate * 2.0);
        else outCode += "var mod_sig := 0.0;\n";

        outCode += QString("var wt_pos := clamp(0.0, wt_base + (mod_sig * %1), 3.0);\n\n").arg(modDepth * 3.0, 0, 'f', 2);

        auto buildOsc = [&](const QString& freqVar) -> QString {
            QString block = QString("  var p := mod(integrate(%1), 1.0);\n").arg(freqVar);

            if (warpMode == 1) {
                double powAmt = (warpAmt < 0) ? (1.0 / (1.0 - warpAmt)) : (1.0 + warpAmt * 4.0);
                block += QString("  p := pow(p, %1);\n").arg(powAmt, 0, 'f', 3);
            } else if (warpMode == 2) {
                block += QString("  p := mod(p * %1, 1.0);\n").arg(1.0 + warpAmt * 3.0, 0, 'f', 3);
            } else if (warpMode == 3) {
                block += QString("  p := (p < 0.5) ? (pow(p*2.0, %1)*0.5) : (1.0 - pow((1.0-p)*2.0, %1)*0.5);\n").arg(1.0 + warpAmt * 3.0, 0, 'f', 3);
            } else if (warpMode == 4) {
                double steps = std::max(2.0, 32.0 * (1.0 - warpAmt));
                block += QString("  p := floor(p * %1) / %1;\n").arg(steps, 0, 'f', 2);
            } else if (warpMode == 5) {
                double foldAmt = warpAmt * 2.0;
                block += QString("  p := mod(p * %1, 1.0);\n  p := (p > 0.5) ? (1.0 - p) * 2.0 : p * 2.0;\n").arg(1.0 + foldAmt, 0, 'f', 3);
            }

            if (wtBank == 0) {
                block += "  var w0 := sin(p * 6.28318);\n";
                block += "  var w1 := (p * 2.0) - 1.0;\n";
                block += "  var w2 := (p < 0.5) ? 1.0 : -1.0;\n";
                block += "  var w3 := sin(p * 6.28318) * cos(p * 25.1327);\n";
            } else if (wtBank == 1) {
                block += "  var w0 := sin(p * 6.28318);\n";
                block += "  var w1 := (sin(p * 6.28318) + 0.5*sin(p * 18.849)) * 0.66;\n";
                block += "  var w2 := (sin(p * 6.28318) + 0.5*sin(p * 18.849) + 0.25*sin(p * 31.415)) * 0.57;\n";
                block += "  var w3 := w2; // Approx odd harmonics\n";
            } else {
                block += "  var w0 := sin(p * 6.28318);\n";
                block += "  var w1 := (p < 0.5) ? 1.0 : -1.0;\n";
                block += "  var w2 := (p < 0.25) ? 1.0 : -1.0;\n";
                block += "  var w3 := (p < 0.1) ? 1.0 : -1.0;\n";
            }

            block += "  var wt_frac := fract(wt_pos);\n";
            if (interpMode == 1) block += "  wt_frac := 0.5 - 0.5 * cos(wt_frac * 3.14159);\n";
            else if (interpMode == 2) block += "  wt_frac := (wt_frac >= 0.5) ? 1.0 : 0.0;\n";

            block += "  var out := (wt_pos < 1.0) ? (w0*(1.0-wt_frac) + w1*wt_frac) :\n"
                     "             (wt_pos < 2.0) ? (w1*(1.0-wt_frac) + w2*wt_frac) :\n"
                     "                              (w2*(1.0-wt_frac) + w3*wt_frac);\n";
            return block;
        };

        QString subBlock = "";
        if (hasSub) {
            subBlock += QString("\n// --- SUB OSCILLATOR ---\nvar pSub := mod(integrate(f * %1), 1.0);\n").arg(subMult);
            if (subShape == 0) subBlock += "var oscSub := sin(pSub * 6.28318);\n";
            else if (subShape == 1) subBlock += "var oscSub := (pSub < 0.5) ? (pSub * 4.0 - 1.0) : (3.0 - pSub * 4.0);\n";
            else subBlock += "var oscSub := (pSub < 0.5) ? 1.0 : -1.0;\n";
            subBlock += QString("oscSub := oscSub * %1;\n\n").arg(subVol, 0, 'f', 2);
        } else {
            subBlock += "var oscSub := 0.0;\n";
        }

        if (voices == 1) {
            outCode += "var oscC := {\n" + buildOsc("f") + "  out;\n};\n";
            outCode += subBlock;
            outCode += "var output := oscC + oscSub;\nclamp(-1.0, output, 1.0);\n";
        } else if (voices == 2) {
            outCode += "var oscL := {\n" + buildOsc(QString("f * %1").arg(1.0 - detune)) + "  out;\n};\n";
            outCode += "var oscR := {\n" + buildOsc(QString("f * %1").arg(1.0 + detune)) + "  out;\n};\n";
            outCode += subBlock;
            // Apply width logic for 2 voices
            outCode += QString("var mixL := (oscL * %1) + (oscR * %2);\n").arg(0.5 + width/2.0).arg(0.5 - width/2.0);
            outCode += QString("var mixR := (oscR * %1) + (oscL * %2);\n").arg(0.5 + width/2.0).arg(0.5 - width/2.0);
            outCode += "var output := (mixL + mixR) * 0.7 + oscSub;\nclamp(-1.0, output, 1.0);\n";
        } else {
            outCode += "var oscC := {\n" + buildOsc("f") + "  out;\n};\n";
            outCode += "var oscL := {\n" + buildOsc(QString("f * %1").arg(1.0 - detune)) + "  out;\n};\n";
            outCode += "var oscR := {\n" + buildOsc(QString("f * %1").arg(1.0 + detune)) + "  out;\n};\n";
            outCode += subBlock;
            outCode += QString("var mixL := (oscL * %1) + (oscR * %2);\n").arg(0.5 + width/2.0).arg(0.5 - width/2.0);
            outCode += QString("var mixR := (oscR * %1) + (oscL * %2);\n").arg(0.5 + width/2.0).arg(0.5 - width/2.0);
            outCode += "var output := (oscC + mixL + mixR) * 0.5 + oscSub;\nclamp(-1.0, output, 1.0);\n";
        }
    }

    QApplication::clipboard()->setText(outCode);

    QDialog* outputDialog = new QDialog(this);
    outputDialog->setWindowTitle("Generated Xpressive Math");
    outputDialog->resize(600, 500);
    outputDialog->setStyleSheet("background-color: #121218; color: #00ffff; font-family: Consolas;");

    QVBoxLayout* dlgLayout = new QVBoxLayout(outputDialog);
    QTextEdit* txtDialogOut = new QTextEdit(outputDialog);
    txtDialogOut->setReadOnly(true);
    txtDialogOut->setStyleSheet("background-color: #08080a; border: 1px solid #00aaaa; padding: 5px;");
    txtDialogOut->setText(outCode);

    dlgLayout->addWidget(txtDialogOut);
    outputDialog->exec();

    if (btnPlay->isChecked()) togglePlay(true);
}

void WavetableTab::togglePlay(bool checked) {
    if (!m_ghostSynth) return;

    if (checked) {
        btnPlay->setText("⏹ Stop");
        btnPlay->setStyleSheet("background-color: #ff007f; color: white; border: 1px solid white;");

        double wtBase = sldWtPos->value() / 100.0;
        int warpMode = cmbWarpMode->currentIndex();
        int interpMode = cmbInterpolation->currentIndex();
        int wtBank = cmbWtBank->currentIndex();
        double warpAmt = sldWarpAmt->value() / 100.0;
        if (warpMode == 1 || warpMode == 4) warpAmt = (sldWarpAmt->value() - 50) / 50.0;

        int modSource = cmbModSource->currentIndex();
        double modDepth = sldModDepth->value() / 100.0;
        double modRate = sldModRate->value() / 10.0;
        int voices = spinUnisonVoices->value();
        double detune = sldUnisonDetune->value() / 1000.0;

        bool hasSub = chkSubOsc->isChecked();
        int subShape = cmbSubShape->currentIndex();
        double subVol = sldSubVol->value() / 100.0;
        double subMult = (cmbSubOctave->currentIndex() == 0) ? 0.5 : (cmbSubOctave->currentIndex() == 1) ? 0.25 : 0.125;

        auto audioAlgo = [=](double t) mutable -> double {
            double modSig = 0.0;
            if (modSource == 1) modSig = std::sin(t * 2.0 * M_PI * modRate);
            else if (modSource == 2) modSig = std::exp(-t * modRate * 2.0);

            double wtPos = std::clamp(wtBase + (modSig * modDepth * 3.0), 0.0, 3.0);
            double baseF = 110.0;

            double subOut = 0.0;
            if (hasSub) {
                double pSub = std::fmod(t * baseF * subMult, 1.0);
                if (subShape == 0) subOut = std::sin(pSub * 2.0 * M_PI);
                else if (subShape == 1) subOut = (pSub < 0.5) ? (pSub * 4.0 - 1.0) : (3.0 - pSub * 4.0);
                else subOut = (pSub < 0.5) ? 1.0 : -1.0;
                subOut *= subVol;
            }

            double out = 0.0;
            if (voices == 1) {
                out = calculateWaveform(t * baseF, wtPos, warpMode, warpAmt, interpMode, wtBank);
            } else if (voices == 2) {
                out += calculateWaveform(t * baseF * (1.0 - detune), wtPos, warpMode, warpAmt, interpMode, wtBank);
                out += calculateWaveform(t * baseF * (1.0 + detune), wtPos, warpMode, warpAmt, interpMode, wtBank);
                out *= 0.7;
            } else {
                out += calculateWaveform(t * baseF, wtPos, warpMode, warpAmt, interpMode, wtBank);
                out += calculateWaveform(t * baseF * (1.0 - detune), wtPos, warpMode, warpAmt, interpMode, wtBank);
                out += calculateWaveform(t * baseF * (1.0 + detune), wtPos, warpMode, warpAmt, interpMode, wtBank);
                out *= 0.5;
            }
            return std::clamp(out + subOut, -1.0, 1.0);
        };

        m_ghostSynth->setAudioSource(audioAlgo);
        m_ghostSynth->start();

    } else {
        btnPlay->setText("▶ Audition Wavetable");
        btnPlay->setStyleSheet("");
        m_ghostSynth->stop();
    }
}
