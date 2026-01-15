#include "mainwindow.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QFile>
#include <QScrollArea>
#include <cmath>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    initSamLibrary(); // Initialize the new SAM library
    setupUI();
    setWindowTitle("Xpressive Sound Design Suite - Ewan Pettigrew - Uses parts of SAM phonetics code for text to speech test");
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

    leadUnisonCount = new QDoubleSpinBox(); leadUnisonCount->setRange(1, 8); leadUnisonCount->setValue(1);
    leadDetuneAmount = new QDoubleSpinBox(); leadDetuneAmount->setRange(0, 0.1); leadDetuneAmount->setSingleStep(0.001);
    leadWaveType = new QComboBox(); leadWaveType->addItems({"saww", "squarew", "sinew"});
    chaosSlider = new QSlider(Qt::Horizontal); chaosSlider->setRange(0, 100);

    sideScroll->setWidget(sideContent); sideScroll->setWidgetResizable(true);
    auto *containerLayout = new QVBoxLayout(sidebarGroup); containerLayout->addWidget(sideScroll);
    mainHLayout->addWidget(sidebarGroup, 1);

    auto *rightLayout = new QVBoxLayout();
    modeTabs = new QTabWidget(this);
    mainHLayout->addLayout(rightLayout, 3);
    rightLayout->addWidget(modeTabs);

    // 1. SID ARCHITECT
    QWidget *sidTab = new QWidget(); auto *sidLayout = new QVBoxLayout(sidTab);
    auto *sidOptGrid = new QGridLayout();
    buildModeSid = new QComboBox(); buildModeSid->addItems({"Modern", "Legacy"});
    sidOptGrid->addWidget(new QLabel("Build Mode:"), 0, 0); sidOptGrid->addWidget(buildModeSid, 0, 1);
    sidLayout->addLayout(sidOptGrid);
    waveVisualizer = new WaveformDisplay(); sidLayout->addWidget(waveVisualizer);
    auto *legend = new QLabel("LEGEND: [Wave] [Dur(s)] [Freq Off] [Decay]");
    legend->setStyleSheet("font-weight: bold; background: #eee; padding: 5px;");
    sidLayout->addWidget(legend);
    auto *scroll = new QScrollArea(); auto *scrollW = new QWidget();
    sidSegmentsLayout = new QVBoxLayout(scrollW); scroll->setWidget(scrollW); scroll->setWidgetResizable(true);
    sidLayout->addWidget(scroll);
    auto *sidBtns = new QHBoxLayout();
    auto *btnAdd = new QPushButton("Add (+)");
    auto *btnClear = new QPushButton("Clear All");
    auto *btnSaveSid = new QPushButton("Export SID Chain");
    sidBtns->addWidget(btnAdd); sidBtns->addWidget(btnClear); sidBtns->addWidget(btnSaveSid);
    sidLayout->addLayout(sidBtns);
    modeTabs->addTab(sidTab, "SID Architect");

    // 2. PCM SAMPLER
    QWidget *pcmTab = new QWidget(); auto *pcmLayout = new QVBoxLayout(pcmTab);
    auto *btnLoad = new QPushButton("Load WAV");
    btnSave = new QPushButton("Generate String");
    btnCopy = new QPushButton("Copy Clipboard");
    pcmLayout->addWidget(btnLoad); pcmLayout->addWidget(btnSave); pcmLayout->addWidget(btnCopy);
    auto *pcmGrid = new QGridLayout();
    buildModeCombo = new QComboBox(); buildModeCombo->addItems({"Modern", "Legacy"});
    sampleRateCombo = new QComboBox(); sampleRateCombo->addItems({"8000", "4000", "2000"});
    maxDurSpin = new QDoubleSpinBox(); maxDurSpin->setRange(0.01, 600.0); maxDurSpin->setValue(2.0);
    normalizeCheck = new QCheckBox("Normalize 4-bit"); normalizeCheck->setChecked(true);
    pcmGrid->addWidget(new QLabel("Build Mode:"), 0, 0); pcmGrid->addWidget(buildModeCombo, 0, 1);
    pcmGrid->addWidget(new QLabel("Rate:"), 1, 0); pcmGrid->addWidget(sampleRateCombo, 1, 1);
    pcmGrid->addWidget(new QLabel("Max(s):"), 2, 0); pcmGrid->addWidget(maxDurSpin, 2, 1);
    pcmLayout->addLayout(pcmGrid); pcmLayout->addWidget(normalizeCheck);
    modeTabs->addTab(pcmTab, "PCM Sampler");

