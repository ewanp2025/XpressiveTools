#include "mainwindow.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QFile>
#include <QScrollArea>
#include <cmath>
#include <algorithm>
#include <functional>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setWindowTitle("Xpressive Sound Design Suite - Ewan Pettigrew");
}

void WaveformDisplay::updateData(const std::vector<SidSegment>& segments) {
    m_segments = segments;
    update();
}

void WaveformDisplay::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height(), midY = h / 2;
    painter.setPen(QColor(200, 200, 200, 100));
    painter.drawLine(0, midY, w, midY);
    if (m_segments.empty()) return;
    double totalDur = 0;
    for (const auto& s : m_segments) totalDur += s.duration->value();
    double xPos = 0;
    for (const auto& s : m_segments) {
        double dur = s.duration->value();
        double segWidth = (dur / (totalDur > 0 ? totalDur : 1.0)) * w;
        QString type = s.waveType->currentText();
        painter.setPen(QPen(QColor(0, 120, 215), 2));
        QPolygonF poly;
        for (int x = 0; x <= (int)segWidth; ++x) {
            double localX = (double)x / (segWidth > 0 ? segWidth : 1.0);
            double env = std::exp(-localX * dur * s.decay->value());
            double phase = x * 0.2;
            double osc = (type.contains("square") || type.contains("PWM")) ? ((std::sin(phase) > 0) ? 1 : -1) :
                             (type.contains("saw") ? (fmod(phase, 6.28) / 3.14) - 1.0 :
                                  (type.contains("triangle") ? (asin(sin(phase)) * (2.0/3.14159)) :
                                       (type.contains("randv") ? ((double)std::rand() / RAND_MAX) * 2 - 1 : std::sin(phase))));
            poly << QPointF(xPos + x, midY + (osc * env * (midY - 20)));
        }
        painter.drawPolyline(poly);
        xPos += segWidth;
        painter.setPen(QColor(150, 150, 150, 80));
        painter.drawLine(xPos, 0, xPos, h);
    }
}

