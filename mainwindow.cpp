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

#include <QClipboard>     // Required for copyToClipboard()
#include <QApplication>   // Required for clipboard access and global app logic
#include <QTextStream>    // Required for saving .xpf files in saveXpfInstrument()
#include <QFrame>         // Required for creating UI segments in addSidSegment()
#include <QMessageBox>    // Recommended for error reporting during file saves


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
    QWidget *arpTab = new QWidget();
    auto *arpLayout = new QVBoxLayout(arpTab);

    // --- SECTION 1: THE OSCILLATOR ---
    auto *oscGroup = new QGroupBox("SID Oscillator");
    auto *oscForm = new QFormLayout(oscGroup);

    buildModeArp = new QComboBox();
    buildModeArp->addItems({"Nightly (Nested - Clean)", "Legacy (Additive)"});

    arpWave = new QComboBox();
    arpWave->addItems({"Pulse (Classic)", "Sawtooth", "Triangle", "Noise (Percussion)", "Metal (Ring Mod)"});

    arpPwmSlider = new QSlider(Qt::Horizontal);
    arpPwmSlider->setRange(1, 99);
    arpPwmSlider->setValue(50); // Square wave default

    oscForm->addRow("Build Mode:", buildModeArp);
    oscForm->addRow("Waveform:", arpWave);
    oscForm->addRow("Pulse Width:", arpPwmSlider);

    arpLayout->addWidget(oscGroup);

    // --- SECTION 2: THE CHORD (INTERVALS) ---
    auto *seqGroup = new QGroupBox("Chord Sequence (0 -> Step 2 -> Step 3)");
    auto *seqForm = new QFormLayout(seqGroup);

    // Populating with Semitones (The way trackers do it)
    QStringList intervals = {
        "0 (Root)", "+3 (Minor 3rd)", "+4 (Major 3rd)", "+5 (4th)",
        "+7 (Perfect 5th)", "+12 (Octave)", "-12 (Sub Octave)",
        "+19 (Octave+5th)", "+24 (2 Octaves)"
    };

    arpInterval1 = new QComboBox(); arpInterval1->addItems(intervals);
    arpInterval1->setCurrentIndex(2); // Major 3rd default

    arpInterval2 = new QComboBox(); arpInterval2->addItems(intervals);
    arpInterval2->setCurrentIndex(4); // Perfect 5th default

    seqForm->addRow("Step 2 Note:", arpInterval1);
    seqForm->addRow("Step 3 Note:", arpInterval2);

    arpLayout->addWidget(seqGroup);

    // --- SECTION 3: SPEED & SYNC ---
    auto *spdGroup = new QGroupBox("Speed / Tempo");
    auto *spdForm = new QFormLayout(spdGroup);

    arpBpmSync = new QCheckBox("Sync to BPM");
    arpBpmSync->setChecked(true);

    arpBpmVal = new QDoubleSpinBox();
    arpBpmVal->setRange(40, 300);
    arpBpmVal->setValue(125); // Classic Techno/Chiptune tempo

    arpSpeedDiv = new QComboBox();
    arpSpeedDiv->addItems({"1/16 (Standard)", "1/32 (Fast)", "1/48 (Triplets)", "1/64 (Hubbard Speed)", "50Hz (PAL Frame)"});
    arpSpeedDiv->setCurrentIndex(3); // Default to "Hubbard Speed"

    // We keep the old Hz spinner just in case they uncheck Sync
    arpSpeed = new QDoubleSpinBox();
    arpSpeed->setRange(0.1, 1000);
    arpSpeed->setValue(50);
    arpSpeed->setVisible(false); // Hidden by default

    spdForm->addRow(arpBpmSync);
    spdForm->addRow("Song BPM:", arpBpmVal);
    spdForm->addRow("Grid Size:", arpSpeedDiv);
    spdForm->addRow("Manual Hz:", arpSpeed);

    arpLayout->addWidget(spdGroup);

    // GENERATE BUTTON
    auto *btnGenArp = new QPushButton("GENERATE C64 ARP");
    btnGenArp->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 40px;");
    arpLayout->addWidget(btnGenArp);

    // Logic to toggle inputs
    connect(arpBpmSync, &QCheckBox::toggled, [=](bool checked){
        arpBpmVal->setVisible(checked);
        arpSpeedDiv->setVisible(checked);
        arpSpeed->setVisible(!checked);
    });

    modeTabs->addTab(arpTab, "Arp Animator");
    connect(btnGenArp, &QPushButton::clicked, this, &MainWindow::generateArpAnimator);

    // 6. WAVETABLE FORGE (The "Hero" Tracker)
    QWidget *wtTab = new QWidget();
    auto *wtLayout = new QVBoxLayout(wtTab);

    // Header: Presets, Loop & Build Mode
    auto *wtHeader = new QHBoxLayout();

    // 1. ADD: Build Mode Selection
    wtHeader->addWidget(new QLabel("Build Mode:"));
    buildModeWavetable = new QComboBox();
    buildModeWavetable->addItems({"Nightly (Nested)", "Legacy (Additive)"});
    wtHeader->addWidget(buildModeWavetable);


    wtHeader->addWidget(new QLabel("| Master Library:"));
    wtPresetCombo = new QComboBox();

    // THE MASTER LIST
    QStringList presets;
    presets << "--- INIT ---" << "00. Empty / Init";

    presets << "--- ROB HUBBARD ---"
            << "01. Commando Bass (Glissando)" << "02. Monty Lead (Pulse+Vib)"
            << "03. Delta Snare (Tri-Noise)" << "04. Zoids Metal (Ring Mod)"
            << "05. Ace 2 Kick (Deep)" << "06. Crazy Comets (Echo)";

    presets << "--- MARTIN GALWAY ---"
            << "07. Wizball Arp (Bubble)" << "08. Parallax Bass (Slap)"
            << "09. Comic Bakery (Lead)" << "10. Arkanoid (Dotted Echo)"
            << "11. Green Beret (Military Snare)";

    presets << "--- JEROEN TEL ---"
            << "12. Cybernoid Metal Drum" << "13. Supremacy Lead (Vibrato)"
            << "14. Turbo Outrun (Bass)" << "15. RoboCop 3 (Title Arp)";

    presets << "--- CHRIS HUELSBECK ---"
            << "16. Turrican I (Huge Arp)" << "17. Turrican II (Pad)"
            << "18. Katakis (Space Lead)" << "19. Great Giana (Bass)";

    presets << "--- TIM FOLLIN ---"
            << "20. Solstice (Intro Lead)" << "21. Ghouls'n'Ghosts (Rain)"
            << "22. Silver Surfer (Arp)" << "23. LED Storm (Bass)";

    presets << "--- BEN DAGLISH ---"
            << "24. Last Ninja (Dark Bass)" << "25. Deflektor (Lead)"
            << "26. Trap (Fast Arp)";

    presets << "--- DAVID WHITTAKER ---"
            << "27. Glider Rider (Square)" << "28. Lazy Jones (Laser)";

    presets << "--- YM / ATARI ST MASTERS ---"
            << "29. YM Buzzer Envelope" << "30. YM Metal Bass"
            << "31. YM 3-Voice Chord" << "32. Digi-Drum (SID-Style)";

    presets << "--- FX / DRUMS (Utility) ---"
            << "33. Coin (Mario Style)" << "34. Power Up"
            << "35. Explosion (Noise Decay)" << "36. Laser (Pew Pew)"
            << "37. 8-Bit Hi-Hat (Closed)" << "38. 8-Bit Hi-Hat (Open)"
            << "39. Fake Chord (Minor)" << "40. Fake Chord (Major)"
            << "--- SID DRUMS EXPANSION ---"
            << "41. Heavy SID Kick (Square Drop)"
            << "42. Snappy Snare (Tri+Noise)"
            << "43. Tech Kick (Metal+Pulse)"
            << "44. Glitch Snare (Ring Mod)";



    wtPresetCombo->addItems(presets);
    wtHeader->addWidget(wtPresetCombo);

    wtLoopCheck = new QCheckBox("Loop Sequence");
    wtHeader->addWidget(wtLoopCheck);

    wtLayout->addLayout(wtHeader);

    // ... (The rest of the Table/Button setup stays exactly the same) ...
    // ... just keep the code below the header from the previous step ...

    // The Tracker Table
    wtTrackerTable = new QTableWidget();
    wtTrackerTable->setColumnCount(4);
    wtTrackerTable->setHorizontalHeaderLabels({"Waveform", "Pitch (+/-)", "PWM %", "Dur (s)"});
    wtTrackerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    wtTrackerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    wtLayout->addWidget(wtTrackerTable);

    // Buttons
    auto *wtBtnLayout = new QHBoxLayout();
    auto *btnAddStep = new QPushButton("Add Step (+)");
    auto *btnRemStep = new QPushButton("Remove Step (-)");
    auto *btnGenWT = new QPushButton("GENERATE SEQUENCE");

    // Style the big button to look cool
    btnGenWT->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 40px;");

    wtBtnLayout->addWidget(btnAddStep);
    wtBtnLayout->addWidget(btnRemStep);
    wtLayout->addLayout(wtBtnLayout);
    wtLayout->addWidget(btnGenWT);

    modeTabs->addTab(wtTab, "Wavetable Forge");

    // Connections
    connect(wtPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::loadWavetablePreset);
    connect(btnGenWT, &QPushButton::clicked, this, &MainWindow::generateWavetableForge);

    // Table Editing Logic
    connect(btnAddStep, &QPushButton::clicked, [=](){
        int row = wtTrackerTable->rowCount();
        wtTrackerTable->insertRow(row);
        // Defaults: Saw, +0 semi, 50% PWM, 0.05s
        wtTrackerTable->setItem(row, 0, new QTableWidgetItem("Saw"));
        wtTrackerTable->setItem(row, 1, new QTableWidgetItem("0"));
        wtTrackerTable->setItem(row, 2, new QTableWidgetItem("50"));
        wtTrackerTable->setItem(row, 3, new QTableWidgetItem("0.05"));
    });

    connect(btnRemStep, &QPushButton::clicked, [=](){
        if (wtTrackerTable->rowCount() > 0)
            wtTrackerTable->removeRow(wtTrackerTable->rowCount()-1);
    });

    // Load default preset
    loadWavetablePreset(1); // Load Hubbard Kick by default

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

    // 9. --- DRUM ARCHITECT TAB ---
    QWidget *drumWidget = new QWidget();
    QVBoxLayout *drumLayout = new QVBoxLayout(drumWidget);

    // Disclaimer regarding Panning and Filter behavior
    drumDisclaimer = new QLabel("⚠️ NOTICE: Panning must be set manually in the Instrument Editor due to XML parsing issues. "
                                "Filters may require manual adjustment (0 Frequency = Silence).");
    drumDisclaimer->setStyleSheet("color: red; font-weight: bold; border: 1px solid red; padding: 5px;");
    drumDisclaimer->setWordWrap(true);
    drumLayout->addWidget(drumDisclaimer);

    drumTypeCombo = new QComboBox();
    drumTypeCombo->addItems({"Kick (LPF)", "Snare (BPF)", "Hi-Hat (HPF)", "Tom (LPF)", "Cowbell (BPF)", "Rimshot (HPF)", "Clap (BPF)"});

    drumWaveCombo = new QComboBox();
    drumWaveCombo->addItems({"Sine", "Triangle", "Square", "Sawtooth"});

    QFormLayout *fLayout = new QFormLayout();

    // Instantiating all sliders
    drumPitchSlider = new QSlider(Qt::Horizontal); drumPitchSlider->setRange(20, 100);
    drumDecaySlider = new QSlider(Qt::Horizontal); drumDecaySlider->setRange(1, 200);
    drumPitchDropSlider = new QSlider(Qt::Horizontal); drumPitchDropSlider->setRange(0, 500);
    drumToneSlider = new QSlider(Qt::Horizontal);  drumToneSlider->setRange(100, 14000); // Min 100 for audibility
    drumSnapSlider = new QSlider(Qt::Horizontal);  drumSnapSlider->setRange(10, 100);    // Min 10 for HPF bite
    drumNoiseSlider = new QSlider(Qt::Horizontal); drumNoiseSlider->setRange(0, 100);
    drumPWMSlider = new QSlider(Qt::Horizontal);   drumPWMSlider->setRange(0, 100);
    drumExpSlider = new QSlider(Qt::Horizontal);   drumExpSlider->setRange(1, 10);      // Exponential factor

    fLayout->addRow("Body Waveform:", drumWaveCombo);
    fLayout->addRow("Base Pitch:", drumPitchSlider);
    fLayout->addRow("Decay Speed:", drumDecaySlider);
    fLayout->addRow("Exponential Curve:", drumExpSlider);
    fLayout->addRow("Pitch Punch (Drop):", drumPitchDropSlider);
    fLayout->addRow("Filter Cutoff:", drumToneSlider);
    fLayout->addRow("Filter Res (Snap):", drumSnapSlider);
    fLayout->addRow("Noise Mix:", drumNoiseSlider);
    fLayout->addRow("Pulse Width:", drumPWMSlider);

    btnGenerateDrum = new QPushButton("Copy XPF to Clipboard");
    btnSaveDrumXpf = new QPushButton("Save Drum as .XPF File");

    drumLayout->addWidget(new QLabel("<b>Internal Filter Drum Designer</b>"));
    drumLayout->addLayout(fLayout);
    drumLayout->addWidget(drumTypeCombo);
    drumLayout->addWidget(btnGenerateDrum);
    drumLayout->addWidget(btnSaveDrumXpf);
    drumLayout->addStretch();

    // SUGGESTION LOGIC
    connect(drumTypeCombo, &QComboBox::currentIndexChanged, [=](int idx){
        switch(idx) {
        case 0: // Kick
            drumWaveCombo->setCurrentText("Sine"); drumPitchSlider->setValue(40);
            drumPitchDropSlider->setValue(350); drumDecaySlider->setValue(40); drumExpSlider->setValue(2); break;
        case 1: // Snare
            drumWaveCombo->setCurrentText("Triangle"); drumNoiseSlider->setValue(70);
            drumToneSlider->setValue(1200); drumDecaySlider->setValue(80); drumExpSlider->setValue(4); break;
        case 2: // Hi-Hat
            drumWaveCombo->setCurrentText("Square"); drumPitchSlider->setValue(80);
            drumDecaySlider->setValue(160); drumNoiseSlider->setValue(100); drumToneSlider->setValue(8000); break;
        case 4: // Cowbell
            drumWaveCombo->setCurrentText("Square"); drumPitchSlider->setValue(80);
            drumPitchDropSlider->setValue(0); drumToneSlider->setValue(3000); drumExpSlider->setValue(3); break;
        case 5: // Rimshot
            drumWaveCombo->setCurrentText("Square"); drumPitchSlider->setValue(95);
            drumNoiseSlider->setValue(20); drumToneSlider->setValue(5000); drumExpSlider->setValue(8); break;
        case 6: // Clap
            drumWaveCombo->setCurrentText("Sawtooth"); drumNoiseSlider->setValue(90);
            drumDecaySlider->setValue(120); drumToneSlider->setValue(1000); drumExpSlider->setValue(5); break;
        }
    });

    modeTabs->addTab(drumWidget, "Drum Designer");
    connect(btnGenerateDrum, &QPushButton::clicked, this, &MainWindow::generateDrumXpf);
    connect(btnSaveDrumXpf, &QPushButton::clicked, this, &MainWindow::generateDrumXpf);

    // 10. VELOCILOGIC (DYNAMICS MAPPER)
    QWidget *velTab = new QWidget();
    auto *velLayout = new QVBoxLayout(velTab);

    // 1. Disclaimer
    velDisclaimer = new QLabel("⚠ VELOCILOGIC: DYNAMIC LAYERING.\n"
                               "Checked working with Legacy only.\n"
                               "");
    velDisclaimer->setStyleSheet("QLabel { color: blue; font-weight: bold; font-size: 14px; border: 2px solid blue; padding: 10px; background-color: #eeeeff; }");
    velDisclaimer->setAlignment(Qt::AlignCenter);
    velDisclaimer->setFixedHeight(80);
    velLayout->addWidget(velDisclaimer);

    // 2. Options
    auto *velOptLayout = new QHBoxLayout();
    velOptLayout->addWidget(new QLabel("Build Mode:"));
    velMapMode = new QComboBox();
    velMapMode->addItems({"Nightly (Nested Ternary)", "Legacy (Additive)"});
    velOptLayout->addWidget(velMapMode);
    velOptLayout->addStretch();
    velLayout->addLayout(velOptLayout);

    // 3. The Zone Table
    velMapTable = new QTableWidget();
    velMapTable->setColumnCount(2);
    velMapTable->setHorizontalHeaderLabels({"Upper Velocity Limit (0-127)", "Expression (Code)"});
    velMapTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    velLayout->addWidget(velMapTable);

    // 4. Buttons
    auto *velBtnLayout = new QHBoxLayout();
    auto *btnAddVel = new QPushButton("Add Velocity Zone");
    auto *btnRemVel = new QPushButton("Remove Zone");
    velBtnLayout->addWidget(btnAddVel);
    velBtnLayout->addWidget(btnRemVel);
    velLayout->addLayout(velBtnLayout);

    auto *btnGenVel = new QPushButton("GENERATE VELOCITY MAP");
    btnGenVel->setStyleSheet("font-weight: bold; height: 40px; background-color: #444; color: white;");
    velLayout->addWidget(btnGenVel);

    modeTabs->addTab(velTab, "Velocilogic");

    // Connections
    connect(btnGenVel, &QPushButton::clicked, this, &MainWindow::generateVelocilogic);

    auto addVelZone = [=](int limit, QString code) {
        int r = velMapTable->rowCount();
        velMapTable->insertRow(r);
        velMapTable->setItem(r, 0, new QTableWidgetItem(QString::number(limit)));
        velMapTable->setItem(r, 1, new QTableWidgetItem(code));
    };

    connect(btnAddVel, &QPushButton::clicked, [=](){ addVelZone(100, "pulse(t*f)"); });
    connect(btnRemVel, &QPushButton::clicked, [=](){
        if(velMapTable->rowCount()>0) velMapTable->removeRow(velMapTable->rowCount()-1);
    });

    // Default "Ghost Note" Setup
    // 0-40: Soft Sine (Ghost note)
    // 41-100: Standard Saw
    // 101-127: Hard Square with Noise (Accent)
    addVelZone(40, "sinew(t*f)*0.5");
    addVelZone(100, "saww(t*f)");
    addVelZone(127, "squarew(t*f) + (randv(t)*0.2)");

    // 11. NOISE FORGE
    QWidget *noiseTab = new QWidget(); auto *noiseLayout = new QFormLayout(noiseTab);
    buildModeNoise = new QComboBox(); buildModeNoise->addItems({"Modern", "Legacy"});
    noiseRes = new QDoubleSpinBox(); noiseRes->setRange(100, 44100); noiseRes->setValue(8000);
    noiseLayout->addRow("Build Mode:", buildModeNoise);
    noiseLayout->addRow("Sample-Rate:", noiseRes);
    auto *btnNoise = new QPushButton("Generate Noise Forge");
    noiseLayout->addRow(btnNoise);
    modeTabs->addTab(noiseTab, "Noise Forge");

     // 12. XPF PACKAGER / MANAGER
    QWidget *xpfTab = new QWidget();
    auto *xpfLayout = new QVBoxLayout(xpfTab);

    // 1. Red Disclaimer (O1 Editing Mode)
    xpfDisclaimer = new QLabel("⚠ NOTICE: O1 EDITING MODE.\n"
                               "This tab is a placeholder the tab logic is not complete or tested\n"
                               "This tool packages your code into Oscillator 1 (O1) only.\n"
                               "O2, Filters, and Wavetables (W1) are disabled by default.\n"
                               "Panning is centered.");
    xpfDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    xpfDisclaimer->setAlignment(Qt::AlignCenter);
    xpfDisclaimer->setFixedHeight(80);
    xpfLayout->addWidget(xpfDisclaimer);

    // 2. Input Area
    auto *xpfGroup = new QGroupBox("Expression Source");
    auto *xpfForm = new QVBoxLayout(xpfGroup);

    xpfInput = new QTextEdit();
    xpfInput->setPlaceholderText("Paste your generated Legacy or Nightly code here...");
    xpfForm->addWidget(xpfInput);
    xpfLayout->addWidget(xpfGroup);

    // 3. Save Button
    btnSaveXpf = new QPushButton("Save as Instrument (.xpf)...");
    btnSaveXpf->setStyleSheet("font-weight: bold; height: 50px; font-size: 14px;");
    xpfLayout->addWidget(btnSaveXpf);

    modeTabs->addTab(xpfTab, "XPF Packager");

    // Connect (FIXED: Uses 'btnSaveXpf' and 'saveXpfInstrument')
    connect(btnSaveXpf, &QPushButton::clicked, this, &MainWindow::saveXpfInstrument);

    // 12. FILTER FORGE
    QWidget *filterTab = new QWidget();

    // 1. CHANGE: Main layout is now VERTICAL
    auto *filterMainLayout = new QVBoxLayout(filterTab);

    // 2. NEW: The Red Disclaimer Label
    filterDisclaimer = new QLabel("⚠ DISCLAIMER: EXPERIMENTAL FEATURE.\n"
                                  "Limited success with FIR filters using last(n).\n"
                                  "May produce unexpected audio artifacts.");
    filterDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    filterDisclaimer->setAlignment(Qt::AlignCenter);
    filterDisclaimer->setFixedHeight(80); // Consistent height

    filterMainLayout->addWidget(filterDisclaimer); // Add to top

    // 3. The Form Controls (Sub-Layout)
    auto *filterForm = new QFormLayout(); // This holds the inputs nicely

    buildModeFilter = new QComboBox(); buildModeFilter->addItems({"Modern", "Legacy"});
    filterType = new QComboBox(); filterType->addItems({"Low-Pass", "High-Pass"});
    filterTaps = new QDoubleSpinBox(); filterTaps->setRange(2, 8); filterTaps->setValue(4);

    filterForm->addRow("Build Mode:", buildModeFilter);
    filterForm->addRow("Type:", filterType);
    filterForm->addRow("Taps:", filterTaps);

    auto *btnFil = new QPushButton("Generate Filter");
    filterForm->addRow(btnFil);

    // Add the form to the main layout
    filterMainLayout->addLayout(filterForm);
    // Push everything to the top so it doesn't float in the middle
    filterMainLayout->addStretch();

    modeTabs->addTab(filterTab, "Filter Forge");

    // 13. LEAD STACKER
    QWidget *leadTab = new QWidget(); auto *leadLayout = new QFormLayout(leadTab);
    leadLayout->addRow("Unison Voices:", leadUnisonCount);
    leadLayout->addRow("Detune Amount:", leadDetuneAmount);
    leadLayout->addRow("Wave Type:", leadWaveType);
    auto *btnLead = new QPushButton("Generate Lead Stack");
    leadLayout->addRow(btnLead);
    modeTabs->addTab(leadTab, "Lead Stacker");

    // 14. RANDOMISER
    QWidget *randTab = new QWidget(); auto *randLayout = new QVBoxLayout(randTab);
    randLayout->addWidget(new QLabel("Chaos Level (Randomness):"));
    randLayout->addWidget(chaosSlider);
    auto *btnRand = new QPushButton("GENERATE CHAOS");
    btnRand->setStyleSheet("background-color: #444; color: white; font-weight: bold; height: 50px;");
    randLayout->addWidget(btnRand);
    modeTabs->addTab(randTab, "Randomiser");

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

    // --- 16. LOGIC CONVERTER TAB (NEW) ---
    QWidget *convTab = new QWidget();

    // 1. CHANGE: Main layout is now VERTICAL to stack the warning on top
    auto *mainConvLayout = new QVBoxLayout(convTab);

    // 2. NEW: The Red Disclaimer Label
    convDisclaimer = new QLabel("⚠ DISCLAIMER: CURRENTLY EXPERIMENTAL.\n"
                                "Only works with SHORT PCM samples (approx < 0.1s).\n"
                                "Long files or complex expressions may cause crashes.");
    convDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    convDisclaimer->setAlignment(Qt::AlignCenter);
    convDisclaimer->setFixedHeight(80); // Fixed height to ensure it's visible

    mainConvLayout->addWidget(convDisclaimer); // Add it to the top

    // 3. The Columns (Input - Buttons - Output)
    auto *hLayout = new QHBoxLayout(); // This holds the boxes side-by-side

    // LEFT SIDE (Input)
    auto *leftGroup = new QGroupBox("Input Formula");
    auto *leftLay = new QVBoxLayout(leftGroup);
    convInput = new QTextEdit();
    convInput->setPlaceholderText("Paste Legacy or Nightly code here...");
    leftLay->addWidget(convInput);

    // MIDDLE (Buttons)
    auto *midLayout = new QVBoxLayout();
    btnToNightly = new QPushButton("Legacy\n-->\nNightly");
    btnToLegacy = new QPushButton("Nightly\n-->\nLegacy");

    midLayout->addStretch();
    midLayout->addWidget(btnToNightly);
    midLayout->addWidget(new QLabel(" ")); // Spacer
    midLayout->addWidget(btnToLegacy);
    midLayout->addStretch();

    // RIGHT SIDE (Output)
    auto *rightGroup = new QGroupBox("Converted Result");
    auto *rightLay = new QVBoxLayout(rightGroup);
    convOutput = new QTextEdit();
    convOutput->setReadOnly(true);
    rightLay->addWidget(convOutput);

    // Add columns to the sub-layout
    hLayout->addWidget(leftGroup, 2);
    hLayout->addLayout(midLayout, 0);
    hLayout->addWidget(rightGroup, 2);

    // Add the sub-layout to the main vertical layout
    mainConvLayout->addLayout(hLayout);

    modeTabs->addTab(convTab, "Logic Converter");

    // Connect the buttons (Using the new Universal Logic)
    connect(btnToNightly, &QPushButton::clicked, [=](){
        convOutput->setText(convertLegacyToNightly(convInput->toPlainText()));
    });

    connect(btnToLegacy, &QPushButton::clicked, [=](){
        convOutput->setText(convertNightlyToLegacy(convInput->toPlainText()));
    });

    // 17. KEY MAPPER (KEYBOARD SPLIT)
    QWidget *keyTab = new QWidget();
    auto *keyLayout = new QVBoxLayout(keyTab);

    // 1. Red Disclaimer
    keyMapDisclaimer = new QLabel("⚠ DISCLAIMER: EXPERIMENTAL KEY MAPPING.\n"
                                  "This feature allows splitting logic across the keyboard.\n"
                                  "Requires further development.\n"
                                  "Only tested with legacy");
    keyMapDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    keyMapDisclaimer->setAlignment(Qt::AlignCenter);
    keyMapDisclaimer->setFixedHeight(80);
    keyLayout->addWidget(keyMapDisclaimer);

    // 2. Options
    auto *keyOptLayout = new QHBoxLayout();
    keyOptLayout->addWidget(new QLabel("Build Mode:"));
    keyMapMode = new QComboBox();
    keyMapMode->addItems({"Nightly (Nested Ternary)", "Legacy (Additive)"});
    keyOptLayout->addWidget(keyMapMode);
    keyOptLayout->addStretch();
    keyLayout->addLayout(keyOptLayout);

    // 3. The Zone Table
    keyMapTable = new QTableWidget();
    keyMapTable->setColumnCount(2);
    keyMapTable->setHorizontalHeaderLabels({"Upper Key Limit (0-127)", "Expression (Code)"});
    keyMapTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    keyLayout->addWidget(keyMapTable);

    // 4. Buttons (Add/Remove/Gen)
    auto *keyBtnLayout = new QHBoxLayout();
    auto *btnAddZone = new QPushButton("Add Split Zone");
    auto *btnRemZone = new QPushButton("Remove Zone");
    keyBtnLayout->addWidget(btnAddZone);
    keyBtnLayout->addWidget(btnRemZone);
    keyLayout->addLayout(keyBtnLayout);

    auto *btnGenKey = new QPushButton("GENERATE KEY MAP");
    btnGenKey->setStyleSheet("font-weight: bold; height: 40px; background-color: #444; color: white;");
    keyLayout->addWidget(btnGenKey);

    modeTabs->addTab(keyTab, "Key Mapper");

    // Connections
    connect(btnGenKey, &QPushButton::clicked, this, &MainWindow::generateKeyMapper);

    // Logic to add rows
    auto addZone = [=](int limit, QString code) {
        int r = keyMapTable->rowCount();
        keyMapTable->insertRow(r);
        keyMapTable->setItem(r, 0, new QTableWidgetItem(QString::number(limit)));
        keyMapTable->setItem(r, 1, new QTableWidgetItem(code));
    };

    connect(btnAddZone, &QPushButton::clicked, [=](){ addZone(72, "sinew(t*f)"); });
    connect(btnRemZone, &QPushButton::clicked, [=](){
        if(keyMapTable->rowCount()>0) keyMapTable->removeRow(keyMapTable->rowCount()-1);
    });

    // Add Default Splits (Bass / Lead)
    addZone(60, "saww(t*f*0.5)"); // Bass (Below Middle C)
    addZone(128, "squarew(t*f)");   // Lead (Everything else)

    // 18 STEP GATE
    QWidget *gateTab = new QWidget();
    QVBoxLayout *gateLayout = new QVBoxLayout(gateTab);

    QLabel *gateDisclaimer = new QLabel("⚠ DISCLAIMER: INCOMPLETE FEATURE.\n"
                                        "Only Legacy gate logic is currently working.");
    gateDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    gateDisclaimer->setAlignment(Qt::AlignCenter);
    gateDisclaimer->setFixedHeight(80);
    gateLayout->addWidget(gateDisclaimer);

    // Controls (Build Mode, Speed, Triplet, Mix)
    QHBoxLayout *gateCtrlLayout = new QHBoxLayout();
    gateBuildMode = new QComboBox(); gateBuildMode->addItems({"Nightly (Variables)", "Legacy (Inline)"});
    gateSpeedCombo = new QComboBox(); gateSpeedCombo->addItems({"1/2 Speed (Slow)", "1x (Synced)", "2x (Fast)", "4x (Hyper)"});
    gateSpeedCombo->setCurrentIndex(1);
    gateTripletCheck = new QCheckBox("Triplet Mode (3/2)");
    gateMixSlider = new QSlider(Qt::Horizontal); gateMixSlider->setRange(0, 100); gateMixSlider->setValue(100);

    gateCtrlLayout->addWidget(new QLabel("Build:")); gateCtrlLayout->addWidget(gateBuildMode);
    gateCtrlLayout->addWidget(new QLabel("Speed:")); gateCtrlLayout->addWidget(gateSpeedCombo);
    gateCtrlLayout->addWidget(gateTripletCheck);
    gateCtrlLayout->addWidget(new QLabel("Mix:")); gateCtrlLayout->addWidget(gateMixSlider);
    gateLayout->addLayout(gateCtrlLayout);

    // 16-Step Grid Buttons
    QGridLayout *gateGrid = new QGridLayout();
    gateGrid->setSpacing(4);
    for(int i=0; i<16; ++i) {
        gateSteps[i] = new QPushButton(QString::number(i+1));
        gateSteps[i]->setCheckable(true);
        gateSteps[i]->setFixedSize(45, 40);
        // Styling: Red (OFF) -> Green (ON)
        gateSteps[i]->setStyleSheet(
            "QPushButton { background-color: #441111; color: #ff9999; border: 1px solid #552222; border-radius: 4px; }"
            "QPushButton:checked { background-color: #00ee00; color: black; border: 1px solid #00aa00; font-weight: bold; }"
            );
        // Default pattern: (Every 3rd step roughly)
        if(i == 0 || i == 2 || i == 3 || i == 6 || i == 8 || i == 10 || i == 11 || i == 14)
            gateSteps[i]->setChecked(true);

        int row = i / 8; // 2 rows of 8
        int col = i % 8;
        gateGrid->addWidget(gateSteps[i], row, col);
    }
    gateLayout->addLayout(gateGrid);

    // Waveform Selector
    QFormLayout *gateForm = new QFormLayout();
    gateShapeCombo = new QComboBox();
    gateShapeCombo->addItems({"Square Wave (Basic)", "Sawtooth (Sharp)", "Sine Wave (Soft)", "Noise (Perc)", "Custom (Paste Below)"});
    gateCustomShape = new QTextEdit();
    gateCustomShape->setPlaceholderText("Paste custom formula here if 'Custom' selected (use 'f' for freq)...");
    gateCustomShape->setMaximumHeight(60);

    gateForm->addRow("Source Wave:", gateShapeCombo);
    gateForm->addRow("Custom Code:", gateCustomShape);
    gateLayout->addLayout(gateForm);

    // Generate Button
    QPushButton *btnGenGate = new QPushButton("GENERATE STEP GATE");
    btnGenGate->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 50px; font-size: 14px;");
    gateLayout->addWidget(btnGenGate);

    modeTabs->addTab(gateTab, "Step Gate");

    connect(btnGenGate, &QPushButton::clicked, this, &MainWindow::generateStepGate);

    // 19. --- NUMBERS 1981 TAB ---
    QWidget *numTab = new QWidget();
    auto *numLayout = new QVBoxLayout(numTab);

    // 1. Top Controls
    auto *numCtrlLayout = new QHBoxLayout();
    numModeCombo = new QComboBox(); numModeCombo->addItems({"Random Stream", "Pattern Editor"});
    numStepsCombo = new QComboBox(); numStepsCombo->addItems({"16 Steps", "32 Steps"}); numStepsCombo->setCurrentIndex(1); // Default 32
    numDuration = new QDoubleSpinBox(); numDuration->setRange(0.01, 1.0); numDuration->setValue(0.2); numDuration->setSingleStep(0.05);

    numCtrlLayout->addWidget(new QLabel("Mode:")); numCtrlLayout->addWidget(numModeCombo);
    numCtrlLayout->addWidget(new QLabel("Steps:")); numCtrlLayout->addWidget(numStepsCombo);
    numCtrlLayout->addWidget(new QLabel("Note Dur:")); numCtrlLayout->addWidget(numDuration);
    numLayout->addLayout(numCtrlLayout);

    // 2. Pattern Editor (Table)
    numPatternTable = new QTableWidget();
    numPatternTable->setRowCount(1);
    numPatternTable->setColumnCount(32); // Max 32
    numPatternTable->setFixedHeight(70);
    numPatternTable->verticalHeader()->setVisible(false);
    numPatternTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Fill with default zeros
    for(int i=0; i<32; ++i) numPatternTable->setItem(0, i, new QTableWidgetItem("0"));

    numLayout->addWidget(new QLabel("<b>Pattern Editor:</b> (Semitones +/- 12). Only used if 'Pattern Editor' mode selected."));
    numLayout->addWidget(numPatternTable);

    // 3. Outputs (O1 & O2)
    auto *numOutLayout = new QHBoxLayout();
    auto *g1 = new QGroupBox("O1: Pan Left (Main)"); auto *l1 = new QVBoxLayout(g1);
    numOut1 = new QTextEdit(); l1->addWidget(numOut1);

    auto *g2 = new QGroupBox("O2: Pan Right (Detuned+Vib)"); auto *l2 = new QVBoxLayout(g2);
    numOut2 = new QTextEdit(); l2->addWidget(numOut2);

    numOutLayout->addWidget(g1);
    numOutLayout->addWidget(g2);
    numLayout->addLayout(numOutLayout);

    // 4. Generate Button
    auto *btnGenNum = new QPushButton("GENERATE NUMBERS 1981");
    btnGenNum->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 40px;");
    numLayout->addWidget(btnGenNum);

    modeTabs->addTab(numTab, "Numbers 1981");
    connect(btnGenNum, &QPushButton::clicked, this, &MainWindow::generateNumbers1981);

    // Logic to hide/show pattern table based on mode
    connect(numModeCombo, &QComboBox::currentIndexChanged, [=](int idx){
        numPatternTable->setVisible(idx == 1);
    });
    // Default visibility check
    numPatternTable->setVisible(false);

    // --- INSERT IN mainwindow.cpp (Inside setupUI) ---

    // 20. DELAY ARCHITECT
    QWidget *delayTab = new QWidget();
    auto *delayLayout = new QVBoxLayout(delayTab);

    // Disclaimer
    QLabel *delayWarn = new QLabel("⚠ DISCLAIMER: LEGACY / PLACEHOLDER\n"
                                   "This feature is not working properly yet.\n"
                                   "Note: handwritten code clamp(-1, (trianglew(integrate(f)) * exp(-t * 20))+(0.6 * last(8000) * (t > 0.2)), 1) worked");

    // Style it red
    delayWarn->setStyleSheet("QLabel { color: red; font-weight: bold; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    delayWarn->setAlignment(Qt::AlignCenter);
    delayWarn->setFixedHeight(80);

    delayLayout->addWidget(delayWarn);

    // Controls
    auto *delayForm = new QFormLayout();

    delayWaveCombo = new QComboBox();
    delayWaveCombo->addItems({"Plucky Triangle (Default)", "Sawtooth Sweep", "Simple Square", "Custom (Below)"});

    delayCustomInput = new QTextEdit();
    delayCustomInput->setPlaceholderText("Paste custom source here (e.g., sinew(integrate(f)))...");
    delayCustomInput->setMaximumHeight(60);

    delayTimeSpin = new QDoubleSpinBox();
    delayTimeSpin->setRange(0.01, 2.0); delayTimeSpin->setValue(0.2); delayTimeSpin->setSuffix(" s");

    delayRateSpin = new QDoubleSpinBox();
    delayRateSpin->setRange(1000, 44100); delayRateSpin->setValue(8000); delayRateSpin->setSuffix(" Hz");

    delayFeedbackSpin = new QDoubleSpinBox();
    delayFeedbackSpin->setRange(0.1, 0.99); delayFeedbackSpin->setValue(0.6); delayFeedbackSpin->setSingleStep(0.1);

    delayTapsSpin = new QSpinBox();
    delayTapsSpin->setRange(1, 16); delayTapsSpin->setValue(4);

    delayForm->addRow("Source:", delayWaveCombo);
    delayForm->addRow("Custom Code:", delayCustomInput);
    delayForm->addRow("Delay Time:", delayTimeSpin);
    delayForm->addRow("Sample Rate:", delayRateSpin); // Needed to calc 'last(n)'
    delayForm->addRow("Feedback:", delayFeedbackSpin);
    delayForm->addRow("Echo Count:", delayTapsSpin);

    delayLayout->addLayout(delayForm);

    auto *btnGenDelay = new QPushButton("GENERATE DELAY CHAIN");
    btnGenDelay->setStyleSheet("font-weight: bold; height: 40px; background-color: #444; color: white;");
    delayLayout->addWidget(btnGenDelay);

    modeTabs->addTab(delayTab, "Delay Architect");

    connect(btnGenDelay, &QPushButton::clicked, this, &MainWindow::generateDelayArchitect);

    // 21. --- MACRO MORPH
    QWidget *macroTab = new QWidget;
    QVBoxLayout *macroLayout = new QVBoxLayout(macroTab);

    // Header
    QLabel *fluxDesc = new QLabel("<b></b> \n"
                                  "Use 'Texture' for vinyl noise and 'Wonk' for off-grid swing.");
    fluxDesc->setStyleSheet("color: #333; padding: 10px; background-color: #eee; border-radius: 5px;");
    macroLayout->addWidget(fluxDesc);

    // --- TOP BAR: BUILD MODE ---
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("Parsing Engine:"));

    macroBuildMode = new QComboBox();
    macroBuildMode->addItems({"Nightly (Clean / Variables)", "Legacy (Inline / Safe)"});
    macroBuildMode->setToolTip("Legacy Mode removes variables for compatibility with older engines.");
    topBar->addWidget(macroBuildMode);
    topBar->addStretch();
    macroLayout->addLayout(topBar);

    // --- GROUP 1: PRESET SELECTOR (Renamed) ---
    QGroupBox *sourceGroup = new QGroupBox("1. Select Vibe");
    sourceGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #666; margin-top: 10px; }");
    QVBoxLayout *sourceLay = new QVBoxLayout(sourceGroup);

    macroStyleCombo = new QComboBox;
    macroStyleCombo->addItems({
        "0. Super Saws (Anthemic)",
        "1. Formant Vocal Lead (Chops)",
        "2. Wobbly Cassette Keys (Lo-Fi)",
        "3. Granular Pad (Jitter)",
        "4. Hollow Bass (Deep House)",
        "5. Portamento Lead (Gliding)",
        "6. Plucky Arp (Short)",
        "7. Vinyl Atmosphere (Texture Only)"
    });
    sourceLay->addWidget(macroStyleCombo);
    macroLayout->addWidget(sourceGroup);

    // --- GROUP 2: THE MACRO CONSOLE ---
    QGroupBox *macroGroup = new QGroupBox("2. Macro Controls");
    macroGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #008080; background-color: #f4fcfc; margin-top: 10px; }");
    QGridLayout *macroGrid = new QGridLayout(macroGroup);

    // Helper to make sliders
    auto addMacro = [&](QString name, QSlider*& slider, int row, int col, int def) {
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100); slider->setValue(def);
        macroGrid->addWidget(new QLabel(name), row * 2, col);
        macroGrid->addWidget(slider, row * 2 + 1, col);
    };

    // Column 1: Tonal Character
    addMacro("Color (Timbre)", macroColorSlider, 0, 0, 50);
    addMacro("Texture (Noise/Grain)", macroTextureSlider, 1, 0, 20);
    addMacro("Bitcrush (Lo-Fi)", macroBitcrushSlider, 2, 0, 0); // Renamed Label

    // Column 2: Space & Time
    addMacro("Time (Envelope)", macroTimeSlider, 0, 1, 50);
    addMacro("Width (Stereo/Detune)", macroWidthSlider, 1, 1, 30);
    addMacro("Wonk (Sidechain/Swing)", macroWonkySlider, 2, 1, 25);

    macroLayout->addWidget(macroGroup);

    // GENERATE BUTTON
    QPushButton *btnGenMacro = new QPushButton("GENERATE FUTURE PATCH");
    btnGenMacro->setStyleSheet("font-weight: bold; font-size: 14px; background-color: #008080; color: white; height: 50px; margin-top: 10px;");
    connect(btnGenMacro, &QPushButton::clicked, this, &MainWindow::generateMacroMorph);
    macroLayout->addWidget(btnGenMacro);

    macroLayout->addStretch();
    modeTabs->addTab(macroTab, "Macro Morph");

    // 22. --- TAB: STRING MACHINE (Evolving & Cinematic) ---
    QWidget *stringTab = new QWidget;
    QVBoxLayout *stringLayout = new QVBoxLayout(stringTab);

    QLabel *strDesc = new QLabel("<b>\n"
                                 "");
    strDesc->setStyleSheet("color: #333; padding: 10px; background-color: #e6f7ff; border-radius: 5px;");
    stringLayout->addWidget(strDesc);

    // --- GROUP 1: MODEL & CHORD ---
    QGroupBox *strModelGroup = new QGroupBox("1. Core Sound");
    strModelGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #446688; margin-top: 10px; }");
    QFormLayout *strModelForm = new QFormLayout(strModelGroup);

    stringModelCombo = new QComboBox;
    stringModelCombo->addItems({
        "Solina String Ensemble (Classic)",
        "Crumar Performer (Brassy)",
        "Logan String Melody (Hollow)",
        "$tinkworx Aquatic Pad (Deep/PWM)",  // NEW: Stinkworx Preset
        "Roland VP-330 (Choral)",
        "Amazing String (Saw Stack)"
    });

    stringChordCombo = new QComboBox;
    stringChordCombo->addItems({
        "OFF (Manual Play)",
        "Octave Stack (8' + 4')",
        "Fifth Stack (Power Chord)",
        "Minor 9th (Amazing Stack)", // Your favorite
        "$tinkworx Minor 11 (Deep)", // New chord type for deep techno
        "Sus4 (Spacey)"
    });

    strModelForm->addRow("Model Inspiration:", stringModelCombo);
    strModelForm->addRow("Chord Memory:", stringChordCombo);
    stringLayout->addWidget(strModelGroup);

    // --- GROUP 2: EVOLUTION DASHBOARD ---
    QGroupBox *strDashGroup = new QGroupBox("2. Evolution & Motion");
    strDashGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #446688; background-color: #f0f8ff; margin-top: 10px; }");
    QGridLayout *strGrid = new QGridLayout(strDashGroup);

    auto addStrSlider = [&](QString name, QSlider*& slider, int row, int col, int def) {
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100); slider->setValue(def);
        strGrid->addWidget(new QLabel(name), row * 2, col);
        strGrid->addWidget(slider, row * 2 + 1, col);
    };

    // Row 1: The "Visual Fix" and Chorus
    addStrSlider("Ensemble (Width)", stringEnsembleSlider, 0, 0, 60);
    addStrSlider("Phase Motion (Visual Fix)", stringMotionSlider, 0, 1, 20); // Makes the scope dance

    // Row 2: Evolution (The logic you asked for)
    addStrSlider("Attack (Vol Swell)", stringAttackSlider, 1, 0, 40);
    addStrSlider("Evolve (Filter Swell)", stringEvolveSlider, 1, 1, 50); // Brightness over time

    // Row 3: Character
    addStrSlider("Vintage Age (Wobble)", stringAgeSlider, 2, 0, 10);
    addStrSlider("Space (Release)", stringSpaceSlider, 2, 1, 50);

    stringLayout->addWidget(strDashGroup);

    QPushButton *btnGenStr = new QPushButton("GENERATE STRING MACHINE");
    btnGenStr->setStyleSheet("font-weight: bold; font-size: 14px; background-color: #446688; color: white; height: 50px; margin-top: 10px;");
    connect(btnGenStr, &QPushButton::clicked, this, &MainWindow::generateStringMachine);
    stringLayout->addWidget(btnGenStr);

    stringLayout->addStretch();
    modeTabs->addTab(stringTab, "String Machine");

    // --- 23. HARDWARE LAB (ANALOG SYNTHESIS SUITE) ---
    QWidget *hwTab = new QWidget();
    QVBoxLayout *hwLayout = new QVBoxLayout(hwTab);

    QLabel *hwDisclaimer = new QLabel("⚠ HARDWARE LAB: DIRECT PARAMETER CONTROL\n"
                                      "This module maps 40 Analog-Style Presets. Legacy. WIP, testing external ADSR");
    hwDisclaimer->setStyleSheet("QLabel { color: #00ff78; font-weight: bold; font-size: 14px; border: 2px solid #00ff78; padding: 10px; background-color: #112211; }");
    hwDisclaimer->setAlignment(Qt::AlignCenter);
    hwLayout->addWidget(hwDisclaimer);

    adsrVisualizer = new EnvelopeDisplay();
    hwLayout->addWidget(adsrVisualizer);

    QFormLayout *hwForm = new QFormLayout();

    // --- POPULATE THE HARDWARE LIBRARY ---
    hwPresetCombo = new QComboBox();
    hwPresetCombo->addItem("-- STUDIO CLASSICS --");
    hwPresetCombo->addItems({"01. Hissing Minimal (Signal)", "02. Analog Drift (Precision)", "03. Resonance Burner (Peak)",
                             "04. Metallic Tick (Percussion)", "05. PWM Rubber (Low-End)", "06. Power Saw (Lead)",
                             "07. Phase Mod (Keys)", "08. Deep Atmosphere (Pad)"});

    hwPresetCombo->addItem("-- MODULAR MINIMAL --");
    for(int i=9; i<=16; ++i) hwPresetCombo->addItem(QString("%1. Minimal Studio Tool %2").arg(i).arg(i-8));

    hwPresetCombo->addItem("-- INDUSTRIAL WAREHOUSE --");
    for(int i=17; i<=24; ++i) hwPresetCombo->addItem(QString("%1. Industrial Grit %2").arg(i).arg(i-16));

    hwPresetCombo->addItem("-- ETHEREAL DRIFT --");
    for(int i=25; i<=32; ++i) hwPresetCombo->addItem(QString("%1. Signal Drift %2").arg(i).arg(i-24));

    hwPresetCombo->addItem("-- SIGNAL GLITCH --");
    for(int i=33; i<=40; ++i) hwPresetCombo->addItem(QString("%1. Frequency Glitch %2").arg(i).arg(i-32));

    hwBaseWave = new QComboBox();
    hwBaseWave->addItems({"saww", "squarew", "trianglew", "sinew"});

    hwAttack = new QSlider(Qt::Horizontal); hwAttack->setRange(0, 100);
    hwDecay = new QSlider(Qt::Horizontal); hwDecay->setRange(0, 100);
    hwSustain = new QSlider(Qt::Horizontal); hwSustain->setRange(0, 100);
    hwRelease = new QSlider(Qt::Horizontal); hwRelease->setRange(0, 100);
    hwCutoff = new QSlider(Qt::Horizontal); hwCutoff->setRange(100, 14000);
    hwResonance = new QSlider(Qt::Horizontal); hwResonance->setRange(0, 100);

    hwPwmSpeed = new QSlider(Qt::Horizontal); hwPwmSpeed->setRange(0, 100);
    hwPwmDepth = new QSlider(Qt::Horizontal); hwPwmDepth->setRange(0, 100);
    hwVibSpeed = new QSlider(Qt::Horizontal); hwVibSpeed->setRange(0, 100);
    hwVibDepth = new QSlider(Qt::Horizontal); hwVibDepth->setRange(0, 100);
    hwNoiseMix = new QSlider(Qt::Horizontal); hwNoiseMix->setRange(0, 100);

    hwBaseNote = new QSpinBox(); hwBaseNote->setRange(0, 127); hwBaseNote->setValue(57); // Middle A
    hwPeakBoost = new QCheckBox("Resonance Peak Boost (Saturator)");

    hwForm->addRow("Preset Library:", hwPresetCombo);
    hwForm->addRow("Oscillator Wave:", hwBaseWave);
    hwForm->addRow("Attack Time:", hwAttack);
    hwForm->addRow("Decay Time:", hwDecay);
    hwForm->addRow("Sustain Level:", hwSustain);
    hwForm->addRow("Release Time:", hwRelease);
    hwForm->addRow("Filter Frequency:", hwCutoff);
    hwForm->addRow("Filter Q/Res:", hwResonance);
    hwForm->addRow("PWM LFO Speed:", hwPwmSpeed);
    hwForm->addRow("PWM LFO Depth:", hwPwmDepth);
    hwForm->addRow("Pitch Vibrato Speed:", hwVibSpeed);
    hwForm->addRow("Pitch Vibrato Depth:", hwVibDepth);
    hwForm->addRow("Signal Noise Mix:", hwNoiseMix);
    hwForm->addRow("Base MIDI Note:", hwBaseNote);
    hwForm->addRow(hwPeakBoost);

    hwLayout->addLayout(hwForm);

    QHBoxLayout *hwBtnLayout = new QHBoxLayout();
    QPushButton *btnRandHw = new QPushButton("RANDOMIZE HARDWARE");
    QPushButton *btnSaveHw = new QPushButton("SAVE PATCH .XPF");
    hwBtnLayout->addWidget(btnRandHw);
    hwBtnLayout->addWidget(btnSaveHw);
    hwLayout->addLayout(hwBtnLayout);

    modeTabs->addTab(hwTab, "Hardware Lab");

    // Live Visualizer Connection
    auto updateHwPreview = [=]() {
        adsrVisualizer->updateEnvelope(
            hwAttack->value() / 100.0,
            hwDecay->value() / 100.0,
            hwSustain->value() / 100.0,
            hwRelease->value() / 100.0
            );
    };

    connect(hwAttack, &QSlider::valueChanged, updateHwPreview);
    connect(hwDecay, &QSlider::valueChanged, updateHwPreview);
    connect(hwSustain, &QSlider::valueChanged, updateHwPreview);
    connect(hwRelease, &QSlider::valueChanged, updateHwPreview);
    connect(hwPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::loadHardwarePreset);
    connect(btnRandHw, &QPushButton::clicked, this, &MainWindow::generateRandomHardware);
    connect(btnSaveHw, &QPushButton::clicked, this, &MainWindow::generateHardwareXpf);


    // 24. NEED TO KNOW / NOTES TAB
    QWidget *notesTab = new QWidget();
    auto *notesLayout = new QVBoxLayout(notesTab);

    QTextEdit *notesText = new QTextEdit();
    notesText->setReadOnly(true);

    // HTML Formatting for clarity
    notesText->setHtml(
        "<h2 style='color:#333;'>Project Status & Limitations</h2>"
        "<p><b>Current Version:</b> Experimental Build</p>"
        "<hr>"

        "<h3 style='color:red;'>CRITICAL LIMITATIONS</h3>"
        "<ul>"
        "<li><b>NO ADSR Shaping:</b> The generated code does not automate the Instrument Envelope (Attack, Decay, Sustain, Release). You must program these expressions yourself or set these knobs manually in the Instrument Editor.</li>"
        "<li><b>O1 Only:</b> The XPF Packager and Generators only output code for <b>Oscillator 1 (O1)</b>. O2, W1, W2, and W3 are disabled or ignored.</li>"
        "<li><b>Filters are Manual:</b> The 'Filter Forge' aims to generate a mathematical approximation of a filter if FIR is resolved, but it does <b>not</b> control the actual Filter Section (Cutoff/Resonance) of the instrument.</li>"
        "</ul>"


        );

    notesLayout->addWidget(notesText);
    modeTabs->addTab(notesTab, "Need to Know");


    // Above here for interfaces

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
    connect(btnGenVel, &QPushButton::clicked, this, &MainWindow::generateVelocilogic);
    connect(btnNoise, &QPushButton::clicked, this, &MainWindow::generateNoiseForge);
    connect(btnSaveXpf, &QPushButton::clicked, this, &MainWindow::saveXpfInstrument);
    connect(btnFil, &QPushButton::clicked, this, &MainWindow::generateFilterForge);
    connect(btnLead, &QPushButton::clicked, this, &MainWindow::generateLeadStack);
    connect(btnRand, &QPushButton::clicked, this, &MainWindow::generateRandomPatch);
}