// 3. CONSOLE LAB
    QWidget *consoleTab = new QWidget(); auto *consoleLayout = new QFormLayout(consoleTab);
    buildModeConsole = new QComboBox(); buildModeConsole->addItems({"Modern", "Legacy"});
    consoleWaveType = new QComboBox(); consoleWaveType->addItems({"NES Triangle", "4-Bit Saw"});
    consoleSteps = new QDoubleSpinBox(); consoleSteps->setValue(16);
    consoleLayout->addRow("Build Mode:", buildModeConsole);
    consoleLayout->addRow("Type:", consoleWaveType);
    consoleLayout->addRow("Steps:", consoleSteps);
    auto *btnGenConsole = new QPushButton("Generate Console String");
    consoleLayout->addRow(btnGenConsole);
    modeTabs->addTab(consoleTab, "Console Lab");

    // 4. SFX MACRO
    QWidget *sfxTab = new QWidget(); auto *sfxLayout = new QFormLayout(sfxTab);
    buildModeSFX = new QComboBox(); buildModeSFX->addItems({"Modern", "Legacy"});
    sfxStartFreq = new QDoubleSpinBox(); sfxStartFreq->setRange(20, 20000); sfxStartFreq->setValue(880);
    sfxEndFreq = new QDoubleSpinBox(); sfxEndFreq->setRange(20, 20000); sfxEndFreq->setValue(110);
    sfxDur = new QDoubleSpinBox(); sfxDur->setValue(0.2);
    sfxLayout->addRow("Build Mode:", buildModeSFX);
    sfxLayout->addRow("Start (Hz):", sfxStartFreq);
    sfxLayout->addRow("End (Hz):", sfxEndFreq);
    sfxLayout->addRow("Dur (s):", sfxDur);
    auto *btnGenSFX = new QPushButton("Generate SFX String");
    sfxLayout->addRow(btnGenSFX);
    modeTabs->addTab(sfxTab, "SFX Macro");

    // 5. ARP ANIMATOR
    QWidget *arpTab = new QWidget(); auto *arpLayout = new QFormLayout(arpTab);
    buildModeArp = new QComboBox(); buildModeArp->addItems({"Modern", "Legacy"});
    arpSpeed = new QDoubleSpinBox(); arpSpeed->setValue(16);
    arpInterval1 = new QComboBox(); arpInterval1->addItems({"None", "Major 3rd", "Minor 3rd"});
    arpInterval2 = new QComboBox(); arpInterval2->addItems({"None", "Perfect 5th", "Octave"});
    arpLayout->addRow("Build Mode:", buildModeArp);
    arpLayout->addRow("Speed:", arpSpeed);
    arpLayout->addRow("Step 2:", arpInterval1);
    arpLayout->addRow("Step 3:", arpInterval2);
    auto *btnGenArp = new QPushButton("Generate Arp String");
    arpLayout->addRow(btnGenArp);
    modeTabs->addTab(arpTab, "Arp Animator");

    // 6. WAVETABLE FORGE
    QWidget *wtTab = new QWidget(); auto *wtLayout = new QFormLayout(wtTab);
    buildModeWavetable = new QComboBox(); buildModeWavetable->addItems({"Modern", "Legacy"});
    wtBase = new QComboBox(); wtBase->addItems({"sinew", "saww", "squarew"});
    wtHarmonics = new QDoubleSpinBox(); wtHarmonics->setValue(4);
    wtLayout->addRow("Build Mode:", buildModeWavetable);
    wtLayout->addRow("Base:", wtBase);
    wtLayout->addRow("Harmonics:", wtHarmonics);
    auto *btnGenWT = new QPushButton("Generate Wavetable");
    wtLayout->addRow(btnGenWT);
    modeTabs->addTab(wtTab, "Wavetable Forge");

    // 7. BESSEL FM
    QWidget *besselTab = new QWidget(); auto *besselLayout = new QVBoxLayout(besselTab);
    auto *bForm = new QFormLayout();
    buildModeBessel = new QComboBox(); buildModeBessel->addItems({"Modern", "Legacy"});
    besselPresetCombo = new QComboBox();
    besselPresetCombo->addItems({
        "-- CATEGORY: KEYS --", "01. DX7 Electric Piano", "02. Glass Tines", "03. Dig-it-al Harp", "04. 80s FM Organ", "05. Toy Piano", "06. Celestial Keys", "07. Polished Brass", "08. Log Drum Keys",
        "-- CATEGORY: BASS --", "09. LatelyBass (TX81Z)", "10. Solid Bass", "11. Rubber Bass", "12. Wood Bass", "13. Slap FM Bass", "14. Sub-Thump", "15. Metallic Drone", "16. Acid FM",
        "-- CATEGORY: BELLS/PERC --", "17. Tubular Bells", "18. Gamelan", "19. Marimba FM", "20. Cowbell (808 style)", "21. FM Snare Crack", "22. Metallic Tom", "23. Wind Chimes", "24. Ice Bell", "25. Church Bell",
        "-- CATEGORY: PADS/LEAD --", "26. Arctic Pad", "27. FM Flute", "28. Oboe-ish", "29. Sync-Lead FM", "30. Space Reed", "31. Harmonic Swell", "32. Thin Pulse Lead", "33. Bottle Blow",
        "-- CATEGORY: FX/NOISE --", "34. Laser Harp", "35. Sci-Fi Computer", "36. Industrial Clang", "37. Digital Rain", "38. Alarm Pulse", "39. Glitch Burst", "40. 8-Bit Explosion"
    });
    besselCarrierWave = new QComboBox(); besselCarrierWave->addItems({"sinew", "saww", "squarew", "trianglew"});
    besselModWave = new QComboBox(); besselModWave->addItems({"sinew", "saww", "squarew", "trianglew"});
    besselCarrierMult = new QDoubleSpinBox(); besselCarrierMult->setRange(0, 64); besselCarrierMult->setValue(1.0);
    besselModMult = new QDoubleSpinBox(); besselModMult->setRange(0, 64); besselModMult->setValue(2.0);
    besselModIndex = new QDoubleSpinBox(); besselModIndex->setRange(0, 100); besselModIndex->setValue(2.0);
    bForm->addRow("Build Mode:", buildModeBessel);
    bForm->addRow("80s LIBRARY:", besselPresetCombo);
    bForm->addRow("Carrier Wave:", besselCarrierWave);
    bForm->addRow("Carrier Mult (C):", besselCarrierMult);
    bForm->addRow("Modulator Wave:", besselModWave);
    bForm->addRow("Modulator Mult (M):", besselModMult);
    bForm->addRow("Mod Index (I):", besselModIndex);
    besselLayout->addLayout(bForm);
    auto *btnBes = new QPushButton("Generate Bessel FM");
    besselLayout->addWidget(btnBes);
    modeTabs->addTab(besselTab, "Bessel FM");

    // 8. HARMONIC LAB
    QWidget *harTab = new QWidget(); auto *harLayout = new QVBoxLayout(harTab);
    buildModeHarmonic = new QComboBox(); buildModeHarmonic->addItems({"Modern", "Legacy"});
    harLayout->addWidget(new QLabel("Build Mode:")); harLayout->addWidget(buildModeHarmonic);
    auto *harGrid = new QGridLayout();
    for(int i=0; i<16; ++i) {
        harmonicSliders[i] = new QSlider(Qt::Vertical); harmonicSliders[i]->setRange(0, 100);
        harGrid->addWidget(new QLabel(QString("H%1").arg(i+1)), 0, i);
        harGrid->addWidget(harmonicSliders[i], 1, i);
    }
    harLayout->addLayout(harGrid);
    auto *btnHar = new QPushButton("Generate Harmonic Wave");
    harLayout->addWidget(btnHar);
    modeTabs->addTab(harTab, "Harmonic Lab");

    // --- DRUM ARCHITECT TAB ---
    QWidget *drumTab = new QWidget();
    auto *drumMainLayout = new QVBoxLayout(drumTab);
    auto *drumForm = new QFormLayout();

    drumBuildMode = new QComboBox(); drumBuildMode->addItems({"Modern", "Legacy"});

    drumType = new QComboBox();
    drumType->addItems({"Kick (Bass Drum)", "Snare/Tom", "Cymbal/Hi-Hat"});
    drumFreqSlider = new QSlider(Qt::Horizontal); drumFreqSlider->setRange(30, 150); drumFreqSlider->setValue(55);
    drumPunchSlider = new QSlider(Qt::Horizontal); drumPunchSlider->setRange(0, 100); drumPunchSlider->setValue(70);
    drumDecaySlider = new QSlider(Qt::Horizontal); drumDecaySlider->setRange(1, 100); drumDecaySlider->setValue(30);
    drumToneSlider = new QSlider(Qt::Horizontal); drumToneSlider->setRange(0, 100); drumToneSlider->setValue(20);

    drumFilterWarning = new QLabel("RECOMMENDED: Add External Low-Pass Filter (approx 400Hz)");
    drumFilterWarning->setStyleSheet("color: #ff0000; font-weight: bold; padding: 5px; border: 1px solid red;");

    auto *btnGenDrum = new QPushButton("Generate Drum Expression");

    drumForm->addRow("Build Mode:", drumBuildMode);
    drumForm->addRow("Drum Type:", drumType);
    drumForm->addRow("Base Freq (Hz):", drumFreqSlider);
    drumForm->addRow("Punch (Transient):", drumPunchSlider);
    drumForm->addRow("Decay Length:", drumDecaySlider);
    drumForm->addRow("Body Tone:", drumToneSlider);

    drumMainLayout->addLayout(drumForm);
    drumMainLayout->addWidget(drumFilterWarning);
    drumMainLayout->addWidget(btnGenDrum);

    modeTabs->addTab(drumTab, "Drum Architect");

    connect(btnGenDrum, &QPushButton::clicked, this, &MainWindow::generateDrumArchitect);

    // 9. VELOCILOGIC
    QWidget *velociTab = new QWidget(); auto *velociLayout = new QFormLayout(velociTab);
    buildModeVeloci = new QComboBox(); buildModeVeloci->addItems({"Modern", "Legacy"});
    velociType = new QComboBox(); velociType->addItems({"Hard/Soft Layer", "Additive Brightness"});
    velociLayout->addRow("Build Mode:", buildModeVeloci);
    velociLayout->addRow("Type:", velociType);
    auto *btnVeloci = new QPushButton("Generate Velocity String");
    velociLayout->addRow(btnVeloci);
    modeTabs->addTab(velociTab, "Velocilogic");

    // 10. NOISE FORGE
    QWidget *noiseTab = new QWidget(); auto *noiseLayout = new QFormLayout(noiseTab);
    buildModeNoise = new QComboBox(); buildModeNoise->addItems({"Modern", "Legacy"});
    noiseRes = new QDoubleSpinBox(); noiseRes->setRange(100, 44100); noiseRes->setValue(8000);
    noiseLayout->addRow("Build Mode:", buildModeNoise);
    noiseLayout->addRow("Sample-Rate:", noiseRes);
    auto *btnNoise = new QPushButton("Generate Noise Forge");
    noiseLayout->addRow(btnNoise);
    modeTabs->addTab(noiseTab, "Noise Forge");

    // 11. XPF PACKAGER
    QWidget *xpfTab = new QWidget(); auto *xpfLayout = new QVBoxLayout(xpfTab);
    xpfLayout->addWidget(new QLabel("Paste Single-Line Xpressive Code:"));
    xpfInput = new QTextEdit(); xpfLayout->addWidget(xpfInput);
    auto *btnXPF = new QPushButton("Package as XPF");
    xpfLayout->addWidget(btnXPF);
    modeTabs->addTab(xpfTab, "XPF Packager");

    // 12. FILTER FORGE
    QWidget *filterTab = new QWidget(); auto *filterLayout = new QFormLayout(filterTab);
    buildModeFilter = new QComboBox(); buildModeFilter->addItems({"Modern", "Legacy"});
    filterType = new QComboBox(); filterType->addItems({"Low-Pass", "High-Pass"});
    filterTaps = new QDoubleSpinBox(); filterTaps->setRange(2, 8); filterTaps->setValue(4);
    filterLayout->addRow("Build Mode:", buildModeFilter);
    filterLayout->addRow("Type:", filterType);
    filterLayout->addRow("Taps:", filterTaps);
    auto *btnFil = new QPushButton("Generate Filter");
    filterLayout->addRow(btnFil);
    modeTabs->addTab(filterTab, "Filter Forge");

    // 13. LEAD STACKER
    QWidget *leadTab = new QWidget(); auto *leadLayout = new QFormLayout(leadTab);
    leadLayout->addRow("Unison Voices:", leadUnisonCount);
    leadLayout->addRow("Detune Amount:", leadDetuneAmount);
    leadLayout->addRow("Wave Type:", leadWaveType);
    auto *btnLead = new QPushButton("Generate Lead Stack");
    leadLayout->addRow(btnLead);
    modeTabs->addTab(leadTab, "Lead Stacker");

    // 14. RANDOMIZER
    QWidget *randTab = new QWidget(); auto *randLayout = new QVBoxLayout(randTab);
    randLayout->addWidget(new QLabel("Chaos Level (Randomness):"));
    randLayout->addWidget(chaosSlider);
    auto *btnRand = new QPushButton("GENERATE CHAOS");
    btnRand->setStyleSheet("background-color: #444; color: white; font-weight: bold; height: 50px;");
    randLayout->addWidget(btnRand);
    modeTabs->addTab(randTab, "Randomizer");

    // --- 15. PHONETIC LAB (NEW) ---
    QWidget *phoneticTab = new QWidget();
    auto *pLayout = new QVBoxLayout(phoneticTab);

    pLayout->addWidget(new QLabel("<b>Phonetic Input:</b> Space separated. Use numbers 1-8 for stress/pitch (e.g., IY4)."));
    phoneticInput = new QTextEdit();
    // Default example: "SAM IS HERE" with stress markers
    phoneticInput->setPlaceholderText("S* A*4 M* space IY5 Z* space H IY4 R*");
    phoneticInput->setMaximumHeight(120);
    pLayout->addWidget(phoneticInput);

    // Settings Bar
    auto *pModeLayout = new QHBoxLayout();
    pModeLayout->addWidget(new QLabel("Render Mode:"));
    parserModeCombo = new QComboBox();
    parserModeCombo->addItems({"High Quality (Smooth)", "Retro (8-bit Grit)"});
    pModeLayout->addWidget(parserModeCombo);

    pModeLayout->addWidget(new QLabel("  Parsing Engine:"));
    parsingStyleCombo = new QComboBox();
    parsingStyleCombo->addItems({"Legacy (Xpressive)", "Nightly Build (Experimental)"});
    pModeLayout->addWidget(parsingStyleCombo);

    pLayout->addLayout(pModeLayout);

    btnGenPhonetic = new QPushButton("Generate Formula to Clipboard");
    pLayout->addWidget(btnGenPhonetic);

    pLayout->addWidget(new QLabel("<b>Phoneme Reference:</b>"));
    QScrollArea *pScroll = new QScrollArea();
    phonemeRefLabel = new QLabel(samLibrary.keys().join(" | "));
    phonemeRefLabel->setWordWrap(true);
    pScroll->setWidget(phonemeRefLabel);
    pScroll->setWidgetResizable(true);
    pLayout->addWidget(pScroll);

    modeTabs->addTab(phoneticTab, "Phonetic Lab");
    connect(btnGenPhonetic, &QPushButton::clicked, this, &MainWindow::generatePhoneticFormula);
    // ------------------------------------------

    statusBox = new QTextEdit(); statusBox->setMaximumHeight(100);
    rightLayout->addWidget(statusBox);
    setCentralWidget(centralWidget);
    resize(1200, 850);

    // Connections
    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::loadWav);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::saveExpr);
    connect(btnCopy, &QPushButton::clicked, this, &MainWindow::copyToClipboard);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addSidSegment);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::clearAllSid);
    connect(btnSaveSid, &QPushButton::clicked, this, &MainWindow::saveSidExpr);
    connect(btnGenConsole, &QPushButton::clicked, this, &MainWindow::generateConsoleWave);
    connect(btnGenSFX, &QPushButton::clicked, this, &MainWindow::generateSFXMacro);
    connect(btnGenArp, &QPushButton::clicked, this, &MainWindow::generateArpAnimator);
    connect(btnGenWT, &QPushButton::clicked, this, &MainWindow::generateWavetableForge);
    connect(btnBes, &QPushButton::clicked, this, &MainWindow::generateBesselFM);
    connect(besselPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::loadBesselPreset);
    connect(btnHar, &QPushButton::clicked, this, &MainWindow::generateHarmonicLab);
    connect(btnVeloci, &QPushButton::clicked, this, &MainWindow::generateVelocilogic);
    connect(btnNoise, &QPushButton::clicked, this, &MainWindow::generateNoiseForge);
    connect(btnXPF, &QPushButton::clicked, this, &MainWindow::generateXPFPackager);
    connect(btnFil, &QPushButton::clicked, this, &MainWindow::generateFilterForge);
    connect(btnLead, &QPushButton::clicked, this, &MainWindow::generateLeadStack);
    connect(btnRand, &QPushButton::clicked, this, &MainWindow::generateRandomPatch);
}

