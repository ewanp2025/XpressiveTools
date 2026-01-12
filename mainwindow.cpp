#include "mainwindow.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QScrollArea>
#include <cmath>
#include <algorithm>
#include <functional>
#include <cstdlib>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setWindowTitle("Xpressive Toolset (Unofficial) - Ewan Pettigrew");
}

// --- WAVEFORM DISPLAY LOGIC ---
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
            double osc = 0;
            if (type.contains("square") || type.contains("PWM")) osc = (std::sin(phase) > 0) ? 1 : -1;
            else if (type.contains("saw")) osc = (fmod(phase, 6.28) / 3.14) - 1.0;
            else if (type.contains("triangle")) osc = (asin(sin(phase)) * (2.0/3.14159));
            else if (type.contains("randv")) osc = ((double)std::rand() / RAND_MAX) * 2 - 1;
            else osc = std::sin(phase);
            poly << QPointF(xPos + x, midY + (osc * env * (midY - 20)));
        }
        painter.drawPolyline(poly);
        xPos += segWidth;
        painter.setPen(QColor(150, 150, 150, 80));
        painter.drawLine(xPos, 0, xPos, h);
    }
}

// --- UI SETUP ---
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

    QWidget *sidTab = new QWidget();
    auto *sidLayout = new QVBoxLayout(sidTab);
    waveVisualizer = new WaveformDisplay();
    sidLayout->addWidget(waveVisualizer);
    auto *legend = new QLabel("LEGEND: [ Wave Type ] [ Duration(s) ] [ Freq Offset ] [ Decay ]");
    legend->setStyleSheet("font-weight: bold; background: #eee; padding: 5px;");
    sidLayout->addWidget(legend);
    auto *scroll = new QScrollArea(); auto *scrollW = new QWidget();
    sidSegmentsLayout = new QVBoxLayout(scrollW); scroll->setWidget(scrollW); scroll->setWidgetResizable(true);
    sidLayout->addWidget(scroll);
    auto *sidBtns = new QHBoxLayout();
    auto *btnAdd = new QPushButton("Add Segment (+)");
    auto *btnSaveSid = new QPushButton("Export SID Chain");
    sidBtns->addWidget(btnAdd); sidBtns->addWidget(btnSaveSid);
    sidLayout->addLayout(sidBtns);
    modeTabs->addTab(sidTab, "SID Architect");

    QWidget *pcmTab = new QWidget();
    auto *pcmLayout = new QVBoxLayout(pcmTab);
    auto *btnLoad = new QPushButton("1. Load WAV File");
    btnSave = new QPushButton("2. Generate String");
    btnCopy = new QPushButton("3. Copy Clipboard");
    btnSave->setEnabled(false); btnCopy->setEnabled(false);
    pcmLayout->addWidget(btnLoad); pcmLayout->addWidget(btnSave); pcmLayout->addWidget(btnCopy);
    auto *pcmGrid = new QGridLayout();
    buildModeCombo = new QComboBox(); buildModeCombo->addItems({"Modern", "Legacy"});
    sampleRateCombo = new QComboBox(); sampleRateCombo->addItems({"8000", "4000", "2000"});
    maxDurSpin = new QDoubleSpinBox(); maxDurSpin->setRange(0.01, 600.0); maxDurSpin->setValue(2.0);
    normalizeCheck = new QCheckBox("Normalize"); normalizeCheck->setChecked(true);
    pcmGrid->addWidget(new QLabel("Build Mode:"), 0, 0); pcmGrid->addWidget(buildModeCombo, 0, 1);
    pcmGrid->addWidget(new QLabel("Sample Rate:"), 1, 0); pcmGrid->addWidget(sampleRateCombo, 1, 1);
    pcmGrid->addWidget(new QLabel("Max Dur(s):"), 2, 0); pcmGrid->addWidget(maxDurSpin, 2, 1);
    pcmLayout->addLayout(pcmGrid); pcmLayout->addWidget(normalizeCheck);
    modeTabs->addTab(pcmTab, "PCM Sampler");

    statusBox = new QTextEdit(); statusBox->setMaximumHeight(100);
    rightLayout->addWidget(statusBox);
    setCentralWidget(centralWidget);
    resize(1200, 850);

    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::loadWav);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::saveExpr);
    connect(btnCopy, &QPushButton::clicked, this, &MainWindow::copyToClipboard);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addSidSegment);
    connect(btnSaveSid, &QPushButton::clicked, this, &MainWindow::saveSidExpr);
}