// The workings ------------------------


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
    // 1. CALCULATE SPEED
    double hz = 0;
    if (arpBpmSync->isChecked()) {
        double bpm = arpBpmVal->value();
        // 1/16th note = BPM / 60 * 4
        // Hubbard Speed (1/64) is 4x faster than that
        int divIdx = arpSpeedDiv->currentIndex();
        if (divIdx == 4) hz = 50.0; // PAL Frame (Classic C64 standard)
        else {
            double multiplier = (divIdx == 0) ? 4.0 : (divIdx == 1) ? 8.0 : (divIdx == 2) ? 12.0 : 16.0;
            hz = (bpm / 60.0) * multiplier;
        }
    } else {
        hz = arpSpeed->value();
    }

    // 2. GET INTERVALS (Parse the combobox text)
    // Helper to extract number from string like "+7 (Perfect 5th)"
    auto getSemi = [](QString s) {
        return s.split(" ")[0].toInt();
    };
    int note1 = 0; // Root is always 0
    int note2 = getSemi(arpInterval1->currentText());
    int note3 = getSemi(arpInterval2->currentText());

    // 3. GENERATE WAVEFORM LOGIC
    QString waveName = arpWave->currentText();
    double pwm = arpPwmSlider->value() / 100.0;

    // We define a lambda to generate the "Audio" part for a specific pitch
    auto genAudio = [&](QString pitchMult) {
        if (waveName.contains("Pulse")) {
            // Pulse Width formula: (sin(f) > width ? 1 : -1)
            // We scale PWM to -1..1 range for the engine
            return QString("(sinew(integrate(f*%1)) > %2 ? 1 : -1)")
                .arg(pitchMult).arg((pwm * 2.0) - 1.0);
        }
        else if (waveName.contains("Metal")) {
            // Ring Mod: Square * Detuned Square
            return QString("(squarew(integrate(f*%1)) * squarew(integrate(f*%1*2.41)))")
                .arg(pitchMult);
        }
        else if (waveName.contains("Noise")) {
            return QString("randv(t*10000)"); // Noise ignores pitch usually
        }
        else {
            QString osc = waveName.contains("Saw") ? "saww" : "trianglew";
            return QString("%1(integrate(f*%2))").arg(osc).arg(pitchMult);
        }
    };

    QString finalExpr;

    // 4. BUILD THE ARP (Nested vs Legacy)

    // Calculate Pitch Multipliers: 2^(semitones/12)
    QString p1 = "1.0";
    QString p2 = QString::number(std::pow(2.0, note2 / 12.0), 'f', 4);
    QString p3 = QString::number(std::pow(2.0, note3 / 12.0), 'f', 4);

    if (buildModeArp->currentIndex() == 0) { // NIGHTLY (Clean Switching)
        // Logic: var step := floor(t * speed) % 3;
        // (step == 0 ? Note1 : (step == 1 ? Note2 : Note3))

        QString selector = QString("mod(floor(t*%1), 3)").arg(hz);

        finalExpr = QString("(%1 < 1 ? %2 : (%1 < 2 ? %3 : %4))")
                        .arg(selector)
                        .arg(genAudio(p1))
                        .arg(genAudio(p2))
                        .arg(genAudio(p3));

    } else { // LEGACY (Additive)
        // Logic: ((Phase 0) * Note1) + ((Phase 1) * Note2) ...
        // We use mod(floor(t*spd), 3) to find the current step index

        QString stepCheck = QString("mod(floor(t*%1), 3)").arg(hz);

        finalExpr = QString("((%1 < 1) * %2) + ((%1 >= 1 & %1 < 2) * %3) + ((%1 >= 2) * %4)")
                        .arg(stepCheck)
                        .arg(genAudio(p1))
                        .arg(genAudio(p2))
                        .arg(genAudio(p3));
    }

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(finalExpr));
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
    int rows = velMapTable->rowCount();
    if (rows == 0) return;

    QString finalFormula;

    // --- MODE A: NIGHTLY (Nested Ternary) ---
    if (velMapMode->currentIndex() == 0) {
        QString nestedBody = "0";
        int startIdx = rows - 1;

        // Base case optimization
        if (velMapTable->item(startIdx, 0)->text().toInt() >= 127) {
            nestedBody = velMapTable->item(startIdx, 1)->text();
            startIdx--;
        }

        for (int i = startIdx; i >= 0; --i) {
            // Convert MIDI (0-127) to Volume (0.0-1.0)
            double rawLimit = velMapTable->item(i, 0)->text().toDouble();
            double normLimit = rawLimit / 127.0;

            QString code = velMapTable->item(i, 1)->text();

            // USE 'v' INSTEAD OF 'vel'
            nestedBody = QString("(v < %1 ? %2 : %3)")
                             .arg(normLimit, 0, 'f', 3)
                             .arg(code)
                             .arg(nestedBody);
        }
        finalFormula = nestedBody;
    }

    // --- MODE B: LEGACY (Additive) ---
    else {
        QStringList segments;
        double lowerBound = 0.0;

        for (int i = 0; i < rows; ++i) {
            // Convert MIDI (0-127) to Volume (0.0-1.0)
            double rawLimit = velMapTable->item(i, 0)->text().toDouble();
            double upperBound = rawLimit / 127.0;
            QString code = velMapTable->item(i, 1)->text();

            QString rangeCheck;

            // 1. First Zone
            if (i == 0 && lowerBound <= 0.001) {
                rangeCheck = QString("(v < %1)").arg(upperBound, 0, 'f', 3);
            }
            // 2. Last Zone
            else if (i == rows - 1 && rawLimit >= 127) {
                rangeCheck = QString("(v >= %1)").arg(lowerBound, 0, 'f', 3);
            }
            // 3. Middle Zones (Use * for AND)
            else {
                rangeCheck = QString("((v >= %1) * (v < %2))")
                .arg(lowerBound, 0, 'f', 3)
                    .arg(upperBound, 0, 'f', 3);
            }

            segments << QString("(%1 * (%2))").arg(rangeCheck).arg(code);
            lowerBound = upperBound;
        }
        finalFormula = segments.join(" + ");
    }

    QString result = QString("clamp(-1, %1, 1)").arg(finalFormula);
    statusBox->setText(result);
    QApplication::clipboard()->setText(result);
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