void MainWindow::setupUI() {
    auto *centralWidget = new QWidget(this);
    auto *mainHLayout = new QHBoxLayout(centralWidget);
    auto *sidebarGroup = new QGroupBox("Modulation & Arps");
    auto *sideScroll = new QScrollArea();
    auto *sideContent = new QWidget();
    auto *sideLayout = new QVBoxLayout(sideContent);
    QStringList mults = {"0.5x", "1x", "2x", "4x"};

    for (int i = 0; i < 5; ++i) {
        auto *mGroup = new QGroupBox(QString(i < 3 ? "Mod %1" : "PWM Mod %1").arg(i + 1));
        auto *mForm = new QFormLayout(mGroup);
        mods[i].shape = new QComboBox(); mods[i].shape->addItems({"sinew", "saww", "squarew", "trianglew"});
        mods[i].rate = new QDoubleSpinBox(); mods[i].rate->setRange(0.1, 100.0);
        mods[i].depth = new QDoubleSpinBox(); mods[i].depth->setRange(0, 1.0);
        mods[i].sync = new QCheckBox("Sync to Tempo");
        mods[i].multiplier = new QComboBox(); mods[i].multiplier->addItems(mults);
        mForm->addRow("Rate:", mods[i].rate); mForm->addRow(mods[i].sync); mForm->addRow("Mult:", mods[i].multiplier); mForm->addRow("Depth:", mods[i].depth);
        sideLayout->addWidget(mGroup);
    }
    for (int i = 0; i < 2; ++i) {
        auto *aGroup = new QGroupBox(QString("Arp %1").arg(i + 1));
        auto *aForm = new QFormLayout(aGroup);
        arps[i].wave = new QComboBox(); arps[i].wave->addItems({"squarew", "trianglew", "saww"});
        arps[i].chord = new QComboBox(); arps[i].chord->addItems({"Major", "Minor", "Dim", "Aug"});
        arps[i].speed = new QDoubleSpinBox(); arps[i].speed->setRange(1.0, 128.0); arps[i].speed->setValue(16.0);
        arps[i].sync = new QCheckBox("Sync to Tempo");
        arps[i].multiplier = new QComboBox(); arps[i].multiplier->addItems(mults);
        aForm->addRow("Wave:", arps[i].wave); aForm->addRow("Chord:", arps[i].chord); aForm->addRow("Speed:", arps[i].speed); aForm->addRow(arps[i].sync); aForm->addRow("Mult:", arps[i].multiplier);
        sideLayout->addWidget(aGroup);
    }
    sideScroll->setWidget(sideContent); sideScroll->setWidgetResizable(true);
    auto *containerLayout = new QVBoxLayout(sidebarGroup); containerLayout->addWidget(sideScroll);
    mainHLayout->addWidget(sidebarGroup, 1);

    auto *rightLayout = new QVBoxLayout();
    modeTabs = new QTabWidget(this);
    mainHLayout->addLayout(rightLayout, 3);
    rightLayout->addWidget(modeTabs);

    // 1. SID ARCHITECT - RESTORED LEGEND
    QWidget *sidTab = new QWidget();
    auto *sidLayout = new QVBoxLayout(sidTab);
    auto *sidOptGrid = new QGridLayout();
    buildModeSid = new QComboBox(); buildModeSid->addItems({"Modern", "Legacy"});
    sidOptGrid->addWidget(new QLabel("Build Mode:"), 0, 0); sidOptGrid->addWidget(buildModeSid, 0, 1);
    sidLayout->addLayout(sidOptGrid);
    waveVisualizer = new WaveformDisplay(); sidLayout->addWidget(waveVisualizer);

    auto *legend = new QLabel("LEGEND: [ Wave Type ] [ Duration(s) ] [ Freq Offset ] [ Decay ]");
    legend->setStyleSheet("font-weight: bold; background: #eee; padding: 5px; border: 1px solid #ccc;");
    sidLayout->addWidget(legend);

    auto *scroll = new QScrollArea(); auto *scrollW = new QWidget();
    sidSegmentsLayout = new QVBoxLayout(scrollW); scroll->setWidget(scrollW); scroll->setWidgetResizable(true);
    sidLayout->addWidget(scroll);
    auto *sidBtns = new QHBoxLayout();
    auto *btnAdd = new QPushButton("Add Segment (+)");
    auto *btnClear = new QPushButton("Clear All");
    auto *btnSaveSid = new QPushButton("Export SID Chain");
    sidBtns->addWidget(btnAdd); sidBtns->addWidget(btnClear); sidBtns->addWidget(btnSaveSid);
    sidLayout->addLayout(sidBtns);
    modeTabs->addTab(sidTab, "SID Architect");

    // 2. PCM SAMPLER
    QWidget *pcmTab = new QWidget();
    auto *pcmLayout = new QVBoxLayout(pcmTab);
    auto *btnLoad = new QPushButton("1. Load WAV");
    btnSave = new QPushButton("2. Generate String");
    btnCopy = new QPushButton("3. Copy Clipboard");
    btnSave->setEnabled(false); btnCopy->setEnabled(false);
    pcmLayout->addWidget(btnLoad); pcmLayout->addWidget(btnSave); pcmLayout->addWidget(btnCopy);
    auto *pcmGrid = new QGridLayout();
    buildModeCombo = new QComboBox(); buildModeCombo->addItems({"Modern", "Legacy"});
    sampleRateCombo = new QComboBox(); sampleRateCombo->addItems({"8000", "4000", "2000"});
    maxDurSpin = new QDoubleSpinBox(); maxDurSpin->setRange(0.01, 600.0); maxDurSpin->setValue(2.0);
    normalizeCheck = new QCheckBox("Normalize 4-bit"); normalizeCheck->setChecked(true);
    pcmGrid->addWidget(new QLabel("Build Mode:"), 0, 0); pcmGrid->addWidget(buildModeCombo, 0, 1);
    pcmGrid->addWidget(new QLabel("Sample Rate:"), 1, 0); pcmGrid->addWidget(sampleRateCombo, 1, 1);
    pcmGrid->addWidget(new QLabel("Max Dur(s):"), 2, 0); pcmGrid->addWidget(maxDurSpin, 2, 1);
    pcmLayout->addLayout(pcmGrid); pcmLayout->addWidget(normalizeCheck);
    modeTabs->addTab(pcmTab, "PCM Sampler");

    // 3. CONSOLE LAB
    QWidget *consoleTab = new QWidget();
    auto *consoleLayout = new QVBoxLayout(consoleTab);
    auto *conForm = new QFormLayout();
    buildModeConsole = new QComboBox(); buildModeConsole->addItems({"Modern", "Legacy"});
    consoleWaveType = new QComboBox(); consoleWaveType->addItems({"NES Triangle", "GameBoy Pulse", "4-Bit Saw"});
    consoleSteps = new QDoubleSpinBox(); consoleSteps->setRange(2, 64); consoleSteps->setValue(16);
    conForm->addRow("Build Mode:", buildModeConsole); conForm->addRow("Wave Type:", consoleWaveType); conForm->addRow("Steps:", consoleSteps);
    consoleLayout->addLayout(conForm);
    auto *btnGenConsole = new QPushButton("Generate Console String");
    consoleLayout->addWidget(btnGenConsole);
    modeTabs->addTab(consoleTab, "Console Lab");

    // 4. SFX MACRO
    QWidget *sfxTab = new QWidget();
    auto *sfxLayout = new QVBoxLayout(sfxTab);
    auto *sfxForm = new QFormLayout();
    buildModeSFX = new QComboBox(); buildModeSFX->addItems({"Modern", "Legacy"});
    sfxStartFreq = new QDoubleSpinBox(); sfxStartFreq->setRange(20, 20000); sfxStartFreq->setValue(880);
    sfxEndFreq = new QDoubleSpinBox(); sfxEndFreq->setRange(20, 20000); sfxEndFreq->setValue(110);
    sfxDur = new QDoubleSpinBox(); sfxDur->setValue(0.2);
    sfxWave = new QComboBox(); sfxWave->addItems({"trianglew", "squarew", "saww", "randv"});
    sfxForm->addRow("Build Mode:", buildModeSFX); sfxForm->addRow("Wave:", sfxWave); sfxForm->addRow("Start (Hz):", sfxStartFreq); sfxForm->addRow("End (Hz):", sfxEndFreq); sfxForm->addRow("Dur (s):", sfxDur);
    sfxLayout->addLayout(sfxForm);
    auto *btnGenSFX = new QPushButton("Generate SFX String");
    sfxLayout->addWidget(btnGenSFX);
    modeTabs->addTab(sfxTab, "SFX Macro");

    // 5. ARP ANIMATOR
    QWidget *arpTab = new QWidget();
    auto *arpLayout = new QVBoxLayout(arpTab);
    auto *arpForm = new QFormLayout();
    buildModeArp = new QComboBox(); buildModeArp->addItems({"Modern", "Legacy"});
    arpSpeed = new QDoubleSpinBox(); arpSpeed->setRange(1, 100); arpSpeed->setValue(16);
    arpInterval1 = new QComboBox(); arpInterval1->addItems({"None", "Major 3rd", "Minor 3rd", "Perfect 4th", "Perfect 5th"});
    arpInterval2 = new QComboBox(); arpInterval2->addItems({"None", "Major 3rd", "Minor 3rd", "Perfect 4th", "Perfect 5th", "Octave"});
    arpForm->addRow("Build Mode:", buildModeArp); arpForm->addRow("Speed (t/N):", arpSpeed); arpForm->addRow("Step 2:", arpInterval1); arpForm->addRow("Step 3:", arpInterval2);
    arpLayout->addLayout(arpForm);
    auto *btnGenArp = new QPushButton("Generate Arp String");
    arpLayout->addWidget(btnGenArp);
    modeTabs->addTab(arpTab, "Arp Animator");

    // 6. WAVETABLE FORGE
    QWidget *wtTab = new QWidget();
    auto *wtLayout = new QVBoxLayout(wtTab);
    auto *wtForm = new QFormLayout();
    buildModeWavetable = new QComboBox(); buildModeWavetable->addItems({"Modern", "Legacy"});
    wtBase = new QComboBox(); wtBase->addItems({"sinew", "saww", "squarew"});
    wtHarmonics = new QDoubleSpinBox(); wtHarmonics->setRange(1, 12); wtHarmonics->setValue(4);
    wtForm->addRow("Build Mode:", buildModeWavetable); wtForm->addRow("Base Type:", wtBase); wtForm->addRow("Harmonics:", wtHarmonics);
    wtLayout->addLayout(wtForm);
    auto *btnGenWT = new QPushButton("Generate Wavetable");
    wtLayout->addWidget(btnGenWT);
    modeTabs->addTab(wtTab, "Wavetable Forge");

    // 7. FILTER FORGE
    QWidget *filterTab = new QWidget();
    auto *filterLayout = new QVBoxLayout(filterTab);
    auto *expNote = new QLabel("NOTE: Filter Forge is EXPERIMENTAL.");
    expNote->setStyleSheet("color: red; font-style: italic;");
    filterLayout->addWidget(expNote);
    auto *fForm = new QFormLayout();
    buildModeFilter = new QComboBox(); buildModeFilter->addItems({"Modern", "Legacy"});
    filterType = new QComboBox(); filterType->addItems({"Low-Pass", "High-Pass"});
    filterTaps = new QDoubleSpinBox(); filterTaps->setRange(2, 8); filterTaps->setValue(4);
    fForm->addRow("Build Mode:", buildModeFilter); fForm->addRow("Type:", filterType); fForm->addRow("Taps:", filterTaps);
    filterLayout->addLayout(fForm);
    auto *btnGenFilter = new QPushButton("Generate Filter String");
    filterLayout->addWidget(btnGenFilter);
    modeTabs->addTab(filterTab, "Filter Forge");

    statusBox = new QTextEdit(); statusBox->setMaximumHeight(100);
    rightLayout->addWidget(statusBox);
    setCentralWidget(centralWidget);
    resize(1200, 850);

    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::loadWav);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::saveExpr);
    connect(btnCopy, &QPushButton::clicked, this, &MainWindow::copyToClipboard);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addSidSegment);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::clearAllSid);
    connect(btnSaveSid, &QPushButton::clicked, this, &MainWindow::saveSidExpr);
    connect(btnGenConsole, &QPushButton::clicked, this, &MainWindow::generateConsoleWave);
    connect(btnGenSFX, &QPushButton::clicked, this, &MainWindow::generateSFXMacro);
    connect(btnGenFilter, &QPushButton::clicked, this, &MainWindow::generateFilterForge);
    connect(btnGenArp, &QPushButton::clicked, this, &MainWindow::generateArpAnimator);
    connect(btnGenWT, &QPushButton::clicked, this, &MainWindow::generateWavetableForge);
}