void MainWindow::loadBesselPreset(int idx) {
    if (idx == 0 || idx == 9 || idx == 18 || idx == 28) return;
    auto setFM = [&](QString cw, QString mw, double cm, double mm, double i) {
        besselCarrierWave->setCurrentText(cw); besselModWave->setCurrentText(mw);
        besselCarrierMult->setValue(cm); besselModMult->setValue(mm); besselModIndex->setValue(i);
    };
    switch(idx) {
    case 1:  setFM("sinew", "sinew", 1.0, 14.0, 1.2); break;
    case 2:  setFM("sinew", "sinew", 1.0, 3.5, 2.5); break;
    case 3:  setFM("trianglew", "sinew", 1.0, 8.0, 0.8); break;
    case 4:  setFM("sinew", "sinew", 2.0, 1.0, 0.5); break;
    case 5:  setFM("sinew", "sinew", 1.0, 19.0, 3.0); break;
    case 10: setFM("sinew", "saww", 1.0, 1.0, 3.5); break;
    case 11: setFM("sinew", "sinew", 1.0, 1.0, 1.8); break;
    case 19: setFM("sinew", "sinew", 1.0, 3.5, 2.0); break;
    case 20: setFM("sinew", "sinew", 1.0, 7.11, 4.0); break;
    case 37: setFM("sinew", "saww", 1.0, 8.0, 6.0); break;
    case 40: setFM("sinew", "saww", 8.0, 0.1, 50.0); break;
    default: setFM("sinew", "sinew", 1.0, 2.0, 2.0); break;
    }
}