// --- NEW IMPLEMENTATION: PHONETIC LAB (SAM) ---

void MainWindow::initSamLibrary() {
    // Phonemes mapped from SAM source tables.
    // https://github.com/s-macke/SAM
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

// ---------------------------------------------------------
// HELPER: Finds a character but ignores anything inside brackets
// Critical for not splitting math like "sin(t+1)"
// ---------------------------------------------------------
int findScopeAwareChar(const QString &str, char target) {
    int balance = 0;
    for (int i = 0; i < str.length(); ++i) {
        if (str[i] == '(') balance++;
        else if (str[i] == ')') balance--;
        else if (str[i] == target && balance == 0) return i;
    }
    return -1;
}

// ---------------------------------------------------------
// FUNCTION 1: Legacy (Time t) --> Nightly (Samples s)
// * Converts "t < 0.1" to "s <= 799" *
// ---------------------------------------------------------
// ---------------------------------------------------------
// FUNCTION 1: Legacy (Chain) --> Nightly (Balanced Tree)
// * Solves the "Stack Overflow" / Bracket Pile-up error *
// ---------------------------------------------------------
// ---------------------------------------------------------
// FUNCTION 1: Universal "Make it Nightly"
// * Accepts 't' (Time) OR 's' (Samples)
// * Outputs perfectly balanced 'var s' tree
// ---------------------------------------------------------
// ---------------------------------------------------------
// FUNCTION 1: Universal "Make it Nightly"
// * Uses RECURSION to handle both Trees and Chains
// * Digs out the true values and rebuilds a clean 'var s' tree
// ---------------------------------------------------------
// ---------------------------------------------------------
// FUNCTION 1: Legacy (Time) --> Nightly (Samples)
// * Auto-Detects Sample Rate (2000 vs 8000)
// * Preserves your Tree Brackets perfectly
// * Removes "clamp"
// ---------------------------------------------------------
QString MainWindow::convertLegacyToNightly(QString input) {
    // 1. Clean formatting
    input = input.replace("\n", "").replace(" ", "").trimmed();

    // Remove "clamp" wrapper if present (User requested raw output)
    if (input.startsWith("clamp(")) {
        int c = input.indexOf(','), p = input.lastIndexOf(')');
        if (c != -1 && p != -1) input = input.mid(c + 1, p - c - 1);
    }
    if (input.startsWith("0.000+")) input = input.mid(6);

    // 2. AUTO-DETECT SAMPLE RATE
    // We scan the text for the smallest non-zero number.
    // Example: If we see "0.000500", we know Rate = 1 / 0.0005 = 2000.
    double sampleRate = 8000.0; // Default fallback

    QRegularExpression floatReg("0\\.00[0-9]+"); // Look for small decimals
    auto matches = floatReg.globalMatch(input);
    double minVal = 1.0;
    bool foundAny = false;

    while (matches.hasNext()) {
        double val = matches.next().captured(0).toDouble();
        if (val > 0.000001 && val < minVal) {
            minVal = val;
            foundAny = true;
        }
    }
    if (foundAny) {
        // Round to nearest hundred to avoid precision errors (e.g., 1999.99 -> 2000)
        sampleRate = std::round(1.0 / minVal);
    }

    // 3. TREE PRESERVATION MODE
    // If the input contains nested '?' (like your example),
    // we do NOT rebuild the tree. We just regex-swap 't' for 's'.
    // This keeps your brackets 100% identical.
    if (input.contains("?")) {

        static QRegularExpression tReg("t<([0-9\\.]+)");
        QString result;
        int lastPos = 0;

        auto it = tReg.globalMatch(input);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();

            // Keep the text structure exactly as is
            result.append(input.mid(lastPos, match.capturedStart() - lastPos));

            // MATH: Convert Time -> Sample
            // Formula: s <= floor(t * rate) - 1
            double tVal = match.captured(1).toDouble();
            int sVal = std::floor(tVal * sampleRate) - 1;
            if (sVal < 0) sVal = 0;

            // Swap "t < 0.005" with "s <= 9"
            result.append(QString("s<=%1").arg(sVal));

            lastPos = match.capturedEnd();
        }
        result.append(input.mid(lastPos));

        // Output correct header, NO CLAMP
        return QString("var s:=floor(t*%1);\n%2")
            .arg(int(sampleRate))
            .arg(result);
    }

    // 4. CHAIN MODE (Fallback)
    // Only used if you give it a flat chain (SID/Arp)
    // ... (Your previous flattening logic could go here if needed,
    // but for PCM trees, the block above handles it perfectly) ...

    return "Error: Input format not recognized (Try standard Legacy PCM).";
}