void MainWindow::generateArpAnimator() {
    double spd = arpSpeed->value();
    QStringList ratios = {"1.0"};
    auto mapRatio = [](const QString& intv) {
        if (intv == "Major 3rd") return "1.2599";
        if (intv == "Minor 3rd") return "1.1892";
        if (intv == "Perfect 4th") return "1.3348";
        if (intv == "Perfect 5th") return "1.4983";
        if (intv == "Octave") return "2.0";
        return "1.0";
    };
    if (arpInterval1->currentText() != "None") ratios << mapRatio(arpInterval1->currentText());
    if (arpInterval2->currentText() != "None") ratios << mapRatio(arpInterval2->currentText());

    bool modern = buildModeArp->currentIndex() == 0;
    QString logic;
    if (modern) {
        logic = QString("(mod(t * %1, %2) < 1 ? %3 : (mod(t * %1, %2) < 2 ? %4 : %5))")
        .arg(spd).arg(ratios.size()).arg(ratios[0]).arg(ratios.size() > 1 ? ratios[1] : ratios[0]).arg(ratios.size() > 2 ? ratios[2] : ratios[0]);
    } else {
        logic = QString("(mod(t*%1,%2)<1)*%3 + (mod(t*%1,%2)>=1&mod(t*%1,%2)<2)*%4").arg(spd).arg(ratios.size()).arg(ratios[0]).arg(ratios.size() > 1 ? ratios[1] : ratios[0]);
    }
    statusBox->setText(QString("sinew(integrate(f * %1))").arg(logic));
    btnCopy->setEnabled(true);
}