void MainWindow::generateConsoleWave() {
    double s = consoleSteps->value();
    QString base = (consoleWaveType->currentIndex() == 0) ? "trianglew(integrate(f))" : "saww(integrate(f))";
    statusBox->setText(QString("floor(%1 * %2) / %2").arg(base).arg(s));
}

void MainWindow::generateSFXMacro() {
    double f1 = sfxStartFreq->value(), f2 = sfxEndFreq->value(), d = sfxDur->value();
    QString audio = QString("sinew(integrate(%1 * exp(-t * %2)))").arg(f1).arg(std::log(f1/f2)/d);
    statusBox->setText((buildModeSFX->currentIndex() == 0) ? QString("(t < %1 ? %2 : 0)").arg(d).arg(audio) : QString("(t < %1) * %2").arg(d).arg(audio));
}

void MainWindow::generateArpAnimator() {
    double spd = arpSpeed->value();
    QString logic = (buildModeArp->currentIndex() == 0) ? QString("(mod(t*%1,2)<1 ? 1 : 1.5)").arg(spd) : QString("(mod(t*%1,2)<1)*1 + (mod(t*%1,2)>=1)*1.5").arg(spd);
    statusBox->setText(QString("sinew(integrate(f * %1))").arg(logic));
}

void MainWindow::generateWavetableForge() {
    int h = (int)wtHarmonics->value(); QString type = wtBase->currentText();
    QStringList p; for(int i=1; i<=h; ++i) p << QString("(1/%1)*%2(t*%1)").arg(i).arg(type);
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(p.join("+")));
}