// ---------------------------------------------------------
// FUNCTION 2: Nightly (Samples s) --> Legacy (Time t)
// * Converts "s <= 799" to "t < 0.1" *
// ---------------------------------------------------------
QString MainWindow::convertNightlyToLegacy(QString input) {
    input = input.replace("\n", "").replace(" ", "").trimmed();
    if (input.startsWith("clamp(")) {
        int c = input.indexOf(','), p = input.lastIndexOf(')');
        if (c != -1 && p != -1) input = input.mid(c + 1, p - c - 1);
    }

    // --- MODE A: PCM Tree Detected ("var s") ---
    // This is the main one used for PCM files
    static QRegularExpression pcmVar("var\\s*s\\s*:=\\s*floor\\(t\\s*\\*\\s*([0-9]+)\\);");
    auto pcmMatch = pcmVar.match(input);

    if (pcmMatch.hasMatch()) {
        double sr = pcmMatch.captured(1).toDouble();
        if (sr == 0) sr = 8000;
        input.remove(pcmMatch.captured(0)); // Remove var declaration

        // Replace all "s <= 123" with "t < 0.015"
        static QRegularExpression sReg("s\\s*<=\\s*([0-9]+)");
        QString result;
        int lastPos = 0;
        auto it = sReg.globalMatch(input);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            result.append(input.mid(lastPos, m.capturedStart() - lastPos));

            // MATH: Convert Sample Index -> Time
            // (Sample + 1) / Rate is the start time of the NEXT sample
            double tVal = (m.captured(1).toDouble() + 1.0) / sr;
            result.append(QString("t < %1").arg(QString::number(tVal, 'f', 6)));
            lastPos = m.capturedEnd();
        }
        result.append(input.mid(lastPos));

        // Return wrapped (The tree structure ? : is valid in Legacy too!)
        return QString("clamp(-1, %1, 1)").arg(result);
    }

    // --- MODE B: Standard Chain Fallback ---
    // If the input isn't PCM (doesn't have var s), we treat it as a SID chain
    if (input.startsWith("0.000+")) input = input.mid(6);

    QStringList additiveParts;
    double lastStartTime = 0.0;
    QString currentLayer = input;

    while (true) {
        int questIdx = currentLayer.indexOf('?');
        if (questIdx == -1) break;

        QString postQuest = currentLayer.mid(questIdx + 1);
        int localColon = findScopeAwareChar(postQuest, ':');
        if (localColon == -1) break;
        int colonIdx = questIdx + 1 + localColon;

        int timeEnd = questIdx - 1;
        int timeStart = currentLayer.lastIndexOf("t<", timeEnd);
        QString timeStr = (timeStart != -1) ? currentLayer.mid(timeStart + 2, timeEnd - timeStart - 1) : "0";

        QString content = currentLayer.mid(questIdx + 1, colonIdx - questIdx - 1);

        QString remainder = currentLayer.mid(colonIdx + 1);
        while (remainder.startsWith("(") && remainder.endsWith(")") &&
               findScopeAwareChar(remainder.mid(1, remainder.length()-2), ':') != -1) {
            remainder = remainder.mid(1, remainder.length() - 2);
        }

        additiveParts << QString("((t >= %1 & t < %2) * %3)")
                             .arg(QString::number(lastStartTime, 'f', 6))
                             .arg(timeStr).arg(content);

        lastStartTime = timeStr.toDouble();
        currentLayer = remainder;
    }

    if (additiveParts.isEmpty()) return "Error: Logic mismatch.";
    return QString("clamp(-1, %1, 1)").arg(additiveParts.join(" + "));
}