void MainWindow::generateWavetableForge() {
    int count = (int)wtHarmonics->value();
    QString type = wtBase->currentText();
    QStringList parts;
    for (int i = 1; i <= count; ++i) parts << QString("(1/%1) * %2(t * %1)").arg(i).arg(type);
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(parts.join(" + ")));
    btnCopy->setEnabled(true);
}

void MainWindow::generateConsoleWave() {
    double steps = consoleSteps->value();
    QString base = consoleWaveType->currentIndex() == 0 ? "trianglew(integrate(f))" : (consoleWaveType->currentIndex() == 1 ? "squarew(integrate(f))" : "saww(integrate(f))");
    statusBox->setText(QString("floor(%1 * %2) / %2").arg(base).arg(steps));
    btnCopy->setEnabled(true);
}

void MainWindow::generateSFXMacro() {
    double f1 = sfxStartFreq->value(), f2 = sfxEndFreq->value(), d = sfxDur->value();
    QString wave = sfxWave->currentText();
    QString freq = QString("%1 * exp(-t * %2)").arg(f1).arg(std::log(f1/f2)/d);
    QString final = buildModeSFX->currentIndex() == 0 ? QString("(t < %1 ? %2(integrate(%3)) : 0)").arg(d).arg(wave, freq) : QString("(t < %1) * %2(integrate(%3))").arg(d).arg(wave, freq);
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(final));
    btnCopy->setEnabled(true);
}