void MainWindow::generateBesselFM() {
    QString fExpr = QString("f*%1 + (%2(integrate(f*%3))*%4*f*%3)").arg(besselCarrierMult->value()).arg(besselModWave->currentText()).arg(besselModMult->value()).arg(besselModIndex->value());
    statusBox->setText(QString("clamp(-1, %1(integrate(%2)), 1)").arg(besselCarrierWave->currentText(), fExpr));
}

void MainWindow::generateHarmonicLab() {
    QStringList t;
    for(int i=0; i<16; ++i) {
        double v = harmonicSliders[i]->value() / 100.0;
        if(v > 0) t << QString("%1 * sinew(integrate(f * %2))").arg(v).arg(i+1);
    }
    statusBox->setText(t.isEmpty() ? "0" : QString("clamp(-1, %1, 1)").arg(t.join(" + ")));
}

void MainWindow::generateLeadStack() {
    int voices = (int)leadUnisonCount->value(); double detune = leadDetuneAmount->value();
    QStringList s;
    for (int i = 0; i < voices; ++i) {
        double offset = (voices == 1) ? 1.0 : 1.0 + (detune * ((double)i / (voices - 1) - 0.5) * 2.0);
        s << QString("(1.0/%1) * %2(integrate(f * %3))").arg(voices).arg(leadWaveType->currentText()).arg(offset, 0, 'f', 4);
    }
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(s.join(" + ")));
}

void MainWindow::generateVelocilogic() {
    statusBox->setText((buildModeVeloci->currentIndex() == 0) ? "(v < 0.5 ? trianglew(integrate(f)) : saww(integrate(f)))" : "(v < 0.5)*trianglew(integrate(f)) + (v >= 0.5)*saww(integrate(f))");
}

void MainWindow::generateNoiseForge() {
    statusBox->setText(QString("randv(floor(t * %1))").arg(noiseRes->value()));
}

void MainWindow::generateXPFPackager() {
    QString c = xpfInput->toPlainText().replace("\"", "&quot;").replace("<", "&lt;");
    statusBox->setText(QString("<?xml version=\"1.0\"?>\n<xpressive version=\"0.1\" O1=\"%1\" />").arg(c));
}

void MainWindow::generateFilterForge() {
    int taps = (int)filterTaps->value();
    QString expr = "(W1";
    for(int i=1; i<taps; ++i) expr += (filterType->currentIndex()==0 ? "+" : "-") + QString("last(%1)").arg(i);
    statusBox->setText(QString("clamp(-1, %1 / %2, 1)").arg(expr).arg(taps));
}

void MainWindow::generateRandomPatch() {
    int theme = std::rand() % 3; double ch = chaosSlider->value() / 100.0;
    if(theme == 0) statusBox->setText(QString("sinew(integrate(f*%1 + sinew(integrate(f*%2))*%3*f*%2))").arg((rand()%4)+1).arg((rand()%8)+1).arg((rand()%15)*ch+1));
    else if(theme == 1) statusBox->setText(QString("floor(saww(integrate(f)) * %1) / %1").arg((int)(16*ch+2)));
    else statusBox->setText("sinew(integrate(f)) + 0.5*sinew(integrate(f*2))");
}


void MainWindow::saveSidExpr() {
    if (sidSegments.empty()) return;

    QString finalExpr;
    bool isModern = (buildModeSid->currentIndex() == 0);

    if (isModern) {
        // We use a simple loop to build the nested ternary string from the BACK to the FRONT.
        // This ensures the nesting is mathematically sound for the Xpressive engine.
        QString nestedBody = "0"; // The final 'else' if time runs out
        double totalTime = 0;
        for (const auto& s : sidSegments) totalTime += s.duration->value();

        double currentTime = totalTime;
        // Iterate backwards to nest properly: (t < t1 ? seg1 : (t < t2 ? seg2 : 0))
        for (int i = sidSegments.size() - 1; i >= 0; --i) {
            const auto& s = sidSegments[i];
            double segmentDur = s.duration->value();
            currentTime -= segmentDur;

            QString fBase = (s.freqOffset->value() == 0) ? "f" : QString("(f + %1)").arg(s.freqOffset->value());
            QString waveExpr = getSegmentWaveform(s, fBase);
            QString envExpr = QString("exp(-(t - %1) * %2)").arg(currentTime, 0, 'f', 4).arg(s.decay->value());

            double tEnd = currentTime + segmentDur;
            nestedBody = QString("(t < %1 ? (%2 * %3) : %4)")
                             .arg(tEnd, 0, 'f', 4)
                             .arg(waveExpr)
                             .arg(envExpr)
                             .arg(nestedBody);
        }
        finalExpr = nestedBody;
    } else {
        // Legacy Mode: Uses additive logic (Segment1 + Segment2)
        QStringList bodies;
        double tPos = 0.0;
        for (const auto& s : sidSegments) {
            QString fBase = (s.freqOffset->value() == 0) ? "f" : QString("(f + %1)").arg(s.freqOffset->value());
            QString waveExpr = getSegmentWaveform(s, fBase);
            bodies << QString("(t >= %1 & t < %2) * %3 * exp(-(t-%1)*%4)")
                          .arg(tPos, 0, 'f', 4)
                          .arg(tPos + s.duration->value(), 0, 'f', 4)
                          .arg(waveExpr)
                          .arg(s.decay->value());
            tPos += s.duration->value();
        }
        finalExpr = bodies.join(" + ");
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
        for(int i=0; i < (int)raw.size()/2; ++i) originalData.push_back(samples[i] / 32768.0);
        maxDurSpin->setValue((double)originalData.size() / fileFs);
        btnSave->setEnabled(true);
    }
}