// ---------------------------------------------------------
// PRESET LOADER: THE MASTERS COLLECTION
// ---------------------------------------------------------
void MainWindow::loadWavetablePreset(int index) {
    // Helper lambda to make adding steps cleaner
    // Args: Waveform, Semitones, PWM, Duration
    auto add = [&](QString w, int p, int pwm, double d) {
        int r = wtTrackerTable->rowCount();
        wtTrackerTable->insertRow(r);
        wtTrackerTable->setItem(r, 0, new QTableWidgetItem(w));
        wtTrackerTable->setItem(r, 1, new QTableWidgetItem(QString::number(p)));
        wtTrackerTable->setItem(r, 2, new QTableWidgetItem(QString::number(pwm)));
        wtTrackerTable->setItem(r, 3, new QTableWidgetItem(QString::number(d)));
    };

    wtTrackerTable->setRowCount(0);
    QString txt = wtPresetCombo->currentText();

    // --- ROB HUBBARD ---
    if (txt.contains("Commando")) { // Glissando Bass
        add("Tri", 0, 0, 0.03); add("Tri", -1, 0, 0.03); add("Tri", -2, 0, 0.03);
        add("Tri", -3, 0, 0.03); add("Tri", -4, 0, 0.03); add("Tri", -5, 0, 0.03);
    }
    else if (txt.contains("Monty")) { // Fast PWM Lead
        add("Pulse", 0, 10, 0.04); add("Pulse", 0, 50, 0.04); add("Pulse", 0, 90, 0.04);
        add("Pulse", 0, 50, 0.04); // PWM Sweep loop
    }
    else if (txt.contains("Delta")) { // The classic SID Snare
        add("Noise", 12, 0, 0.02); // Snap
        add("TriNoise", 0, 0, 0.04); // Body (Tri mixed with Noise)
        add("Tri", -5, 0, 0.05);   // Tonal tail
    }
    else if (txt.contains("Zoids")) { // Ring Mod Simulation
        add("Metal", 0, 0, 0.03); add("Metal", 1, 0, 0.03);
        add("Metal", -1, 0, 0.03); add("Metal", 0, 0, 0.03);
    }
    else if (txt.contains("Ace 2")) { // Kick
        add("Square", 0, 50, 0.01); add("Tri", -12, 0, 0.02); add("Tri", -24, 0, 0.08);
    }
    else if (txt.contains("Comets")) { // Echo
        add("Saw", 0, 0, 0.06); add("Saw", 0, 0, 0.06); // Main
        add("Saw", 0, 0, 0.06); // Space
        add("Saw", 0, 0, 0.04); // Echo (simulated by decay in engine usually, but here distinct)
    }

    // --- MARTIN GALWAY ---
    else if (txt.contains("Wizball")) { // Bubble Arp
        add("Tri", 0, 0, 0.03); add("Tri", 4, 0, 0.03); add("Tri", 7, 0, 0.03);
    }
    else if (txt.contains("Parallax")) { // Slap Bass
        add("Saw", 12, 0, 0.02); // Slap
        add("Pulse", 0, 20, 0.05); // Body
        add("Pulse", 0, 40, 0.10); // Open
    }
    else if (txt.contains("Comic")) { // Lead
        add("Pulse", 0, 50, 0.03); add("Pulse", 0, 50, 0.03); // Wait
        add("Pulse", 1, 50, 0.02); add("Pulse", 0, 50, 0.02); // Vibrato
    }
    else if (txt.contains("Arkanoid")) { // Dotted Echo Effect
        add("Saw", 0, 0, 0.06); // Note
        add("Saw", 0, 0, 0.06); // Space
        add("Saw", 0, 0, 0.04); // Echo 1
        add("Saw", 0, 0, 0.04); // Space
        add("Saw", 0, 0, 0.02); // Echo 2
    }
    else if (txt.contains("Green Beret")) { // Military Snare
        add("Noise", 10, 0, 0.02); // Crack
        add("Noise", 5, 0, 0.03);  // Body
        add("Noise", 0, 0, 0.05);  // Decay
    }
    // --- JEROEN TEL ---
    else if (txt.contains("Cybernoid")) { // Metallic Drum
        add("Metal", 24, 0, 0.02); add("Metal", 12, 0, 0.02); add("Noise", 0, 0, 0.05);
    }
    else if (txt.contains("Supremacy")) { // Vibrato Lead
        add("Saw", 0, 0, 0.05); add("Saw", 0, 0, 0.05);
        add("Saw", 1, 0, 0.02); add("Saw", -1, 0, 0.02); // Late Vibrato
    }
    else if (txt.contains("Turbo Outrun")) { // Slap Bass
        add("Metal", 0, 0, 0.02);   // Metallic Attack
        add("Pulse", -12, 40, 0.04); // Deep Body
        add("Pulse", -12, 60, 0.08); // PWM Open
    }
    else if (txt.contains("RoboCop 3")) { // Title Screen Arp
        // Famous intro: Root -> +7 -> +12 -> +19
        add("Saw", 0, 0, 0.02); add("Saw", 7, 0, 0.02);
        add("Saw", 12, 0, 0.02); add("Saw", 19, 0, 0.02);
    }
    // --- CHRIS HUELSBECK ---
    else if (txt.contains("Turrican I")) { // Huge Arp
        add("Pulse", 0, 50, 0.02); add("Pulse", 12, 50, 0.02);
        add("Pulse", 0, 25, 0.02); add("Pulse", 19, 25, 0.02);
    }
    else if (txt.contains("Katakis")) { // Space Lead
        add("SawSync", 0, 0, 0.04); add("SawSync", 0, 0, 0.04);
    }
    else if (txt.contains("Turrican II")) { // Pad / Sweep
        add("Pulse", 0, 10, 0.05); add("Pulse", 0, 20, 0.05);
        add("Pulse", 0, 30, 0.05); add("Pulse", 0, 40, 0.05);
        add("Pulse", 0, 50, 0.20); // Sustain
    }
    else if (txt.contains("Great Giana")) { // Classic C64 Bass
        add("Tri", 0, 0, 0.03);
        add("Square", 0, 50, 0.10); // Solid square body
    }
    // --- TIM FOLLIN ---
    else if (txt.contains("Solstice")) { // Prog Lead
        add("Pulse", 0, 15, 0.02); add("Pulse", 0, 20, 0.02);
        add("Pulse", 0, 25, 0.02); add("Pulse", 0, 30, 0.02); // PWM Swell
    }
    else if (txt.contains("Ghouls")) { // Rain
        add("Noise", 24, 0, 0.01); add("Noise", 12, 0, 0.02);
    }
    else if (txt.contains("Silver Surfer")) { // Insane Arp
        // Follin often used irregular intervals
        add("Pulse", 0, 25, 0.01); add("Pulse", 4, 25, 0.01);
        add("Pulse", 7, 25, 0.01); add("Pulse", 11, 25, 0.01);
        add("Pulse", 14, 25, 0.01); add("Pulse", 12, 50, 0.01);
    }
    else if (txt.contains("LED Storm")) { // Rolling Bass
        add("Saw", 12, 0, 0.02); // Pick
        add("Saw", 0, 0, 0.03);  // Low
        add("Saw", 0, 0, 0.03);  // Low
        add("Saw", 12, 0, 0.02); // Octave Jump
    }

    // --- BEN DAGLISH ---
    else if (txt.contains("Last Ninja")) { // Dark Bass
        add("Saw", 0, 0, 0.04);
        add("Tri", 0, 0, 0.04); // Mixing waveforms for texture
        add("Tri", -12, 0, 0.10);
    }
    else if (txt.contains("Deflektor")) { // Glide Lead
        add("Pulse", 0, 50, 0.02); add("Pulse", 1, 50, 0.02);
        add("Pulse", 2, 50, 0.02); add("Pulse", 3, 50, 0.02); // Slide up
        add("Pulse", 4, 10, 0.10); // Thin Sustain
    }
    else if (txt.contains("Trap")) { // Fast Arp
        add("Square", 0, 50, 0.02); add("Square", 4, 50, 0.02);
        add("Square", 7, 50, 0.02); add("Square", 0, 50, 0.02);
    }

    // --- DAVID WHITTAKER ---
    else if (txt.contains("Glider Rider")) { // Pure Square
        add("Square", 0, 50, 0.05); add("Square", 0, 50, 0.05); // Just a pure, unmodulated tone
    }
    else if (txt.contains("Lazy Jones")) { // Laser
        add("Saw", 24, 0, 0.01); add("Saw", 20, 0, 0.01);
        add("Saw", 16, 0, 0.01); add("Saw", 12, 0, 0.01);
        add("Saw", 8, 0, 0.01);  add("Saw", 4, 0, 0.01);
    }

    // --- YM / ATARI ---
    else if (txt.contains("YM Buzzer")) { // YM Envelope
        add("Saw", 0, 0, 0.02); add("Saw", 0, 0, 0.02); // Step
        add("Pulse", 0, 50, 0.01); // Buzz
    }
    else if (txt.contains("YM Metal")) {
        add("Metal", 0, 0, 0.05); add("Metal", -12, 0, 0.05);
    }
    else if (txt.contains("YM 3-Voice")) { // Fake Chord
        // Rapidly cycling 3 notes to simulate a chord on one channel
        add("Saw", 0, 0, 0.01); add("Saw", 4, 0, 0.01);
        add("Saw", 7, 0, 0.01);
    }
    else if (txt.contains("Digi-Drum")) { // 4-bit Sample Emulation
        // Rapid volume changes to simulate a sample curve
        add("Pulse", -24, 50, 0.01); add("Pulse", -24, 50, 0.01);
        add("Pulse", -24, 90, 0.01); // Thinner (quiet)
        add("Pulse", -24, 10, 0.01); // Thinner (quiet)
    }
    // --- FX ---
    else if (txt.contains("Coin")) {
        add("Pulse", 0, 50, 0.03); add("Pulse", 5, 50, 0.03);
        add("Pulse", 10, 50, 0.03); add("Pulse", 15, 50, 0.10);
    }
    else if (txt.contains("Explosion")) {
        add("Noise", 0, 0, 0.10); add("Noise", -5, 0, 0.10); add("Noise", -10, 0, 0.20);
    }
    else if (txt.contains("Fake Chord (Major)")) {
        add("Saw", 0, 0, 0.01); add("Saw", 4, 0, 0.01); add("Saw", 7, 0, 0.01);
    }
    else if (txt.contains("Power Up")) {
        add("Tri", 0, 0, 0.02); add("Tri", 2, 0, 0.02);
        add("Tri", 4, 0, 0.02); add("Tri", 5, 0, 0.02);
        add("Tri", 7, 0, 0.02); add("Tri", 12, 0, 0.10);
    }
    else if (txt.contains("Laser")) { // Pew Pew
        add("Saw", 30, 0, 0.01); add("Saw", 20, 0, 0.01);
        add("Saw", 10, 0, 0.01); add("Saw", 0, 0, 0.01);
        add("Saw", -10, 0, 0.01);
    }
    else if (txt.contains("Hi-Hat (Closed)")) {
        add("Metal", 40, 0, 0.01); // High pitch metal
        add("Noise", 40, 0, 0.01);
    }
    else if (txt.contains("Hi-Hat (Open)")) {
        add("Metal", 40, 0, 0.02);
        add("Noise", 40, 0, 0.04); // Longer decay
    }
    else if (txt.contains("Fake Chord (Minor)")) {
        add("Saw", 0, 0, 0.01); add("Saw", 3, 0, 0.01); add("Saw", 7, 0, 0.01);
    }
    // --- NEW SID DRUMS EXPANSION ---
    else if (txt.contains("Heavy SID Kick")) {
        // Classic "Bomp": High square click -> Rapid Triangle drop
        add("Square", 36, 50, 0.01); // Click (High)
        add("Tri", 12, 0, 0.01);     // Body Start
        add("Tri", 0, 0, 0.02);      // Body Drop
        add("Tri", -12, 0, 0.04);    // Low
        add("Tri", -24, 0, 0.08);    // Sub Bass Tail
    }
    else if (txt.contains("Snappy Snare")) {
        // The "Galway" Snare: Noise for the crack, Triangle for the body
        add("Noise", 24, 0, 0.01);    // Sharp Crack
        add("TriNoise", 12, 0, 0.02); // Body (Tone + Noise mixed)
        add("TriNoise", 0, 0, 0.03);  // Decay
        add("Noise", -12, 0, 0.05);   // Tail (Just noise)
    }
    else if (txt.contains("Tech Kick")) {
        // Jeroen Tel Style: Metallic attack -> Solid Pulse body
        add("Metal", 12, 0, 0.01);    // Ring Mod Attack (Crunchy)
        add("Pulse", 0, 50, 0.02);    // Body
        add("Pulse", -12, 50, 0.05);  // Low
        add("Pulse", -24, 50, 0.10);  // Sub
    }
    else if (txt.contains("Glitch Snare")) {
        // Sci-Fi / Industrial Snare using Ring Mod
        add("Metal", 24, 0, 0.02);    // High Ring Mod
        add("Metal", 12, 0, 0.03);    // Mid Ring Mod
        add("Noise", 0, 0, 0.06);     // Noise Tail
    }
}