void MainWindow::generateFilterForge() {
    int taps = (int)filterTaps->value();
    QString filterExpr = "(W1";
    for(int i = 1; i < taps; ++i) filterExpr += (filterType->currentIndex() == 0 ? " + " : " - ") + QString("last(%1)").arg(i);
    // FIXED: Corrected syntax error by adding missing closing quotes and parentheses
    statusBox->setText(QString("clamp(-1, %1 / %2, 1)").arg(filterExpr).arg(taps));
    btnCopy->setEnabled(true);
}

void MainWindow::saveSidExpr() {
    if (sidSegments.empty()) return;
    QString finalExpr = "";
    bool isModern = (buildModeSid->currentIndex() == 0);
    if (isModern) {
        std::function<QString(int, double)> buildSidTree = [&](int index, double tStart) -> QString {
            if (index >= (int)sidSegments.size()) return "0";
            const auto& s = sidSegments[index];
            QString fBase = (s.freqOffset->value() == 0) ? "f" : QString("(f + %1)").arg(s.freqOffset->value());
            QString waveExpr = getSegmentWaveform(s, fBase);
            double tEnd = tStart + s.duration->value();
            return QString("(t < %1 ? (%2 * exp(-(t - %3) * %4)) : %5)").arg(QString::number(tEnd, 'f', 4)).arg(waveExpr).arg(QString::number(tStart, 'f', 4)).arg(s.decay->value()).arg(buildSidTree(index + 1, tEnd));
        };
        finalExpr = buildSidTree(0, 0.0);
    } else {
        QString body = ""; double tPos = 0.0;
        for (const auto& s : sidSegments) {
            QString fBase = (s.freqOffset->value() == 0) ? "f" : QString("(f + %1)").arg(s.freqOffset->value());
            QString waveExpr = getSegmentWaveform(s, fBase);
            body += (body.isEmpty() ? "" : " + ") + QString("(t >= %1 & t < %2) * %3 * exp(-(t-%1)*%4)").arg(tPos).arg(tPos + s.duration->value()).arg(waveExpr).arg(s.decay->value());
            tPos += s.duration->value();
        }
        finalExpr = body;
    }
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(finalExpr));
    waveVisualizer->updateData(sidSegments);
    btnCopy->setEnabled(true);
}

void MainWindow::clearAllSid() {
    for(auto& s : sidSegments) s.container->deleteLater();
    sidSegments.clear();
    waveVisualizer->updateData(sidSegments);
}

void MainWindow::loadWav() {
    QString path = QFileDialog::getOpenFileName(this, "Select WAV", "", "WAV (*.wav)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        file.seek(24); file.read(reinterpret_cast<char*>(&fileFs), 4);
        file.seek(44); QByteArray raw = file.readAll();
        originalData.clear(); const int16_t* samples = reinterpret_cast<const int16_t*>(raw.data());
        for(int i=0; i < raw.size()/2; ++i) originalData.push_back(samples[i] / 32768.0);
        maxDurSpin->setValue((double)originalData.size() / fileFs);
        btnSave->setEnabled(true);
    }
}

void MainWindow::saveExpr() {
    if (originalData.empty()) return;
    double targetFs = sampleRateCombo->currentText().toDouble();
    std::vector<double> proc; double step = (double)fileFs / targetFs;
    int maxS = std::min((int)originalData.size(), (int)(maxDurSpin->value() * targetFs));
    for(int i=0; i < maxS; ++i) {
        double d = originalData[int(i*step)];
        if (normalizeCheck->isChecked()) { d = std::round((d + 1.0) * 0.5 * 15.0); d = ((d / 15.0) * 2.0 - 1.0) * 0.95; }
        proc.push_back(d);
    }
    statusBox->setText((buildModeCombo->currentIndex() == 0) ? generateModernPCM(proc, targetFs) : generateLegacyPCM(proc, targetFs));
    btnCopy->setEnabled(true);
}

