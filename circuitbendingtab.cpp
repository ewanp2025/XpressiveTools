#include "circuitbendingtab.h"
#include "synthengine.h"
#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <cmath>

CircuitBendingTab::CircuitBendingTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth)
{
    setupUI();
    generateExpression();
}

CircuitBendingTab::~CircuitBendingTab() {}

void CircuitBendingTab::setupUI() {
    // Styling to mimic a vintage green Printed Circuit Board (PCB)
    this->setStyleSheet(R"(
        QWidget { background-color: #0b220b; color: #ccffcc; font-family: "Consolas", monospace; }
        QGroupBox { border: 2px solid #b8860b; border-radius: 4px; margin-top: 12px; font-weight: bold; background-color: #0d2b0d; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; color: #ffd700; }
        QPushButton { background-color: #1a1a1a; border: 2px solid #b8860b; padding: 8px; color: #ffd700; font-weight: bold; }
        QPushButton:hover { background-color: #333333; color: #ffffff; }
        QPushButton:checked { background-color: #aa0000; color: #ffffff; border: 2px solid #ff0000; }
        QTextEdit { background-color: #050f05; border: 1px solid #b8860b; color: #00ff00; }
        QComboBox, QSlider { background-color: #111; color: #ccffcc; border: 1px solid #b8860b; }
        QSlider::groove:horizontal { border: 1px solid #b8860b; height: 6px; background: #000; }
        QSlider::handle:horizontal { background: #ffd700; width: 14px; margin: -4px 0; border-radius: 2px; }
        QCheckBox { spacing: 5px; }
        QCheckBox::indicator { width: 16px; height: 16px; border: 2px solid #b8860b; border-radius: 8px; background: #000; }
        QCheckBox::indicator:checked { background: #ff0000; border: 2px solid #ffaa00; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);


    m_scope = new UniversalScope();
    m_scope->setMinimumHeight(120);
    mainLayout->addWidget(m_scope);


    QGroupBox* grpSubstrate = new QGroupBox("Layer 1: The Substrate (Source Toy)");
    QHBoxLayout* subLayout = new QHBoxLayout(grpSubstrate);

    m_cmbBuildMode = new QComboBox();
    m_cmbBuildMode->addItems({"Nightly (ExprTk Variables)", "Legacy (Standard Math)"});
    subLayout->addWidget(new QLabel("Output Format:"));
    subLayout->addWidget(m_cmbBuildMode);
    connect(m_cmbBuildMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CircuitBendingTab::updateUI);




    m_cmbSubstrate = new QComboBox();
    m_cmbSubstrate->addItems({
        "Speak & Spell Emulator (Phonetic Formants)",
        "8-Bit Drum Machine (Decaying Transients)",
        "PCM ROMpler Chip (Lo-Fi Square/Saw Combo)"
    });
    subLayout->addWidget(new QLabel("Target Device:"));
    subLayout->addWidget(m_cmbSubstrate, 1);
    
    QLabel* lblExprTkNotice = new QLabel("️ Circuit Bending Requires ExprTk (Nightly) Parsing");
    lblExprTkNotice->setStyleSheet("color: #ffaa00; font-weight: bold; background: #331100; border: 1px solid #ff5500; padding: 4px;");
    subLayout->addWidget(lblExprTkNotice);
    mainLayout->addWidget(grpSubstrate);

    QHBoxLayout* midLayout = new QHBoxLayout();

    QGroupBox* grpMatrix = new QGroupBox("Layer 2: Virtual PCB Patch Bay");
    QGridLayout* matrixLayout = new QGridLayout(grpMatrix);
    

    for (int col = 0; col < 4; ++col) {
        QLabel* header = new QLabel(m_targetNames[col]);
        header->setAlignment(Qt::AlignCenter);
        header->setStyleSheet("color: #b8860b; font-size: 11px;");
        matrixLayout->addWidget(header, 0, col + 1);
    }

    for (int row = 0; row < 4; ++row) {
        QLabel* rowLabel = new QLabel(m_anomalyNames[row]);
        rowLabel->setStyleSheet("color: #b8860b; font-size: 11px;");
        matrixLayout->addWidget(rowLabel, row + 1, 0);

        for (int col = 0; col < 4; ++col) {
            m_patchMatrix[row][col] = new QCheckBox();
            m_patchMatrix[row][col]->setToolTip(QString("Route %1 to %2").arg(m_anomalyNames[row], m_targetNames[col]));
            
            // Center the checkbox in the grid cell
            QWidget* cbContainer = new QWidget();
            QHBoxLayout* cbLayout = new QHBoxLayout(cbContainer);
            cbLayout->setAlignment(Qt::AlignCenter);
            cbLayout->setContentsMargins(0,0,0,0);
            cbLayout->addWidget(m_patchMatrix[row][col]);
            
            matrixLayout->addWidget(cbContainer, row + 1, col + 1);
            connect(m_patchMatrix[row][col], &QCheckBox::toggled, this, &CircuitBendingTab::updateUI);
        }
    }
    midLayout->addWidget(grpMatrix, 2);

    QGroupBox* grpMacros = new QGroupBox("Layer 3: Hardware Interactors");
    QFormLayout* macroLayout = new QFormLayout(grpMacros);

    auto makeSlider = [&](QSlider*& sld, QLabel*& lbl, QString name, int min, int max, int def) {
        sld = new QSlider(Qt::Horizontal);
        sld->setRange(min, max);
        sld->setValue(def);
        lbl = new QLabel(QString("%1").arg(def));
        macroLayout->addRow(name, lbl);
        macroLayout->addRow(sld);
        connect(sld, &QSlider::valueChanged, this, &CircuitBendingTab::updateUI);
    };

    makeSlider(m_sldSagDepth, m_lblSag, "Voltage Sag / Drain:", 0, 100, 85);
    makeSlider(m_sldClockStarve, m_lblClock, "Clock Starve:", 0, 100, 10);
    makeSlider(m_sldRomGlitch, m_lblRom, "ROM Scramble Rate:", 1, 200, 50);
    makeSlider(m_sldBodyPressure, m_lblBody, "Body Contact Pressure:", 0, 100, 40);

    midLayout->addWidget(grpMacros, 1);
    mainLayout->addLayout(midLayout);


    QGroupBox* grpOutput = new QGroupBox("Compiled Xpressive Code");
    QVBoxLayout* outLayout = new QVBoxLayout(grpOutput);
    m_txtOutput = new QTextEdit();
    m_txtOutput->setReadOnly(true);
    outLayout->addWidget(m_txtOutput);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_btnGenerate = new QPushButton("Compile Circuit Logic");
    m_btnPlay = new QPushButton("▶ Audition Ghost Circuit");
    m_btnPlay->setCheckable(true);
    
    btnLayout->addWidget(m_btnPlay);
    btnLayout->addWidget(m_btnGenerate);
    outLayout->addLayout(btnLayout);

    mainLayout->addWidget(grpOutput);

    connect(m_btnGenerate, &QPushButton::clicked, this, &CircuitBendingTab::generateExpression);
    connect(m_btnPlay, &QPushButton::toggled, this, &CircuitBendingTab::togglePlay);
    connect(m_cmbSubstrate, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CircuitBendingTab::updateUI);
}

void CircuitBendingTab::updateUI() {
    m_lblSag->setText(QString("%1%").arg(m_sldSagDepth->value()));
    m_lblClock->setText(QString("%1%").arg(m_sldClockStarve->value()));
    m_lblRom->setText(QString("%1 Hz").arg(m_sldRomGlitch->value() / 10.0, 0, 'f', 1));
    m_lblBody->setText(QString("%1%").arg(m_sldBodyPressure->value()));

    generateExpression();
}

QString CircuitBendingTab::buildExprTkString() {
    bool isNightly = (m_cmbBuildMode->currentIndex() == 0);

    if (!isNightly) {

        return "(sin(integrate(f)) * (1.0 - (" + QString::number(m_sldSagDepth->value() / 100.0) + " * abs(sin(integrate(f))))))";
    }
    QString out = "// --- CIRCUIT BENDING SIMULATOR ---\n";
    out += "// Note: Paste this directly into O1 in LMMS 1.3 (ExprTk enabled)\n\n";


    out += QString("var sag_depth := %1;\n").arg(m_sldSagDepth->value() / 100.0, 0, 'f', 3);
    out += QString("var starve := %1;\n").arg(m_sldClockStarve->value() / 100.0, 0, 'f', 3);
    out += QString("var rom_rate := %1;\n").arg(m_sldRomGlitch->value() / 10.0, 0, 'f', 2);
    out += QString("var body_amt := %1;\n").arg(m_sldBodyPressure->value() / 100.0, 0, 'f', 3);
    out += "var f_sample := 8000;\n\n";


    bool sagToClock = m_patchMatrix[0][0]->isChecked();
    bool sagToAmp   = m_patchMatrix[0][2]->isChecked();
    
    bool cmosToClock = m_patchMatrix[1][0]->isChecked();
    bool cmosToOut   = m_patchMatrix[1][3]->isChecked();
    
    bool romToPitch = m_patchMatrix[2][1]->isChecked();
    bool romToOut   = m_patchMatrix[2][3]->isChecked();
    
    bool bodyToPitch = m_patchMatrix[3][1]->isChecked();
    bool bodyToAmp   = m_patchMatrix[3][2]->isChecked();

    QString pitchMod = "1.0";
    if (bodyToPitch) pitchMod += " + (body_amt * noise() * sin(t*15))";
    if (romToPitch)  pitchMod += " + (noise() > 0.9 ? randv(t*rom_rate) : 0.0)";
    out += "var pitch_mod := " + pitchMod + ";\n\n";



    int subType = m_cmbSubstrate->currentIndex();
    QString approxBase;
    if (subType == 0) approxBase = "sin(integrate(f)) * (sin(t*8) > 0 ? 1 : 0)"; // Speak & Spell pulse
    else if (subType == 1) approxBase = "sin(integrate(f * exp(-t*10))) * exp(-t*5)"; // 8-Bit Drum
    else approxBase = "saww(integrate(f))"; // ROMpler
    
    out += "// Envelope Follower (Power Supply Status)\n";
    out += "var base_approx := " + approxBase + ";\n";
    out += "var env := (0.01 * abs(base_approx)) + (0.99 * last(1));\n\n";


    out += "// Dynamic Clock Generation\n";
    QString clockRate = "(1.0 - starve)";
    if (sagToClock) clockRate += " - (sag_depth * env)";
    if (cmosToClock) {
        out += "var c_clk := floor(t * f_sample);\n";
        out += "var cmos_disrupt := ((c_clk * (shr(c_clk, 3) or shr(c_clk, 6))) % 256) / 256.0;\n";
        clockRate += " - (0.5 * cmos_disrupt)";
    }
    out += "var clock_rate := max(0.01, " + clockRate + ");\n";
    out += "var dyn_phase := integrate(clock_rate * f * pitch_mod);\n\n";


    out += "// The Foundational Toy Circuit\n";
    QString substrate;
    if (subType == 0) { // Speak & Spell
        substrate = "(sin(dyn_phase) + sin(dyn_phase*1.5))*0.5 * (sin(t*8)>0?1:0)";
    } else if (subType == 1) { // 8-Bit Drum
        substrate = "sin(dyn_phase * exp(-t*10)) * exp(-t*5)";
    } else { // ROMpler
        substrate = "(saww(dyn_phase) * 0.5 + squarew(dyn_phase * 0.5) * 0.5)";
    }
    out += "var core_sig := " + substrate + ";\n\n";


    QString ampMod = "1.0";
    if (sagToAmp) ampMod += " * (1.0 - (sag_depth * env))";
    if (bodyToAmp) ampMod += " * (1.0 - (body_amt * abs(noise())))";
    out += "var amp_sig := core_sig * " + ampMod + ";\n\n";

    QString finalOut = "amp_sig";


    if (cmosToOut) {
        out += "// CMOS Bytebeat Intermodulation\n";
        out += "var n := floor(dyn_phase * f_sample / f);\n";
        out += "var cmos_sig := (((n * (shr(n, 5) or shr(n, 8))) % 256) / 128.0) - 1.0;\n";
        finalOut = "(amp_sig * 0.5) + (cmos_sig * 0.5)";
    }


    if (romToOut) {
        out += "// ROM Aleatoric Scrambler (Incantor Matrix)\n";
        out += "var chaotic_index := abs(sin(2 * 3.1415 * rom_rate * integrate(clock_rate))) + (noise() * 0.4);\n";
        out += "var mem_address := floor(t * 14 * chaotic_index) % 4;\n";
        out += "var rom_sig := switch {\n";
        out += "  case mem_address < 1.0 : " + finalOut + ";\n";
        out += "  case mem_address < 2.0 : sin(dyn_phase * 1.5 + sin(dyn_phase * 1.5));\n"; // Feedback FM
        out += "  case mem_address < 3.0 : noise() * 0.5;\n"; // Memory dump static
        out += "  case mem_address < 4.0 : 0.0;\n"; // Hard crash
        out += "};\n";
        finalOut = "rom_sig";
    }


    out += "\n// Master VCA & Limiter\n";
    out += "clamp(-1.0, " + finalOut + ", 1.0);";

    return out;
}

void CircuitBendingTab::generateExpression() {
    QString code = buildExprTkString();
    m_txtOutput->setText(code);
    QApplication::clipboard()->setText(code);

    if (m_btnPlay->isChecked()) {
        togglePlay(true);
    }
}

void CircuitBendingTab::togglePlay(bool checked) {
    if (!m_ghostSynth) return;

    if (checked) {
        m_btnPlay->setText("⏹ Disconnect Power (Stop)");
        m_btnPlay->setStyleSheet("background-color: #aa0000; color: white; border: 2px solid #ff0000;");


        double sagDepth = m_sldSagDepth->value() / 100.0;
        double starve = m_sldClockStarve->value() / 100.0;
        double romRate = m_sldRomGlitch->value() / 10.0;
        double bodyAmt = m_sldBodyPressure->value() / 100.0;
        int subType = m_cmbSubstrate->currentIndex();

        bool sagToClock = m_patchMatrix[0][0]->isChecked();
        bool sagToAmp   = m_patchMatrix[0][2]->isChecked();
        bool cmosToClock= m_patchMatrix[1][0]->isChecked();
        bool cmosToOut  = m_patchMatrix[1][3]->isChecked();
        bool romToPitch = m_patchMatrix[2][1]->isChecked();
        bool romToOut   = m_patchMatrix[2][3]->isChecked();
        bool bodyToPitch= m_patchMatrix[3][1]->isChecked();
        bool bodyToAmp  = m_patchMatrix[3][2]->isChecked();

        auto synthAlgo = [=, env=0.0, dyn_phase=0.0, last_t=0.0](double t) mutable -> double {
            double dt = t - last_t;
            if (dt <= 0 || dt > 0.1) dt = 0.0000226;
            last_t = t;

            double f = 110.0;
            double pi2 = 6.2831853;


            double pMod = 1.0;
            double nNoise = ((double)rand()/RAND_MAX * 2.0 - 1.0);
            if (bodyToPitch) pMod += (bodyAmt * nNoise * std::sin(t*15));
            if (romToPitch && ((double)rand()/RAND_MAX > 0.9)) pMod += nNoise;


            double baseApx = 0.0;
            if (subType == 0) baseApx = std::sin(t*f*pi2) * (std::sin(t*8)>0?1:0);
            else if (subType == 1) baseApx = std::sin(t*f*std::exp(-t*10)*pi2) * std::exp(-t*5);
            else baseApx = (2.0 * std::fmod(t*f, 1.0) - 1.0);
            
            env = (0.01 * std::abs(baseApx)) + (0.99 * env);


            double clkRate = 1.0 - starve;
            if (sagToClock) clkRate -= (sagDepth * env);
            if (cmosToClock) {
                int c_clk = (int)(t * 8000.0);
                double cmosD = ((c_clk * ((c_clk >> 3) | (c_clk >> 6))) % 256) / 256.0;
                clkRate -= (0.5 * cmosD);
            }
            if (clkRate < 0.01) clkRate = 0.01;

            dyn_phase += (clkRate * f * pMod * pi2 * dt);


            double core = 0.0;
            if (subType == 0) core = (std::sin(dyn_phase) + std::sin(dyn_phase*1.5))*0.5 * (std::sin(t*8)>0?1:0);
            else if (subType == 1) core = std::sin(dyn_phase * std::exp(-t*10)) * std::exp(-t*5);
            else core = (2.0 * std::fmod(dyn_phase/pi2, 1.0) - 1.0) * 0.5 + ((std::sin(dyn_phase*0.5)>0)?0.5:-0.5);


            double aMod = 1.0;
            if (sagToAmp) aMod *= (1.0 - (sagDepth * env));
            if (bodyToAmp) aMod *= (1.0 - (bodyAmt * std::abs(nNoise)));
            double ampSig = core * aMod;

            double finalOut = ampSig;


            if (cmosToOut) {
                int n = (int)((dyn_phase/pi2) * 8000.0 / f);
                double cmosSig = (((n * ((n >> 5) | (n >> 8))) % 256) / 128.0) - 1.0;
                finalOut = (ampSig * 0.5) + (cmosSig * 0.5);
            }

            if (romToOut) {
                double chaotic = std::abs(std::sin(pi2 * romRate * (dyn_phase/pi2))) + (std::abs(nNoise) * 0.4);
                int mem = (int)(t * 14.0 * chaotic) % 4;
                if (mem == 0) finalOut = finalOut;
                else if (mem == 1) finalOut = std::sin(dyn_phase*1.5 + std::sin(dyn_phase*1.5));
                else if (mem == 2) finalOut = nNoise * 0.5;
                else finalOut = 0.0;
            }

            return std::clamp(finalOut, -1.0, 1.0);
        };

        m_scope->updateScope(synthAlgo, 0.5, 1.0);
        m_ghostSynth->setAudioSource(synthAlgo);
        m_ghostSynth->start();

    } else {
        m_btnPlay->setText("▶ Audition Ghost Circuit");
        m_btnPlay->setStyleSheet("");
        m_ghostSynth->stop();
    }
}