// ---------------------------------------------------------
// GENERATOR: WITH NEW "SPECIAL" WAVEFORMS
// ---------------------------------------------------------
void MainWindow::generateWavetableForge() {
    int rows = wtTrackerTable->rowCount();
    if (rows == 0) return;

    double totalDuration = 0;
    for(int i=0; i<rows; ++i)
        if(wtTrackerTable->item(i,3)) totalDuration += wtTrackerTable->item(i, 3)->text().toDouble();

    QString timeVar = "t";
    if (wtLoopCheck->isChecked()) timeVar = QString("mod(t, %1)").arg(totalDuration, 0, 'f', 4);

    // MODE SELECTION
    if (buildModeWavetable->currentIndex() == 0) { // NIGHTLY (Nested)
        QString nestedBody = "0";
        double currentTime = totalDuration;

        for (int i = rows - 1; i >= 0; --i) {
            QString type = wtTrackerTable->item(i, 0)->text().toLower();
            int pitch = wtTrackerTable->item(i, 1)->text().toInt();
            double pwmVal = wtTrackerTable->item(i, 2)->text().toDouble() / 100.0;
            double dur = wtTrackerTable->item(i, 3)->text().toDouble();
            currentTime -= dur;

            double pitchMult = std::pow(2.0, pitch / 12.0);

            // --- NEW WAVEFORM LOGIC ---
            QString audio;

            if (type.contains("trinoise")) {
                // Classic C64 Snare Drum trick (Tri + Noise)
                audio = QString("(trianglew(integrate(f*%1)) + 0.5*randv(t*10000))").arg(pitchMult);
            }
            else if (type.contains("metal")) {
                // Ring Mod Simulation (Square * Detuned Square)
                audio = QString("(squarew(integrate(f*%1)) * squarew(integrate(f*%2)))")
                            .arg(pitchMult).arg(pitchMult * 2.41);
            }
            else if (type.contains("sawsync")) {
                // Hard Sync Simulation (Saw * Saw window)
                audio = QString("(saww(integrate(f*%1)) * saww(integrate(f*%2)))")
                            .arg(pitchMult).arg(pitchMult * 0.5);
            }
            else if (type.contains("pulse")) {
                audio = QString("(sinew(integrate(f*%1)) > %2 ? 1 : -1)")
                .arg(pitchMult, 0, 'f', 4).arg((pwmVal * 2.0) - 1.0, 0, 'f', 4);
            }
            else if (type.contains("noise")) {
                audio = "randv(t * 10000)";
            }
            else {
                QString osc = type.contains("tri") ? "trianglew" : "saww";
                audio = QString("%1(integrate(f*%2))").arg(osc).arg(pitchMult, 0, 'f', 4);
            }

            nestedBody = QString("(%1 < %2 ? %3 : %4)")
                             .arg(timeVar).arg(currentTime + dur, 0, 'f', 4)
                             .arg(audio).arg(nestedBody);
        }
        statusBox->setText(QString("clamp(-1, %1, 1)").arg(nestedBody));
    }
    else { // LEGACY (Additive)
        QStringList additiveParts;
        double currentTime = 0.0;
        for (int i = 0; i < rows; ++i) {
            QString type = wtTrackerTable->item(i, 0)->text().toLower();
            int pitch = wtTrackerTable->item(i, 1)->text().toInt();
            double pwmVal = wtTrackerTable->item(i, 2)->text().toDouble() / 100.0;
            double dur = wtTrackerTable->item(i, 3)->text().toDouble();

            double pitchMult = std::pow(2.0, pitch / 12.0);
            QString audio;

            // Same special waveforms for Legacy
            if (type.contains("trinoise")) audio = QString("(trianglew(integrate(f*%1)) + 0.5*randv(t*10000))").arg(pitchMult);
            else if (type.contains("metal")) audio = QString("(squarew(integrate(f*%1)) * squarew(integrate(f*%2)))").arg(pitchMult).arg(pitchMult * 2.41);
            else if (type.contains("sawsync")) audio = QString("(saww(integrate(f*%1)) * saww(integrate(f*%2)))").arg(pitchMult).arg(pitchMult * 0.5);
            else if (type.contains("pulse")) audio = QString("(sinew(integrate(f*%1)) > %2 ? 1 : -1)").arg(pitchMult).arg((pwmVal * 2.0) - 1.0);
            else if (type.contains("noise")) audio = "randv(t * 10000)";
            else {
                QString osc = type.contains("tri") ? "trianglew" : "saww";
                audio = QString("%1(integrate(f*%2))").arg(osc).arg(pitchMult);
            }

            QString block = QString("((%1 >= %2 & %1 < %3) * %4)")
                                .arg(timeVar).arg(currentTime, 0, 'f', 4)
                                .arg(currentTime + dur, 0, 'f', 4).arg(audio);
            additiveParts << block;
            currentTime += dur;
        }
        statusBox->setText(QString("clamp(-1, %1, 1)").arg(additiveParts.join(" + ")));
    }
}

// ---------------------------------------------------------
// XPF PACKAGER: INJECTS CODE INTO INSTRUMENT TEMPLATE
// ---------------------------------------------------------
void MainWindow::saveXpfInstrument() {
    // ... (The logic I gave you in the previous step goes here) ...
    // If you need the full logic block again, let me know!

    QString code = xpfInput->toPlainText();
    if (code.isEmpty()) {
        statusBox->setText("Error: No code to package! Paste something in the XPF tab first.");
        return;
    }

    // Escape XML characters (Basic safety)
    code = code.replace("&", "&amp;")
               .replace("\"", "&quot;")
               .replace("'", "&apos;")
               .replace("<", "&lt;")
               .replace(">", "&gt;")
               .replace("\n", ""); // Remove newlines for the attribute

    // THE MASTER TEMPLATE
    // Based on 'foryoumyfriend.xpf' but cleaned up:
    // - O1="1" (Enabled)
    // - W1="0" (Disabled)
    // - O2="0" (Disabled)
    // - pan="0" (Centered)
    // - src1="..." (Your Code)

    QString xmlContent =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE lmms-project>\n"
        "<lmms-project creator=\"WaveConv\" version=\"20\">\n"
        "  <head/>\n"
        "  <instrumenttracksettings name=\"WaveConv_Patch\" type=\"0\" muted=\"0\" solo=\"0\">\n"
        "    <instrumenttrack usemasterpitch=\"1\" vol=\"100\" pitch=\"0\" pan=\"0\" basenote=\"57\">\n" // Centered Pan
        "      <instrument name=\"xpressive\">\n"
        "        <xpressive \n"
        "           version=\"0.1\" \n"
        "           gain=\"1\" \n"
        "           O1=\"1\" \n"           // O1 Enabled
        "           O2=\"0\" \n"           // O2 Disabled
        "           W1=\"0\" \n"           // W1 Disabled (Removed as requested)
        "           W2=\"0\" \n"
        "           src1=\"" + code + "\" \n"  // <--- YOUR CODE GOES HERE
                 "           src2=\"\" \n"
                 "           p1=\"0\" p2=\"0\" \n"  // Panning for Oscs (Centered)
                 "           crse1=\"0\" fine1=\"0\" \n"
                 "           crse2=\"0\" fine2=\"0\" \n"
                 "           ph1=\"0\" ph2=\"0\" \n"
                 "           bin=\"\" \n"           // Cleared binary data
                 "        >\n"
                 "          <key/>\n"
                 "        </xpressive>\n"
                 "      </instrument>\n"
                 "      <eldata fcut=\"14000\" fres=\"0.5\" ftype=\"0\" fwet=\"0\">\n"
                 "        <elvol amt=\"1\" att=\"0\" dec=\"0.5\" hold=\"0.5\" rel=\"0.1\" sustain=\"0.5\"/>\n"
                 "        <elcut amt=\"0\"/>\n"
                 "        <elres amt=\"0\"/>\n"
                 "      </eldata>\n"
                 "    </instrumenttrack>\n"
                 "  </instrumenttracksettings>\n"
                 "</lmms-project>\n";

    // Save File Dialog
    QString fileName = QFileDialog::getSaveFileName(this, "Save Instrument", "", "LMMS Instrument (*.xpf)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << xmlContent;
        file.close();
        statusBox->setText("Saved successfully to: " + fileName);
    } else {
        statusBox->setText("Error: Could not save file.");
    }
}
// ---------------------------------------------------------
// GENERATOR: KEY MAPPER (KEYBOARD SPLITS)
// ---------------------------------------------------------
void MainWindow::generateKeyMapper() {
    int rows = keyMapTable->rowCount();
    if (rows == 0) {
        statusBox->setText("Error: Key Map Table is empty.");
        return;
    }

    QString finalFormula;

    // --- MODE A: NIGHTLY (Nested Ternary) ---
    if (keyMapMode->currentIndex() == 0) {
        QString nestedBody = "0";
        int startIdx = rows - 1;

        // Base case: If last row goes to 127, it captures everything else
        if (keyMapTable->item(startIdx, 0)->text().toInt() >= 127) {
            nestedBody = keyMapTable->item(startIdx, 1)->text();
            startIdx--;
        }

        for (int i = startIdx; i >= 0; --i) {
            QString limit = keyMapTable->item(i, 0)->text();
            QString code = keyMapTable->item(i, 1)->text();
            nestedBody = QString("(key < %1 ? %2 : %3)").arg(limit).arg(code).arg(nestedBody);
        }
        finalFormula = nestedBody;
    }

    // --- MODE B: LEGACY (Additive) ---
    else {
        QStringList segments;
        int lowerBound = 0;

        for (int i = 0; i < rows; ++i) {
            int upperBound = keyMapTable->item(i, 0)->text().toInt();
            QString code = keyMapTable->item(i, 1)->text();

            QString rangeCheck;

            // Logic to match your working example exactly
            if (i == 0 && lowerBound == 0) {
                // First Zone: Just check Upper Limit (e.g. key < 60)
                rangeCheck = QString("(key < %1)").arg(upperBound);
            }
            else if (i == rows - 1 && upperBound >= 127) {
                // Last Zone: Just check Lower Limit (e.g. key >= 60)
                rangeCheck = QString("(key >= %1)").arg(lowerBound);
            }
            else {
                // Middle Zone: Check Both (e.g. key >= 40 * key < 60)
                rangeCheck = QString("((key >= %1) * (key < %2))").arg(lowerBound).arg(upperBound);
            }

            segments << QString("((%1) * (%2))").arg(rangeCheck).arg(code);
            lowerBound = upperBound;
        }
        finalFormula = segments.join(" + ");
    }

    QString result = QString("clamp(-1, %1, 1)").arg(finalFormula);
    statusBox->setText(result);
    QApplication::clipboard()->setText(result);
}