QString MainWindow::generateModernPCM(const std::vector<double>& q, double sr) {
    int N = q.size(); if (N == 0) return "0";
    QString header = QString("var s := floor(t * %1);\n").arg(sr);
    std::function<QString(int, int)> buildTree = [&](int start, int end) -> QString {
        if (start == end) return QString::number(q[start], 'f', 3);
        int mid = start + (end - start) / 2;
        return QString("((s <= %1) ? (%2) : (%3))").arg(mid).arg(buildTree(start, mid)).arg(buildTree(mid + 1, end));
    };
    return header + buildTree(0, N - 1);
}

QString MainWindow::generateLegacyPCM(const std::vector<double>& q, double sr) {
    int blockSize = 128, N = q.size(); QStringList blocks;
    for (int b = 0; b < std::ceil((double)N / blockSize); ++b) {
        int start = b * blockSize, end = std::min((b + 1) * blockSize, N);
        QString seg = "";
        for (int i = start; i < end; ++i) seg += QString("(t<%1?%2:").arg(QString::number(i/sr, 'f', 6), QString::number(q[i], 'f', 3));
        seg += "0" + QString(")").repeated(end - start);
        blocks.append(seg);
    }
    QString expr = blocks.last();
    for (int k = blocks.size() - 2; k >= 0; --k) expr = QString("(t<%1?%2:%3)").arg(QString::number((double)((k + 1) * blockSize) / sr, 'f', 6), blocks[k], expr);
    return expr;
}

QString MainWindow::getSegmentWaveform(const SidSegment& s, const QString& fBase) {
    QString wave = s.waveType->currentText();
    if (wave.contains("PWM")) return QString("sgn(mod(t, 1/%1) < (%2 / %1)) * 2 - 1").arg(fBase, getModulatorFormula(wave.contains("4") ? 3 : 4));
    if (wave.contains("FM:")) return QString("trianglew(integrate(%1 + (%2 * 500)))").arg(fBase, getModulatorFormula(0));
    if (wave.contains("Arp")) return getArpFormula(wave.contains("1") ? 0 : 1);
    if (wave == "randv") return "randv(t * srate)";
    return QString("%1(integrate(%2))").arg(wave, fBase);
}

QString MainWindow::getModulatorFormula(int i) { return QString("(0.5 + %1(t * %2) * %3)").arg(mods[i].shape->currentText()).arg(mods[i].rate->value()).arg(mods[i].depth->value()); }

QString MainWindow::getArpFormula(int i) {
    QString wave = arps[i].wave->currentText(), speed = arps[i].sync->isChecked() ? QString("(tempo/60) * %1").arg(arps[i].multiplier->currentText().remove('x')) : QString::number(arps[i].speed->value());
    QString r1 = (arps[i].chord->currentText() == "Minor") ? "1.1892" : "1.2599", r2 = (arps[i].chord->currentText() == "Dim") ? "1.4142" : "1.4983";
    return QString("%1(integrate(f * (mod(t * %2, 3) < 1 ? 1 : (mod(t * %2, 3) < 2 ? %3 : %4))))").arg(wave, speed, r1, r2);
}

void MainWindow::addSidSegment() {
    auto *frame = new QFrame(); auto *h = new QHBoxLayout(frame);
    auto *type = new QComboBox(); type->addItems({"trianglew", "squarew", "saww", "randv", "PWM (Mod 4)", "PWM (Mod 5)", "Arp 1", "Arp 2", "FM: Mod 1", "sinew"});
    auto *dur = new QDoubleSpinBox(); dur->setValue(0.1); auto *off = new QDoubleSpinBox(); off->setRange(-10000, 10000);
    auto *dec = new QDoubleSpinBox(); dec->setValue(0); auto *rem = new QPushButton("X");
    h->addWidget(type); h->addWidget(dur); h->addWidget(off); h->addWidget(dec); h->addWidget(rem);
    sidSegmentsLayout->addWidget(frame); sidSegments.push_back({type, dur, dec, off, rem, frame});
    connect(rem, &QPushButton::clicked, this, &MainWindow::removeSidSegment);
}

void MainWindow::removeSidSegment() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    for (auto it = sidSegments.begin(); it != sidSegments.end(); ++it) if (it->deleteBtn == btn) { it->container->deleteLater(); sidSegments.erase(it); break; }
}

void MainWindow::copyToClipboard() { QApplication::clipboard()->setText(statusBox->toPlainText()); }