// --- PCM SAMPLER LOGIC ---
void MainWindow::loadWav() {
    QString path = QFileDialog::getOpenFileName(this, "Select WAV", "", "WAV (*.wav)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        file.seek(24); file.read(reinterpret_cast<char*>(&fileFs), 4);
        file.seek(44); QByteArray raw = file.readAll();
        originalData.clear(); const int16_t* samples = reinterpret_cast<const int16_t*>(raw.data());
        for(int i=0; i < raw.size()/2; ++i) originalData.push_back(samples[i] / 32768.0);
        double duration = (double)originalData.size() / fileFs;
        maxDurSpin->setValue(duration);
        statusBox->setText(QString("WAV Loaded: %1s detected.").arg(duration, 0, 'f', 2));
        btnSave->setEnabled(true);
    }
}

QString MainWindow::generateModernPCM(const std::vector<double>& q, double sr) {
    int N = q.size();
    if (N == 0) return "0";
    QString header = QString("var s := floor(t * %1);\n").arg(sr);

    std::function<QString(int, int)> buildTree = [&](int start, int end) -> QString {
        if (start == end) return QString::number(q[start], 'f', 3);
        int mid = start + (end - start) / 2;
        return QString("((s <= %1) ? (%2) : (%3))")
            .arg(QString::number(mid), buildTree(start, mid), buildTree(mid + 1, end));
    };

    return header + buildTree(0, N - 1);
}

QString MainWindow::generateLegacyPCM(const std::vector<double>& q, double sr) {
    int blockSize = 128;
    int N = q.size(); QStringList blocks;
    for (int b = 0; b < std::ceil((double)N / blockSize); ++b) {
        int start = b * blockSize, end = std::min((b + 1) * blockSize, N);
        QString seg = "";
        for (int i = start; i < end; ++i) seg += QString("(t<%1?%2:").arg(QString::number(i/sr, 'f', 6), QString::number(q[i], 'f', 3));
        seg += "0" + QString(")").repeated(end - start);
        blocks.append(seg);
    }
    QString expr = blocks.last();
    for (int k = blocks.size() - 2; k >= 0; --k) {
        double T = (double)((k + 1) * blockSize) / sr;
        expr = QString("(t<%1?%2:%3)").arg(QString::number(T, 'f', 6), blocks[k], expr);
    }
    return expr;
}

void MainWindow::saveExpr() {
    if (originalData.empty()) return;
    double targetFs = sampleRateCombo->currentText().toDouble();
    std::vector<double> proc; double step = (double)fileFs / targetFs;
    int maxS = std::min((int)originalData.size(), (int)(maxDurSpin->value() * targetFs));
    for(int i=0; i < maxS; ++i) {
        double d = originalData[int(i*step)];
        if (normalizeCheck->isChecked()) {
            d = std::round((d + 1.0) * 0.5 * 15.0);
            d = ((d / 15.0) * 2.0 - 1.0) * 0.95;
        }
        proc.push_back(d);
    }
    // CLAMP REMOVED PER USER REQUEST
    QString final = (buildModeCombo->currentIndex() == 0) ? generateModernPCM(proc, targetFs) : generateLegacyPCM(proc, targetFs);
    statusBox->setText(final);
    btnCopy->setEnabled(true);
}