// ---------------------------------------------------------
// GENERATOR: STEP GATE
// ---------------------------------------------------------
void MainWindow::generateStepGate() {
    // 1. Calculate Rate
    // Base: 16th notes at Tempo. Formula: (tempo/60) * 4
    QString speedExpr = "(tempo/60.0)*4.0";
    double mult = 1.0;
    int sIdx = gateSpeedCombo->currentIndex();
    if(sIdx == 0) mult = 0.5;
    if(sIdx == 2) mult = 2.0;
    if(sIdx == 3) mult = 4.0;
    if(gateTripletCheck->isChecked()) mult *= 1.5; // Triplet feel (faster)

    speedExpr += QString("*%1").arg(mult);

    // 2. Get Carrier Waveform
    QString wave = "squarew(integrate(f))"; // Default
    int shapeIdx = gateShapeCombo->currentIndex();
    if(shapeIdx == 1) wave = "saww(integrate(f))";
    else if(shapeIdx == 2) wave = "sinew(integrate(f))";
    else if(shapeIdx == 3) wave = "randv(t*10000)";
    else if(shapeIdx == 4) wave = QString("(%1)").arg(gateCustomShape->toPlainText());

    // 3. Build Gate Pattern
    // We map step (0-15) to On(1) or Off(0)
    QString gateLogic;
    bool nightly = (gateBuildMode->currentIndex() == 0);

    if (nightly) {
        // Nightly: Use variables for cleaner code
        // "var step := mod(floor(t * speed), 16);"
        QString stepMap = "0";
        // Build nested ternary backwards: (step == 15 ? val : (step == 14 ? val ...))
        for(int i=15; i>=0; --i) {
            QString val = gateSteps[i]->isChecked() ? "1" : "0";
            stepMap = QString("(step == %1 ? %2 : %3)").arg(i).arg(val).arg(stepMap);
        }
        gateLogic = QString("var step := mod(floor(t * %1), 16);\nvar g := %2;\n(g * %3)").arg(speedExpr).arg(stepMap).arg(wave);
    } else {
        // Legacy: Inline everything (Additive)
        // "((mod(floor(t*rate),16)==0)*1) + ..."
        QStringList parts;
        for(int i=0; i<16; ++i) {
            if(gateSteps[i]->isChecked()) {
                // We use >= i & < i+1 logic to catch the step
                parts << QString("((mod(floor(t*%1),16) >= %2 & mod(floor(t*%1),16) < %3))")
                             .arg(speedExpr).arg(i).arg(i+1);
            }
        }
        if(parts.isEmpty()) gateLogic = "0";
        else gateLogic = QString("(%1) * %2").arg(parts.join(" + ")).arg(wave);
    }

    // 4. Mix Amount (Dry / Wet)
    double mix = gateMixSlider->value() / 100.0;
    if(mix < 1.0) {
        // Formula: (Original_Wave * (1-mix)) + (Gated_Wave * mix)
        gateLogic = QString("((%1 * %2) + (%3 * %4))").arg(wave).arg(1.0-mix).arg(gateLogic).arg(mix);
    }

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(gateLogic));
    QApplication::clipboard()->setText(statusBox->toPlainText());
}

QString MainWindow::getXpfTemplate() {
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
          << "<elvol rel=\"0.1\" dec=\"0.5\" sustain=\"0\" amt=\"0\"/>" // AMT=0 for manual ADSR
          << "</eldata></instrumenttrack></instrumenttracksettings></lmms-project>";
    return lines.join("\n");
}

void MainWindow::generateDrumXpf() {
    QString waveFunc = drumWaveCombo->currentText().toLower() + "w";
    if(waveFunc == "sinew") waveFunc = "sinew";

    double decayBase = drumDecaySlider->value();
    double expFactor = drumExpSlider->value(); //

    // Pitch Drop Formula
    QString pitch = QString("(f + (%1 * exp(-t * %2)))").arg(drumPitchDropSlider->value()).arg(decayBase / 2.0);

    // Osc + Noise Mix
    double nMix = drumNoiseSlider->value() / 100.0;
    QString source = QString("((%1(integrate(%2)) * %3) + (randv(t*10000) * %4))")
                         .arg(waveFunc).arg(pitch).arg(1.0 - nMix).arg(nMix);

    // Final Exponential Volume Shape
    QString formula = QString("(%1 * exp(-t * %2))").arg(source).arg(decayBase * expFactor);
    formula = formula.replace("\"", "&quot;");

    int typeIndex = drumTypeCombo->currentIndex();
    int filterType = 0; // Default LPF (0)
    if (typeIndex == 1 || typeIndex == 4 || typeIndex == 6) filterType = 2; // BPF
    if (typeIndex == 2 || typeIndex == 5) filterType = 1; // HPF

    QString xpf = getXpfTemplate()
                      .arg(drumTypeCombo->currentText()).arg(drumPitchSlider->value())
                      .arg(formula).arg(drumToneSlider->value())
                      .arg(drumSnapSlider->value() / 100.0).arg(filterType)
                      .arg(0.1).arg(0.5);

    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    if (clickedButton == btnSaveDrumXpf) {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Drum", "", "LMMS Preset (*.xpf)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << xpf;
                file.close();
                statusBox->setText("Drum saved: " + fileName);
            }
        }
    } else {
        QApplication::clipboard()->setText(xpf);
        statusBox->setText("Drum XPF copied to clipboard!");
    }
}


void MainWindow::generateNumbers1981() {
    int steps = (numStepsCombo->currentIndex() == 0) ? 16 : 32;
    double dur = numDuration->value();
    bool isRandom = (numModeCombo->currentIndex() == 0);

    // Formula: (tempo / 15) is roughly 4 steps per beat if tempo is 60.
    QString speedMath = "(tempo / 15.0)";

    // 1. Determine the Pitch Source
    QString pitchSource;

    if (isRandom) {
        // Random Logic: randv(-1 to 1) * 12 gives +/- 12 semitones
        pitchSource = QString("randv(floor(mod(t * %1, %2))) * 12")
                          .arg(speedMath).arg(steps);
    } else {
        // Pattern Editor Logic
        QString nested = "0"; // Default

        // Iterate BACKWARDS to build the nest
        for(int i = steps - 1; i >= 0; --i) {
            QString val = numPatternTable->item(0, i)->text();
            if(val.isEmpty()) val = "0";
            nested = QString("(s == %1 ? %2 : %3)").arg(i).arg(val).arg(nested);
        }

        pitchSource = QString("var s := floor(mod(t * %1, %2));\n%3")
                          .arg(speedMath).arg(steps).arg(nested);
    }

    // 2. Gate Logic
    QString gate = QString("(mod(t * %1, 1) < %2)").arg(speedMath).arg(dur);

    // 3. O1 (Left)
    QString o1 = QString("squarew(integrate(f * semitone(%1))) * %2")
                     .arg(pitchSource).arg(gate);

    // 4. O2 (Right) - Detuned + Vibrato
    QString pitchSourceO2 = pitchSource;

    if (isRandom) {
        pitchSourceO2 = QString("randv(floor(mod(t * %1, %2))) * 12 + 0.5 * sinew(t * 12)")
        .arg(speedMath).arg(steps);
    } else {
        pitchSourceO2 += " + 0.5 * sinew(t * 12)";
    }

    QString o2 = QString("squarew(integrate(f * 1.02 * semitone(%1))) * %2")
                     .arg(pitchSourceO2).arg(gate);

    // 5. Output
    numOut1->setText(o1);
    numOut2->setText(o2);
}

void MainWindow::generateDelayArchitect() {
    // 1. Determine Source Audio
    QString source;
    int idx = delayWaveCombo->currentIndex();
    if (idx == 0) source = "(trianglew(integrate(f)) * exp(-t * 20))"; // Pluck
    else if (idx == 1) source = "(saww(integrate(f)) * exp(-t * 5))";
    else if (idx == 2) source = "(squarew(integrate(f)) * exp(-t * 10))";
    else source = QString("(%1)").arg(delayCustomInput->toPlainText());

    // 2. Calculate Delay Variables
    double time = delayTimeSpin->value();       // e.g. 0.2s
    double rate = delayRateSpin->value();       // e.g. 8000Hz
    double fb = delayFeedbackSpin->value();     // e.g. 0.6
    int taps = delayTapsSpin->value();          // e.g. 4

    int samples = (int)(time * rate);           // Convert Time to Sample Count (e.g. 1600)

    // 3. Build Multitap Chain
    // Formula: Source + Tap1 + Tap2...
    // Tap N: last(samples * N) * (feedback ^ N) * (t > time * N)

    QStringList chain;
    chain << source; // The dry signal

    for(int i = 1; i <= taps; ++i) {
        int offset = samples * i;
        double gain = std::pow(fb, i);
        double startTime = time * i;

        // We check (t > startTime) to prevent garbage data from looking back before t=0
        QString tap = QString("(%1 * last(%2) * (t > %3))")
                          .arg(gain, 0, 'f', 3)
                          .arg(offset)
                          .arg(startTime, 0, 'f', 3);
        chain << tap;
    }

    // 4. Output
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(chain.join(" + ")));
    QApplication::clipboard()->setText(statusBox->toPlainText());
}