// FIX: Outputting normalized floats (-1.0 to 1.0) but quantised to 4-bit steps.
// This matches MATLAB logic: q = q / (levels-1) * 2 - 1
void MainWindow::saveExpr() {
    if (originalData.empty()) return;
    double targetFs = sampleRateCombo->currentText().toDouble();
    std::vector<double> proc; double step = (double)fileFs / targetFs;
    int maxS = std::min((int)originalData.size(), (int)(maxDurSpin->value() * targetFs));

    double maxVal = 0.0;
    // 1. Scan for Normalization
    if (normalizeCheck->isChecked()) {
        for(int i=0; i < maxS; ++i) {
            double d = std::abs(originalData[int(i*step)]);
            if (d > maxVal) maxVal = d;
        }
    }
    if (maxVal < 0.0001) maxVal = 1.0; // Prevent div by zero

    for(int i=0; i < maxS; ++i) {
        double d = originalData[int(i*step)];

        // 2. Normalize (-1.0 to 1.0)
        if (normalizeCheck->isChecked()) d /= maxVal;

        // 3. Quantize to 4-bit (16 levels), then Scale Back to -1..1
        // MATLAB equivalent: q = round((y+1)*0.5*15) / 15 * 2 - 1
        if (normalizeCheck->isChecked()) { // Reuse check for "4-bit mode"
            double temp = (d + 1.0) * 0.5 * 15.0; // Scale to 0..15
            int stepVal = std::round(temp);       // Round to integer step
            if(stepVal < 0) stepVal = 0;
            if(stepVal > 15) stepVal = 15;

            // Convert back to -1.0 .. 1.0 range
            d = (stepVal / 15.0) * 2.0 - 1.0;
        }

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
    int N = q.size(); if (N == 0) return "0";
    // Recursive binary tree with Float formatting
    std::function<QString(int, int)> buildLegacyTree = [&](int start, int end) -> QString {
        if (start == end) return QString::number(q[start], 'f', 3);
        int mid = start + (end - start) / 2;
        double midTime = (double)(mid + 1) / sr;
        return QString("(t < %1 ? %2 : %3)")
            .arg(QString::number(midTime, 'f', 6))
            .arg(buildLegacyTree(start, mid))
            .arg(buildLegacyTree(mid + 1, end));
    };
    return buildLegacyTree(0, N - 1);
}

QString MainWindow::getSegmentWaveform(const SidSegment& s, const QString& fBase) {
    QString wave = s.waveType->currentText();
    if (wave.contains("PWM")) return QString("sgn(mod(t, 1/%1) < (%2 / %1)) * 2 - 1").arg(fBase, getModulatorFormula(wave.contains("4") ? 3 : 4));
    if (wave.contains("FM:")) return QString("trianglew(integrate(%1 + (%2 * 500)))").arg(fBase, getModulatorFormula(0));
    if (wave.contains("Arp")) return getArpFormula(wave.contains("1") ? 0 : 1);
    if (wave == "randv") return "randv(t * srate)";
    return QString("%1(integrate(%2))").arg(wave, fBase);
}

QString MainWindow::getModulatorFormula(int index) { return QString("(0.5 + %1(t * %2) * %3)").arg(mods[index].shape->currentText()).arg(mods[index].rate->value()).arg(mods[index].depth->value()); }

QString MainWindow::getArpFormula(int index) {
    QString wave = arps[index].wave->currentText(), speed = arps[index].sync->isChecked() ? QString("(tempo/60) * %1").arg(arps[index].multiplier->currentText().remove('x')) : QString::number(arps[index].speed->value());
    QString r1 = (arps[index].chord->currentText() == "Minor") ? "1.1892" : "1.2599", r2 = (arps[index].chord->currentText() == "Dim") ? "1.4142" : "1.4983";
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

void MainWindow::generateDrumArchitect() {
    double freq = drumFreqSlider->value();
    double punch = drumPunchSlider->value() / 100.0;
    double decay = drumDecaySlider->value() / 100.0;
    double tone = drumToneSlider->value() / 100.0;

    // Core Drum Math based on your CR78 and XR90 files
    // Pitch Env: Starts high, drops fast to base freq
    QString pitchEnv = QString("(%1 * (1 + %2 * exp(-t * 120)))").arg(freq).arg(punch * 15);
    // Amplitude Env: Exponential decay
    QString ampEnv = QString("exp(-t * %1)").arg(1.0 / (decay + 0.01) * 8);

    // Saturation/Body Tone logic (CR71 style)
    QString waveType = (tone > 0.5) ? "saww" : "sinew";
    QString rawExpr = QString("%1(integrate(%2)) * %3").arg(waveType, pitchEnv, ampEnv);

    QString finalExpr;
    if (drumBuildMode->currentIndex() == 0) { // Modern
        finalExpr = QString("var drum := %1;\nclamp(-1, drum, 1)").arg(rawExpr);
    } else { // Legacy
        finalExpr = QString("clamp(-1, %1, 1)").arg(rawExpr);
    }

    statusBox->setText(finalExpr);

    // Dynamic Filter Suggestions
    if (freq < 60) {
        drumFilterWarning->setText("SUGGESTION: Add Low-Pass @ 350Hz + subtle High-Pass @ 30Hz to clear sub-mud.");
    } else if (tone > 0.5) {
        drumFilterWarning->setText("SUGGESTION: Add aggressive Band-Pass @ 1kHz to simulate Snare/Tom shell.");
    } else {
        drumFilterWarning->setText("SUGGESTION: Add Low-Pass @ 500Hz for a classic clean analog kick.");
    }
}

// --- NEW IMPLEMENTATION: PHONETIC LAB (SAM) ---

void MainWindow::initSamLibrary() {
    // 81 Phonemes mapped from SAM source tables.
    // Structure: {Name, F1, F2, F3, Voiced, A1, A2, A3, LENGTH}

    // --- VOICED VOWELS (Long & Loud) ---
    samLibrary["IY"] = {"IY", 10, 84, 110, true, 15, 10, 5, 18};
    samLibrary["IH"] = {"IH", 14, 73, 93,  true, 15, 10, 5, 15};
    samLibrary["EH"] = {"EH", 19, 67, 91,  true, 15, 10, 5, 16};
    samLibrary["AE"] = {"AE", 24, 63, 88,  true, 15, 10, 5, 18};
    samLibrary["AA"] = {"AA", 27, 40, 89,  true, 15, 10, 5, 18};
    samLibrary["AH"] = {"AH", 23, 44, 87,  true, 15, 10, 5, 16};
    samLibrary["AO"] = {"AO", 21, 31, 88,  true, 15, 10, 5, 18};
    samLibrary["UH"] = {"UH", 16, 37, 82,  true, 15, 10, 5, 15};
    samLibrary["AX"] = {"AX", 20, 45, 89,  true, 15, 10, 5, 12};
    samLibrary["IX"] = {"IX", 14, 73, 93,  true, 15, 10, 5, 12};
    samLibrary["ER"] = {"ER", 18, 49, 62,  true, 15, 10, 5, 18};
    samLibrary["UX"] = {"UX", 14, 36, 82,  true, 15, 10, 5, 15};
    samLibrary["OH"] = {"OH", 18, 30, 88,  true, 15, 10, 5, 18};

    // --- DIPHTHONGS (Very Long) ---
    samLibrary["EY"] = {"EY", 19, 72, 90,  true, 15, 10, 5, 20};
    samLibrary["AY"] = {"AY", 27, 39, 88,  true, 15, 10, 5, 22};
    samLibrary["OY"] = {"OY", 21, 31, 88,  true, 15, 10, 5, 22};
    samLibrary["AW"] = {"AW", 27, 43, 88,  true, 15, 10, 5, 22};
    samLibrary["OW"] = {"OW", 18, 30, 88,  true, 15, 10, 5, 20};
    samLibrary["UW"] = {"UW", 13, 34, 82,  true, 15, 10, 5, 18};

    // --- LIQUIDS & NASALS (Medium) ---
    samLibrary["M*"] = {"M*", 6,  46, 81,  true, 12, 8, 4, 15};
    samLibrary["N*"] = {"N*", 6,  54, 121, true, 12, 8, 4, 15};
    samLibrary["NX"] = {"NX", 6,  86, 101, true, 12, 8, 4, 15};
    samLibrary["R*"] = {"R*", 18, 50, 60,  true, 12, 8, 4, 14};
    samLibrary["L*"] = {"L*", 14, 30, 110, true, 12, 8, 4, 14};
    samLibrary["W*"] = {"W*", 11, 24, 90,  true, 12, 8, 4, 12};
    samLibrary["Y*"] = {"Y*", 9,  83, 110, true, 12, 8, 4, 12};

    // --- VOICED CONSONANTS (Short) ---
    samLibrary["Z*"] = {"Z*", 9,  51, 93,  true, 10, 6, 3, 10};
    samLibrary["ZH"] = {"ZH", 10, 66, 103, true, 10, 6, 3, 10};
    samLibrary["V*"] = {"V*", 8,  40, 76,  true, 10, 6, 3, 8};
    samLibrary["DH"] = {"DH", 10, 47, 93,  true, 10, 6, 3, 8};
    samLibrary["J*"] = {"J*", 6,  66, 121, true, 10, 6, 3, 8};
    samLibrary["B*"] = {"B*", 6,  26, 81,  true, 10, 6, 3, 6};
    samLibrary["D*"] = {"D*", 6,  66, 121, true, 10, 6, 3, 6};
    samLibrary["G*"] = {"G*", 6,  110, 112, true, 10, 6, 3, 6};
    samLibrary["GX"] = {"GX", 6,  84, 94,  true, 10, 6, 3, 6};

    // --- UNVOICED / NOISE ---

    // Fricatives (Sustained Noise)
    samLibrary["S*"] = {"S*", 6,  73, 99,  false, 8, 0, 0, 12};
    samLibrary["SH"] = {"SH", 6,  79, 106, false, 8, 0, 0, 12};
    samLibrary["F*"] = {"F*", 6,  26, 81,  false, 8, 0, 0, 10};
    samLibrary["TH"] = {"TH", 6,  66, 121, false, 8, 0, 0, 10};
    samLibrary["/H"] = {"/H", 14, 73, 93,  false, 8, 0, 0, 10};
    samLibrary["CH"] = {"CH", 6,  79, 101, false, 8, 0, 0, 10};

    // Plosives (Bursts) - UPDATED FOR AUDIBILITY
    samLibrary["P*"] = {"P*", 6,  26, 81,  false, 10, 0, 0, 5};
    samLibrary["T*"] = {"T*", 6,  66, 121, false, 10, 0, 0, 5};
    samLibrary["K*"] = {"K*", 6,  85, 101, false, 10, 0, 0, 6};
    samLibrary["KX"] = {"KX", 6,  84, 94,  false, 10, 0, 0, 6};

    // Special Characters
    samLibrary[" *"] = {" *", 0, 0, 0, false, 0, 0, 0, 5};
    samLibrary[".*"] = {".*", 19, 67, 91, false, 0, 0, 0, 10};

    // Internal Bridge Phonemes
    for(int b=43; b<=77; ++b) {
        QString key = QString("**%1").arg(b);
        if(!samLibrary.contains(key)) samLibrary[key] = {"**", 6, 60, 100, true, 10, 5, 2, 8};
    }
}

void MainWindow::generatePhoneticFormula() {
    QStringList input = phoneticInput->toPlainText().split(" ", Qt::SkipEmptyParts);

    // --- SETTINGS ---
    double frameTime = 0.012;
    double hzScale = 19.5;

    // Engine Logic: 0 = Legacy (Additive), 1 = Nightly (Nested Ternary)
    bool isNightly = (parsingStyleCombo->currentIndex() == 1);
    bool isLofi = (parserModeCombo->currentIndex() == 1);

    // Temp structure to hold parsed phonemes before assembly
    struct ParsedSegment {
        QString content;
        double duration;
    };
    QList<ParsedSegment> sequence;

    // --- 1. PARSE INPUT TO DATA ---
    for(int i = 0; i < std::min((int)input.size(), 128); ++i) {
        QString rawToken = input[i].toUpper();
        QRegularExpression re("([A-Z\\*\\/\\.\\?\\,\\-]+)(\\d*)");
        QRegularExpressionMatch match = re.match(rawToken);

        QString key = match.captured(1);
        QString stressStr = match.captured(2);

        if(!samLibrary.contains(key)) continue;
        SAMPhoneme p = samLibrary[key];

        // Stress Logic
        int stress = stressStr.isEmpty() ? 4 : stressStr.toInt();
        double pitchMult = 0.85 + (stress * 0.05);
        double duration = (p.length * frameTime) * (0.8 + (stress * 0.05));

        QString content;

        if (p.voiced) {
            // VOICED SYNTHESIS
            double freq1 = p.f1 * hzScale;
            double freq2 = p.f2 * hzScale;
            double freq3 = p.f3 * hzScale;

            QString s1 = QString("%1*sinew(integrate(%2))").arg(p.a1 * 0.05).arg(freq1);
            QString s2 = QString("%1*sinew(integrate(%2))").arg(p.a2 * 0.05).arg(freq2);
            QString s3 = QString("%1*sinew(integrate(%2))").arg(p.a3 * 0.05).arg(freq3);
            QString glottal = QString("(0.8 * (1 - mod(t*f*%1, 1)))").arg(pitchMult);

            content = QString("((%1 + %2 + %3) * %4)").arg(s1, s2, s3, glottal);
        } else {
            // NOISE SYNTHESIS
            double rawF1 = (p.f1 > 0 ? p.f1 : 100);
            double noiseColor = (rawF1 > 90 ? 90 : rawF1) * 80.0;
            double noiseAmp = (p.length < 8) ? 0.9 : 0.4;

            content = QString("(%1 * randv(t*%2))").arg(noiseAmp).arg(noiseColor);
        }

        sequence.append({content, duration});
    }

    if (sequence.isEmpty()) return;

    QString finalFormula;

    // --- 2. ASSEMBLE FORMULA ---
    if (isNightly) {
        // NIGHTLY BUILD: Nested Ternary Logic (Recursive Backwards)
        // Matches "Modern" logic in SID Architect

        QString nestedBody = "0"; // End of chain (silence)
        double totalTime = 0;
        for(const auto &s : sequence) totalTime += s.duration;

        double currentTime = totalTime;

        // Iterate backwards from last sound to first
        for (int i = sequence.size() - 1; i >= 0; --i) {
            ParsedSegment seg = sequence[i];
            currentTime -= seg.duration;

            // Envelopes relative to local segment start (currentTime)
            // Fast attack/release to prevent clicks
            double fadeSpeed = 120.0;
            QString attack = QString("min(1, (t-%1)*%2)").arg(currentTime).arg(fadeSpeed);
            QString decay = QString("min(1, (%1-t)*%2)").arg(currentTime + seg.duration).arg(fadeSpeed);

            double tEnd = currentTime + seg.duration;

            // Structure: (t < end_time ? (sound * env) : (next_nested_block))
            nestedBody = QString("(t < %1 ? (%2 * %3 * %4) : %5)")
                             .arg(tEnd, 0, 'f', 4)
                             .arg(seg.content)
                             .arg(attack)
                             .arg(decay)
                             .arg(nestedBody);
        }
        finalFormula = nestedBody;

    } else {
        // LEGACY PARSING: Additive Logic (Forwards)
        // Calculates all segments simultaneously and sums them

        QStringList stringSegments;
        double time = 0.0;

        for (const auto &seg : sequence) {
            double fadeSpeed = 120.0; // Same envelope speed
            QString attack = QString("min(1, (t-%1)*%2)").arg(time).arg(fadeSpeed);
            QString decay = QString("min(1, (%1-t)*%2)").arg(time + seg.duration).arg(fadeSpeed);

            // Structure: ((Range Check) * Sound * Env)
            QString block = QString("((t >= %1 & t < %2) * %3 * %4 * %5)")
                                .arg(time)
                                .arg(time + seg.duration)
                                .arg(seg.content)
                                .arg(attack)
                                .arg(decay);
            stringSegments << block;
            time += seg.duration;
        }
        finalFormula = stringSegments.join(" + ");
    }

    // --- 3. POST-PROCESSING (Audio Quality) ---
    if (isLofi) {
        // Bitcrush / Retro mode: Quantize to 16 steps
        finalFormula = QString("clamp(-1, floor((%1) * 16)/16, 1)").arg(finalFormula);
    } else {
        // High Quality: Simple safety clamp
        finalFormula = QString("clamp(-1, %1, 1)").arg(finalFormula);
    }

    statusBox->setText(finalFormula);
    QApplication::clipboard()->setText(finalFormula);
}