// --- SID LOGIC & STUBS ---
QString MainWindow::getArpFormula(int i) {
    QString wave = arps[i].wave->currentText();
    QString speedExpr = arps[i].sync->isChecked() ? QString("(tempo/60) * %1").arg(arps[i].multiplier->currentText().remove('x')) : QString::number(arps[i].speed->value());
    QString r1 = (arps[i].chord->currentText() == "Minor") ? "1.1892" : "1.2599";
    QString r2 = (arps[i].chord->currentText() == "Dim") ? "1.4142" : "1.4983";
    return QString("%1(integrate(f * (mod(t * %2, 3) < 1 ? 1 : (mod(t * %2, 3) < 2 ? %3 : %4))))").arg(wave, speedExpr, r1, r2);
}

void MainWindow::saveSidExpr() {
    QString body = ""; double tPos = 0.0;
    for (const auto& s : sidSegments) {
        QString wave = s.waveType->currentText(), fBase = (s.freqOffset->value() == 0) ? "f" : QString("(f + %1)").arg(s.freqOffset->value()), waveExpr;
        if (wave.contains("PWM")) waveExpr = QString("sgn(mod(t, 1/%1) < (%2 / %1)) * 2 - 1").arg(fBase, getModulatorFormula(wave.contains("4") ? 3 : 4));
        else if (wave.contains("FM:")) waveExpr = QString("trianglew(integrate(%1 + (%2 * 500)))").arg(fBase, getModulatorFormula(0));
        else if (wave.contains("Arp")) waveExpr = getArpFormula(wave.contains("1") ? 0 : 1);
        else if (wave == "randv") waveExpr = "randv(t * srate)";
        else waveExpr = QString("%1(integrate(%2))").arg(wave, fBase);
        body += (body.isEmpty() ? "" : " + ") + QString("(t >= %1 & t < %2) * %3 * exp(-(t-%1)*%4)").arg(tPos).arg(tPos + s.duration->value()).arg(waveExpr).arg(s.decay->value());
        tPos += s.duration->value();
    }
    // SID EXPORT STILL USES CLAMP FOR PEAK PROTECTION
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(body));
    waveVisualizer->updateData(sidSegments);
    btnCopy->setEnabled(true);
}

void MainWindow::addSidSegment() {
    auto *frame = new QFrame(); auto *h = new QHBoxLayout(frame);
    auto *type = new QComboBox(); type->addItems({"trianglew", "squarew", "saww", "randv", "PWM (Mod 4)", "PWM (Mod 5)", "Arp 1", "Arp 2", "FM: Mod 1", "Tonal Noise (YM)", "Pitch Bend", "Hardware Buzz"});
    auto *dur = new QDoubleSpinBox(); dur->setValue(0.1);
    auto *off = new QDoubleSpinBox(); off->setRange(-10000, 10000);
    auto *dec = new QDoubleSpinBox(); dec->setValue(0);
    auto *rem = new QPushButton("X");
    h->addWidget(type); h->addWidget(dur); h->addWidget(off); h->addWidget(dec); h->addWidget(rem);
    sidSegmentsLayout->addWidget(frame);
    sidSegments.push_back({type, dur, dec, off, rem, frame});
    connect(rem, &QPushButton::clicked, this, &MainWindow::removeSidSegment);
}

void MainWindow::removeSidSegment() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    for (auto it = sidSegments.begin(); it != sidSegments.end(); ++it) {
        if (it->deleteBtn == btn) { it->container->deleteLater(); sidSegments.erase(it); break; }
    }
}
void MainWindow::copyToClipboard() { QApplication::clipboard()->setText(statusBox->toPlainText()); }
QString MainWindow::getModulatorFormula(int i) { return QString("(0.5 + %1(t * %2) * %3)").arg(mods[i].shape->currentText()).arg(mods[i].rate->value()).arg(mods[i].depth->value()); }
void MainWindow::clearAllSid() {}