void MainWindow::generateMacroMorph() {
    int style = macroStyleCombo->currentIndex();
    bool isLegacy = (macroBuildMode->currentIndex() == 1);

    // Fetch Slider Values (0.0 to 1.0)
    double mColor = macroColorSlider->value() / 100.0;
    double mTime  = macroTimeSlider->value() / 100.0;
    double mGrit  = macroBitcrushSlider->value() / 100.0; // Bitcrush
    double mTex   = macroTextureSlider->value() / 100.0;
    double mWidth = macroWidthSlider->value() / 100.0;
    double mWonk  = macroWonkySlider->value() / 100.0;

    QString osc, env;

    // --- ALGORITHM LIBRARY ---
    switch(style) {
    case 0: // FUTURE SUPER SAWS
        osc = QString("((saww(integrate(f)) + saww(integrate(f * %1)) + saww(integrate(f * %2))) / 3)")
                  .arg(1.0 + (mWidth * 0.02))
                  .arg(1.0 - (mWidth * 0.02));
        // Filter Logic
        osc = QString("(%1 * %2 + sinew(integrate(f)) * %3)")
                  .arg(osc).arg(mColor).arg(1.0 - mColor);
        env = QString("min(1, t * 20) * exp(-t * %1)").arg(5.0 - (mTime * 4.0));
        break;

    case 1: // FORMANT VOCAL LEAD
    {
        // Base: Triangle wave for body
        QString base = "trianglew(integrate(f/2))";

        // Formant Modulation (PWM-like effect)
        // Nightly can use a variable for the LFO to keep it readable
        // Legacy must inline the LFO math directly into the phase

        double vibSpeed = 6.0;
        double vibDepth = mTime * 0.05;

        if (isLegacy) {
            // Inline: f * (1 + sin(t*6)*depth)
            QString lfo = QString("(1.0 + sinew(t*%1)*%2)").arg(vibSpeed).arg(vibDepth);

            // Apply LFO to frequency inside the integrator
            osc = QString("(%1 * (0.5 + 0.4 * sinew(integrate(f * %2 * %3))))")
                      .arg(base)
                      .arg(lfo) // Frequency mod
                      .arg(2.0 + (mColor * 3.0)); // Formant shift
        } else {
            // Nightly: Cleaner variable
            osc = QString("(%1 * (0.5 + 0.4 * sinew(integrate(f * %2))))")
                      .arg(2.0 + (mColor * 3.0));

            // Prepend Variable
            if(mTime > 0) {
                osc = QString("var vib:=sinew(t*%1)*%2; %3")
                .arg(vibSpeed).arg(vibDepth)
                    .arg(osc.replace("(f", "(f*(1+vib)"));
            }
        }
        env = "1";
        break;
    }

    case 2: // WOBBLY CASSETTE KEYS
    {
        double drift = 1.0 + (mWidth * 0.005);
        osc = QString("(trianglew(integrate(f * %1)) + %2 * sinew(integrate(f * 4)))")
                  .arg(drift).arg(mColor * 0.5);
        env = QString("exp(-t * %1)").arg(10.0 - (mTime * 8.0));
        break;
    }

    case 3: // GRANULAR PAD
        osc = QString("(saww(integrate(f)) * (0.8 + 0.2 * randv(t * %1)))")
                  .arg(50 + mTex * 500);
        env = QString("min(1, t * %1)").arg(0.5 + mTime * 2.0);
        break;

    case 4: // HOLLOW BASS
        osc = QString("(squarew(integrate(f)) * (1 - %1 * exp(-t*20)))")
                  .arg(mColor);
        env = "1";
        break;

    case 5: // PORTAMENTO LEAD
        osc = QString("saww(integrate(f)) + %1 * saww(integrate(f * 1.01))").arg(mWidth);
        env = "1";
        break;

    case 6: // PLUCKY ARP
        osc = "squarew(integrate(f)) * (sinew(integrate(f*2)) > 0 ? 1 : 0)";
        env = QString("exp(-t * %1)").arg(20.0 - mTime * 10.0);
        break;

    case 7: // VINYL ATMOSPHERE
        osc = "0";
        env = "1";
        break;
    }

    // --- GLOBAL PROCESSING ---

    // 1. Apply Envelope
    if (style != 7) {
        osc = QString("(%1 * %2)").arg(osc).arg(env);
    }

    // 2. TEXTURE LAYER (Noise)
    if (mTex > 0 || style == 7) {
        QString noise = QString("(randv(t * 8000) * %1)").arg(mTex * 0.15);
        if (style == 7) osc = noise;
        else osc = QString("(%1 + %2)").arg(osc).arg(noise);
    }

    // 3. WONK (Sidechain)
    if (mWonk > 0) {
        QString sidechain = QString("(1.0 - %1 * abs(sinew(t * 15)))").arg(mWonk * 0.8);
        osc = QString("(%1 * %2)").arg(osc).arg(sidechain);
    }

    // 4. BITCRUSH (Formerly VHS Degrade)
    if (mGrit > 0) {
        double steps = 16.0 - (mGrit * 14.0);
        // Both Legacy and Nightly handle 'floor' and 'steps' logic fine inline
        osc = QString("floor(%1 * %2) / %2").arg(osc).arg(steps);
    }

    // Final Safety Clamp
    statusBox->setText(QString("clamp(-1, %1, 1)").arg(osc));
    QApplication::clipboard()->setText(statusBox->toPlainText());
}
void MainWindow::generateStringMachine() {
    int model = stringModelCombo->currentIndex();
    int chord = stringChordCombo->currentIndex();

    // Normalized Values
    double valEns    = stringEnsembleSlider->value() / 100.0;
    double valAtt    = stringAttackSlider->value() / 100.0;
    double valEvolve = stringEvolveSlider->value() / 100.0;
    double valMotion = stringMotionSlider->value() / 100.0;
    double valAge    = stringAgeSlider->value() / 100.0;
    double valRel    = stringSpaceSlider->value() / 100.0;

    // --- 1. DEFINE THE OSCILLATOR "CELL" ---
    auto getOsc = [&](double detuneMult, double mix, double phaseOffset) {
        QString shape;

        // $tinkworx / Aquatic Logic:
        // Uses PWM (Pulse Width Modulation) that drifts over time (t)
        if (model == 3) {
            // Pulse wave where width modulates from 50% (Square) to Thin
            double pwmSpeed = 2.0 + (valMotion * 5.0);
            shape = QString("(sinew(integrate(f*%1)) > (0.8 * sinew(t*%2 + %3)) ? 1 : -1)")
                        .arg(detuneMult)
                        .arg(pwmSpeed)
                        .arg(phaseOffset); // Phase offset ensures lines don't overlap on scope
        }
        // Amazing String / Solina Logic:
        else {
            // EVOLUTION LOGIC:
            // Mixes Triangle (Dark) -> Saw (Bright) over time based on 'Evolve' slider
            // This replicates your "t" logic but makes it dynamic.

            // Base Tone: Sawtooth
            QString saw = QString("saww(integrate(f*%1))").arg(detuneMult);
            // Dark Tone: Triangle
            QString tri = QString("trianglew(integrate(f*%1))").arg(detuneMult);

            // Evolve factor: 0 = Static Saw, 1 = Tri fading into Saw
            if (valEvolve > 0) {
                // Logic: (Dark * (1-env)) + (Bright * env)
                double speed = 1.0 + (valEvolve * 4.0);
                QString filterEnv = QString("(1 - exp(-t*%1))").arg(speed);
                shape = QString("((%1 * (1-%3)) + (%2 * %3))").arg(tri).arg(saw).arg(filterEnv);
            } else {
                shape = saw; // Pure Amazing String style
            }
        }

        // VISUAL FIX: PHASE MOTION
        // We inject a tiny frequency modulation based on time to make the scope "dance"
        // instead of locking into a static jagged line.
        if (valMotion > 0) {
            shape = shape.replace("(f*", QString("(f * (1 + %1 * sinew(t*3 + %2)) *")
                                      .arg(valMotion * 0.002) // Subtle drift
                                      .arg(phaseOffset));     // Unique per osc
        }

        // VINTAGE AGE (Wobble)
        if (valAge > 0) {
            shape = shape.replace("(f", QString("(f * (1 + %1 * sinew(t*6))").arg(valAge * 0.005));
        }

        return QString("(%1 * %2)").arg(shape).arg(mix);
    };

    // --- 2. BUILD THE ENSEMBLE ---
    double spread = 1.0 + (valEns * 0.015);

    // We pass distinct Phase Offsets (0, 2, 4) to ensure the visual lines separate
    QString oscC = getOsc(1.0, 0.5, 0.0);
    QString oscL = getOsc(spread, 0.25, 2.0);
    QString oscR = getOsc(2.0 - spread, 0.25, 4.0);

    QString cell = QString("(%1 + %2 + %3)").arg(oscC, oscL, oscR);

    // --- 3. CHORD MEMORY LOGIC ---
    QString stack;
    if (chord == 0) stack = cell; // Manual
    else if (chord == 1) stack = QString("(%1 + 0.5*%2)").arg(cell).arg(cell.replace("(f", "(f*2"));
    else if (chord == 2) stack = QString("(%1 + 0.5*%2)").arg(cell).arg(cell.replace("(f", "(f*1.498"));
    else if (chord == 3) { // Amazing String (m9)
        QString c1=cell; QString c2=cell; c2.replace("(f","(f*1.189");
        QString c3=cell; c3.replace("(f","(f*1.498");
        QString c4=cell; c4.replace("(f","(f*1.781");
        QString c5=cell; c5.replace("(f","(f*2.245");
        stack = QString("(0.25*%1+0.2*%2+0.2*%3+0.15*%4+0.15*%5)").arg(c1,c2,c3,c4,c5);
    }
    else if (chord == 4) { // $tinkworx Minor 11 (Deep Dub Chord)
        // Root, m3, 5, m7, 11 (No 9th)
        QString c1=cell;
        QString c2=cell; c2.replace("(f","(f*1.189"); // m3
        QString c3=cell; c3.replace("(f","(f*1.498"); // 5
        QString c4=cell; c4.replace("(f","(f*1.781"); // m7
        QString c5=cell; c5.replace("(f","(f*2.669"); // 11 (Perfect 4th up an octave)
        stack = QString("(0.3*%1+0.2*%2+0.2*%3+0.15*%4+0.15*%5)").arg(c1,c2,c3,c4,c5);
    }
    else if (chord == 5) { // Sus4
        QString c1=cell; QString c2=cell; c2.replace("(f","(f*1.334");
        QString c3=cell; c3.replace("(f","(f*1.498");
        stack = QString("(0.4*%1+0.3*%2+0.3*%3)").arg(c1,c2,c3);
    }

    // --- 4. EVOLUTION ENVELOPE ---
    // User requested "evolving strings" logic like: (t < 0.6) * (t/0.6)
    // We make the '0.6' adjustable via Attack Slider.

    double attTime = 0.01 + (valAtt * 2.0); // 0 to 2 seconds
    double relTime = 0.1 + (valRel * 3.0);  // Release tail

    // Attack Logic
    QString envLogic = QString("min(1, t / %1)").arg(attTime);

    // Release Logic (Simulated for raw code export)
    // Note: True release requires note-off events which pure math formulas can't always see
    // without a 'gate' variable, but we can simulate a "decay" curve if they hold the note.
    // For now, we stick to the Attack Swell as requested for the "Amazing" evolution.

    QString finalResult = QString("(%1 * %2)").arg(stack).arg(envLogic);

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(finalResult));
    QApplication::clipboard()->setText(statusBox->toPlainText());
}

void MainWindow::loadHardwarePreset(int idx) {
    if (idx <= 0) return;
    struct HardwarePatch { QString wave; int a, d, s, r, f, q, ps, pd, vs, vd, n; bool peak; };
    QMap<int, HardwarePatch> lib;

    // --- 1. THE STUDIO CLASSICS (1-8) ---
    lib[1] = {"squarew", 0, 30, 0, 10, 1100, 20, 12, 15, 0, 0, 65, false};   // Hissing Minimal
    lib[2] = {"saww", 5, 55, 40, 35, 3800, 30, 0, 0, 8, 10, 5, false};      // Analog Drift
    lib[3] = {"squarew", 0, 25, 0, 10, 2800, 95, 35, 70, 5, 2, 0, true};     // Resonance Burner
    lib[4] = {"trianglew", 0, 15, 0, 5, 4500, 60, 0, 0, 0, 0, 20, true};     // Metallic Tick
    lib[5] = {"trianglew", 2, 45, 15, 20, 450, 45, 45, 40, 0, 0, 0, false};  // PWM Low-End
    lib[6] = {"saww", 0, 70, 60, 30, 14000, 15, 0, 0, 10, 15, 10, true};     // Power Saw
    lib[7] = {"sinew", 10, 80, 50, 60, 1200, 10, 5, 10, 15, 5, 2, false};    // Phase Mod Keys
    lib[8] = {"saww", 60, 90, 80, 90, 900, 5, 10, 20, 12, 12, 15, false};    // Deep Pad

    // --- 2. MODULAR MINIMAL (9-16) ---
    for(int i=9; i<=16; ++i)
        lib[i] = {"squarew", 0, 10+i, 10, 10, 2000+(i*100), 10+(i*2), 5, 5, 0, 0, 5, false};

    // --- 3. INDUSTRIAL WAREHOUSE (17-24) ---
    for(int i=17; i<=24; ++i)
        lib[i] = {"squarew", 0, 20+(i-16)*5, 0, 15, 1000+(i*50), 85, 40, 60, 0, 0, 25, true};

    // --- 4. SIGNAL DRIFT (25-32) ---
    for(int i=25; i<=32; ++i)
        lib[i] = {"saww", 40+(i-24)*5, 80, 70, 85, 1500, 10, 5, 10, 12, 25, 30, false};

    // --- 5. FREQUENCY GLITCH (33-40) ---
    for(int i=33; i<=40; ++i)
        lib[i] = {"trianglew", 0, 5+(i-32)*2, 0, 2, 9000-(i*100), 50, 90, 95, 0, 0, 70, true};

    if (lib.contains(idx)) {
        HardwarePatch p = lib[idx];
        hwBaseWave->setCurrentText(p.wave);
        hwAttack->setValue(p.a); hwDecay->setValue(p.d);
        hwSustain->setValue(p.s); hwRelease->setValue(p.r);
        hwCutoff->setValue(p.f); hwResonance->setValue(p.q);
        hwPwmSpeed->setValue(p.ps); hwPwmDepth->setValue(p.pd);
        hwVibSpeed->setValue(p.vs); hwVibDepth->setValue(p.vd);
        hwNoiseMix->setValue(p.n);
        hwPeakBoost->setChecked(p.peak);

        // Update the visualizer
        adsrVisualizer->updateEnvelope(p.a/100.0, p.d/100.0, p.s/100.0, p.r/100.0);
    }
}

void MainWindow::generateHardwareXpf() {
    QString wave = hwBaseWave->currentText();
    QString pitchMod = QString("(1 + sinew(t * %1) * %2)").arg(hwVibSpeed->value()/10.0).arg(hwVibDepth->value()/500.0);
    QString osc = (wave == "squarew") ?
                      QString("(sinew(integrate(f * %1)) > (sinew(t * %2) * %3) ? 1 : -1)").arg(pitchMod).arg(hwPwmSpeed->value()/10.0).arg(hwPwmDepth->value()/100.0) :
                      QString("%1(integrate(f * %2))").arg(wave).arg(pitchMod);

    double nMix = hwNoiseMix->value() / 100.0;
    QString finalSource = QString("((%1 * %2) + (randv(t*10000) * %3))").arg(osc).arg(1.0 - nMix).arg(nMix);
    if (hwPeakBoost->isChecked()) finalSource = QString("clamp(-1, %1 * 1.8, 1)").arg(finalSource);

    QString xml =
        "<?xml version=\"1.0\"?>\n<!DOCTYPE lmms-project>\n"
        "<lmms-project version=\"20\" creator=\"WaveConv\" type=\"instrumenttracksettings\">\n"
        "  <head/>\n"
        "  <instrumenttracksettings name=\"Hardware_Patch\" muted=\"0\" solo=\"0\">\n"
        "    <instrumenttrack vol=\"100\" pan=\"0\" basenote=\"" + QString::number(hwBaseNote->value()) + "\" pitchrange=\"1\">\n"
                                                 "      <instrument name=\"xpressive\">\n"
                                                 "        <xpressive version=\"0.1\" O1=\"" + finalSource.replace("\"", "&quot;") + "\" O2=\"0\" bin=\"\">\n"
                                                "          <key/>\n"
                                                "        </xpressive>\n"
                                                "      </instrument>\n"
                                                "      <eldata fcut=\"" + QString::number(hwCutoff->value()) + "\" fres=\"" + QString::number(hwResonance->value()/100.0) + "\" ftype=\"0\" fwet=\"1\">\n"
                                                                                                              "        <elvol att=\"" + QString::number(hwAttack->value()/100.0) + "\" dec=\"" + QString::number(hwDecay->value()/100.0) + "\" sustain=\"" + QString::number(hwSustain->value()/100.0) + "\" rel=\"" + QString::number(hwRelease->value()/100.0) + "\" amt=\"1\"/>\n"
                                                                                                                                                                                                                                             "      </eldata>\n"
                                                                                                                                                                                                                                             "    </instrumenttrack>\n"
                                                                                                                                                                                                                                             "  </instrumenttracksettings>\n"
                                                                                                                                                                                                                                             "</lmms-project>\n";

    QString fileName = QFileDialog::getSaveFileName(this, "Save Hardware Patch", "", "LMMS Patch (*.xpf)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) { QTextStream(&file) << xml; file.close(); }
    }
}

void MainWindow::generateRandomHardware() {
    // Randomize Waveform
    hwBaseWave->setCurrentIndex(std::rand() % 4);

    // Randomize ADSR for aggressive or soft tones
    hwAttack->setValue(std::rand() % 40);
    hwDecay->setValue(20 + (std::rand() % 60));
    hwSustain->setValue(std::rand() % 80);
    hwRelease->setValue(10 + (std::rand() % 50));

    // Randomize Filter
    hwCutoff->setValue(500 + (std::rand() % 8000));
    hwResonance->setValue(std::rand() % 90);

    // Randomize Movement
    hwPwmSpeed->setValue(std::rand() % 100);
    hwPwmDepth->setValue(std::rand() % 70);
    hwVibSpeed->setValue(std::rand() % 50);
    hwVibDepth->setValue(std::rand() % 30);

    // Randomize Noise Mix
    hwNoiseMix->setValue(std::rand() % 40);

    statusBox->setText("Hardware Parameters Randomized!");
}

