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
    QWidget *drumTab = new QWidget();

    // 1. CHANGE: Main layout is now VERTICAL
    auto *drumMainLayout = new QVBoxLayout(drumTab);

    // 2. NEW: The Red Disclaimer Label
    drumDisclaimer = new QLabel("⚠ DISCLAIMER: PLACEHOLDER FEATURE.\n"
                                "The intent is to suggest and automate the instrument filter external to the expression\n"
                                "via exporting an instrument .xpf in Legacy or Nightly build format.\n"
                                "This will be an additional part of the project.");
    drumDisclaimer->setStyleSheet("QLabel { color: red; font-weight: bold; font-size: 14px; border: 2px solid red; padding: 10px; background-color: #ffeeee; }");
    drumDisclaimer->setAlignment(Qt::AlignCenter);
    drumDisclaimer->setFixedHeight(80); // Consistent height

    drumMainLayout->addWidget(drumDisclaimer); // Add to top

    // 3. The Drum Controls (Sub-Layout)
    auto *drumForm = new QFormLayout();

    // Re-create the controls (Standard setup)
    drumType = new QComboBox();
    drumType->addItems({"Kick (808 Style)", "Snare (Noise+Tone)", "Hi-Hat (Metal)", "Tom (Low)", "Clap (Digital)"});

    drumBuildMode = new QComboBox();
    drumBuildMode->addItems({"Nightly (Nested)", "Legacy (Additive)"});

    // Sliders
    drumFreqSlider = new QSlider(Qt::Horizontal); drumFreqSlider->setRange(0, 100); drumFreqSlider->setValue(50);
    drumPunchSlider = new QSlider(Qt::Horizontal); drumPunchSlider->setRange(0, 100); drumPunchSlider->setValue(80);
    drumDecaySlider = new QSlider(Qt::Horizontal); drumDecaySlider->setRange(0, 100); drumDecaySlider->setValue(40);
    drumToneSlider = new QSlider(Qt::Horizontal); drumToneSlider->setRange(0, 100); drumToneSlider->setValue(20);

    // Filter Warning (The small yellow one we had before)
    drumFilterWarning = new QLabel("Note: Filters must be applied manually in this version.");
    drumFilterWarning->setStyleSheet("color: orange; font-style: italic;");

    // Add rows to the form
    drumForm->addRow("Drum Type:", drumType);
    drumForm->addRow("Build Mode:", drumBuildMode);
    drumForm->addRow("Base Freq:", drumFreqSlider);
    drumForm->addRow("Punch/Snap:", drumPunchSlider);
    drumForm->addRow("Decay Length:", drumDecaySlider);
    drumForm->addRow("Tone/Click:", drumToneSlider);
    drumForm->addRow("", drumFilterWarning);

    auto *btnDrum = new QPushButton("Generate Drum");
    drumForm->addRow(btnDrum);

    // Add the form to the main layout
    drumMainLayout->addLayout(drumForm);
    drumMainLayout->addStretch(); // Push to top

    modeTabs->addTab(drumTab, "Drum Architect");

    // Connect the button
    connect(btnDrum, &QPushButton::clicked, this, &MainWindow::generateDrumArchitect);

    // 10. VELOCILOGIC (DYNAMICS MAPPER)
    QWidget *velTab = new QWidget();
    auto *velLayout = new QVBoxLayout(velTab);

    // 1. Disclaimer
    velDisclaimer = new QLabel("⚠ VELOCILOGIC: DYNAMIC LAYERING.\n"
                               "Placeholder.\n"
                               "Code not working at present.");
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
                                  "Is not working yet.");
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
    addZone(128, "pulse(t*f)");   // Lead (Everything else)

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

    // ---------------------------------------------------------
    // 20. LEGACY PIANO EDITOR (EXPERIMENTAL - SELF CONTAINED)
    // ---------------------------------------------------------
   // QWidget *pianoTab = new QWidget();
    // Use a ScrollArea because we have a LOT of sliders now
  //  QScrollArea *pianoScroll = new QScrollArea();
  //  QWidget *pianoContent = new QWidget();
 //   QVBoxLayout *pianoLayout = new QVBoxLayout(pianoContent);

    // 1. HEADER
//    QLabel *pianoWarn = new QLabel("🎹 LEGACY PIANO LAB (SELF-CONTAINED) 🎹\n"
//                                   "This tool generates a piano from internal C++ code.\n"
//                                   "Experimental: Not working for nightly build and external filter control not mapped yet");
//    pianoWarn->setStyleSheet("border: 2px solid #444; background: #eee; color: #333; font-weight: bold; padding: 10px;");
//    pianoWarn->setAlignment(Qt::AlignCenter);
//    pianoLayout->addWidget(pianoWarn);

    // 2. SLIDER HELPERS
//    auto mkSlider = [](int max, int def) {
//        QSlider *s = new QSlider(Qt::Horizontal);
//        s->setRange(0, max); s->setValue(def);
//        return s;
//    };
//    auto addRow = [&](QFormLayout* f, QString name, QSlider* s) {
//        f->addRow(name, s);
//    };

    // --- GROUP 1: OSCILLATOR MATH (THE EXPRESSION) ---
//    QGroupBox *grpMath = new QGroupBox("Oscillator Expression (O1)");
//    grpMath->setStyleSheet("QGroupBox { font-weight: bold; color: blue; border: 1px solid blue; margin-top: 10px; }::title { subcontrol-origin: margin; left: 10px; padding: 0 3px;}");
//    QFormLayout *formMath = new QFormLayout(grpMath);

//    pianoMixSawSlider  = mkSlider(100, 66); // Default 0.66
//    pianoMixSubSlider  = mkSlider(100, 66);
//    pianoMixHighSlider = mkSlider(100, 66);

//    addRow(formMath, "Center Saw Vol:", pianoMixSawSlider);
//    addRow(formMath, "Sub-Octave Vol:", pianoMixSubSlider);
//    addRow(formMath, "High-Octave Vol:", pianoMixHighSlider);
//    pianoLayout->addWidget(grpMath);

    // --- GROUP 2: FILTER & ENVELOPE ---
//    QGroupBox *grpEnv = new QGroupBox("Filter & Envelope");
//    QFormLayout *formEnv = new QFormLayout(grpEnv);

//    pianoVolSlider    = mkSlider(200, 63);    // Master Vol
//    pianoCutoffSlider = mkSlider(10000, 6608); // Filter Hz
//    pianoResSlider    = mkSlider(100, 10);     // Res
//    pianoAttSlider    = mkSlider(100, 0);      // Att
//    pianoDecSlider    = mkSlider(100, 30);     // Dec
//    pianoSusSlider    = mkSlider(100, 0);      // Sus
//    pianoRelSlider    = mkSlider(100, 17);     // Rel

//    addRow(formEnv, "Master Vol:", pianoVolSlider);
//    addRow(formEnv, "Filter Cutoff:", pianoCutoffSlider);
//    addRow(formEnv, "Resonance:", pianoResSlider);
//    addRow(formEnv, "Attack:", pianoAttSlider);
//    addRow(formEnv, "Decay:", pianoDecSlider);
//    addRow(formEnv, "Sustain:", pianoSusSlider);
//    addRow(formEnv, "Release:", pianoRelSlider);
//    pianoLayout->addWidget(grpEnv);

    // --- GROUP 3: EFFECTS (PLATE REVERB) ---
//    QGroupBox *grpFx = new QGroupBox("LADSPA Effects (Plate Reverb)");
//    grpFx->setStyleSheet("QGroupBox { font-weight: bold; color: darkgreen; border: 1px solid darkgreen; margin-top: 10px; }::title { subcontrol-origin: margin; left: 10px; padding: 0 3px;}");
//    QFormLayout *formFx = new QFormLayout(grpFx);

//    pianoRevBandwidth = mkSlider(100, 54); // Port 2
//    pianoRevTail      = mkSlider(100, 45); // Port 3
//    pianoRevDamping   = mkSlider(100, 24); // Port 4
//    pianoRevMix       = mkSlider(100, 28); // Port 5 (Dry/Wet)

 //   addRow(formFx, "Rev Bandwidth:", pianoRevBandwidth);
 //   addRow(formFx, "Rev Tail (Size):", pianoRevTail);
//    addRow(formFx, "Rev Damping:", pianoRevDamping);
//    addRow(formFx, "Dry/Wet Mix:", pianoRevMix);
//    pianoLayout->addWidget(grpFx);

    // 4. SOURCE VIEWER (Read Only now)
//    pianoSourceEdit = new QTextEdit();
//    pianoSourceEdit->setReadOnly(true);
//    pianoSourceEdit->setMaximumHeight(80);
//    pianoSourceEdit->setPlaceholderText("Generated XML will appear here...");
//    pianoLayout->addWidget(new QLabel("Internal Template Preview:"));
//    pianoLayout->addWidget(pianoSourceEdit);

    // 5. SAVE BUTTON
//    QPushButton *btnSavePiano = new QPushButton("GENERATE & SAVE PIANO (.XPF)");
//    btnSavePiano->setStyleSheet("font-weight: bold; background-color: #444; color: white; height: 50px; font-size: 14px;");
//    pianoLayout->addWidget(btnSavePiano);

//    pianoStatusLabel = new QLabel("Ready.");
//    pianoLayout->addWidget(pianoStatusLabel);

    // Finalize Layout
//    pianoScroll->setWidget(pianoContent);
//    pianoScroll->setWidgetResizable(true);

    // Add to Tab
//    pianoTab->setLayout(new QVBoxLayout);
//    pianoTab->layout()->addWidget(pianoScroll);
//    modeTabs->addTab(pianoTab, "Legacy Piano (Exp)");

//    connect(btnSavePiano, &QPushButton::clicked, this, &MainWindow::saveLegacyPiano);

    // 20. NEED TO KNOW / NOTES TAB
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
        "<li><b>NO ADSR Shaping (except for external (non expression) test in Piano Lab:</b> The generated code does not automate the Instrument Envelope (Attack, Decay, Sustain, Release). You must program these expressions yourself or set these knobs manually in the Instrument Editor.</li>"
        "<li><b>Please forgive my bad grammar and poor structure, I am working late each night on this and tidy up and notes is usually the last thing I do before I go to sleep.</li>"
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

// ---------------------------------------------------------
// GENERATOR: VELOCILOGIC (DYNAMICS)
// ---------------------------------------------------------
void MainWindow::generateVelocilogic() {
    int rows = velMapTable->rowCount();
    if (rows == 0) return;

    QString finalFormula;

    // --- MODE A: NIGHTLY (Nested Ternary) ---
    // Structure: (vel < 0.5 ? Soft : (vel < 0.8 ? Med : Hard))
    if (velMapMode->currentIndex() == 0) {

        QString nestedBody = "0";
        int startIdx = rows - 1;

        // Base case optimization
        if (velMapTable->item(startIdx, 0)->text().toInt() >= 127) {
            nestedBody = velMapTable->item(startIdx, 1)->text();
            startIdx--;
        }

        for (int i = startIdx; i >= 0; --i) {
            // Convert 0-127 to 0.0-1.0
            double rawLimit = velMapTable->item(i, 0)->text().toDouble();
            double normLimit = rawLimit / 127.0;

            QString code = velMapTable->item(i, 1)->text();

            nestedBody = QString("(vel < %1 ? %2 : %3)")
                             .arg(normLimit, 0, 'f', 3)
                             .arg(code)
                             .arg(nestedBody);
        }
        finalFormula = nestedBody;
    }

    // --- MODE B: LEGACY (Additive) ---
    // Structure: ((vel < 0.5) * Soft) + ...
    else {
        QStringList segments;
        double lowerBound = 0.0;

        for (int i = 0; i < rows; ++i) {
            double rawLimit = velMapTable->item(i, 0)->text().toDouble();
            double upperBound = rawLimit / 127.0;
            QString code = velMapTable->item(i, 1)->text();

            QString rangeCheck;
            if (i == rows - 1 && rawLimit >= 127) {
                rangeCheck = QString("(vel >= %1)").arg(lowerBound, 0, 'f', 3);
            } else {
                rangeCheck = QString("(vel >= %1 & vel < %2)")
                .arg(lowerBound, 0, 'f', 3)
                    .arg(upperBound, 0, 'f', 3);
            }

            segments << QString("(%1 * %2)").arg(rangeCheck).arg(code);
            lowerBound = upperBound;
        }
        finalFormula = segments.join(" + ");
    }

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(finalFormula));
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
    if (rows == 0) return;

    QString finalFormula;

    // --- MODE A: NIGHTLY (Nested Ternary) ---
    // Structure: (key < Limit1 ? Code1 : (key < Limit2 ? Code2 : Code3))
    if (keyMapMode->currentIndex() == 0) {

        // Start with the last zone (The "Else" case)
        QString nestedBody = "0";

        // We iterate backwards.
        // The last row is usually the "Rest of the keyboard" (Limit 127/128)
        // But for nesting, we build from the bottom up.

        // Optimization: If the last row goes to 128, it's the base case.
        int startIdx = rows - 1;
        if (keyMapTable->item(startIdx, 0)->text().toInt() >= 127) {
            nestedBody = keyMapTable->item(startIdx, 1)->text();
            startIdx--; // We consumed this row as the base
        }

        for (int i = startIdx; i >= 0; --i) {
            QString limit = keyMapTable->item(i, 0)->text();
            QString code = keyMapTable->item(i, 1)->text();

            // Format: (key < LIMIT ? CODE : NEXT_BLOCK)
            nestedBody = QString("(key < %1 ? %2 : %3)")
                             .arg(limit)
                             .arg(code)
                             .arg(nestedBody);
        }
        finalFormula = nestedBody;
    }

    // --- MODE B: LEGACY (Additive) ---
    // Structure: ((key < 60) * Bass) + ((key >= 60 & key < 84) * Pad) ...
    else {
        QStringList segments;
        int lowerBound = 0; // Start at MIDI 0

        for (int i = 0; i < rows; ++i) {
            int upperBound = keyMapTable->item(i, 0)->text().toInt();
            QString code = keyMapTable->item(i, 1)->text();

            // Range Logic
            QString rangeCheck;
            if (i == rows - 1 && upperBound >= 127) {
                // Last segment goes to end: (key >= lower)
                rangeCheck = QString("(key >= %1)").arg(lowerBound);
            } else {
                // Middle segment: (key >= lower & key < upper)
                rangeCheck = QString("(key >= %1 & key < %2)")
                                 .arg(lowerBound)
                                 .arg(upperBound);
            }

            // Combine: (Range * Code)
            segments << QString("(%1 * %2)").arg(rangeCheck).arg(code);

            // Set up next loop
            lowerBound = upperBound;
        }
        finalFormula = segments.join(" + ");
    }

    statusBox->setText(QString("clamp(-1, %1, 1)").arg(finalFormula));
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
//void MainWindow::saveLegacyPiano() {
//    QString tmpl = R"(<?xml version="1.0"?>
//<!DOCTYPE lmms-project>
//<lmms-project creator="LMMS" version="20" type="instrumenttracksettings">
//  <head/>
//  <instrumenttracksettings type="0" name="House Piano_v5" muted="0" solo="0" mutedBeforeSolo="0">
//    <instrumenttrack pan="0" pitch="0" vol="__VOL__" pitchrange="1" basenote="69" usemasterpitch="1" fxch="0">
//     <instrument name="xpressive">
//        <xpressive version="0.1"
//           O1="__MATH_EXPR__"
//           W1sample=")";
// Append Base64 Chunk 1
//    tmpl += R"(AACAvwDgf78AwH+/AKB/vwCAf78AYH+/AEB/vwAgf78AAH+/AOB+vwDAfr8AoH6/AIB+vwBgfr8AQH6/ACB+vwAAfr8A4H2/AMB9vwCgfb8AgH2/AGB9vwBAfb8AIH2/AAB9vwDgfL8AwHy/AKB8vwCAfL8AYHy/AEB8vwAgfL8AAHy/AOB7vwDAe78AoHu/AIB7vwBge78AQHu/ACB7vwAAe78A4Hq/AMB6vwCger8AgHq/AGB6vwBAer8AIHq/AAB6vwDgeb8AwHm/AKB5vwCAeb8AYHm/AEB5vwAgeb8AAHm/AOB4vwDAeL8AoHi/AIB4vwBgeL8AQHi/ACB4vwAAeL8A4He/AMB3vwCgd78AgHe/AGB3vwBAd78AIHe/AAB3vwDgdr8AwHa/AKB2vwCAdr8AYHa/AEB2vwAgdr8AAHa/AOB1vwDAdb8AoHW/AIB1vwBgdb8AQHW/ACB1vwAAdb8A4HS/AMB0vwCgdL8AgHS/AGB0vwBAdL8AIHS/AAB0vwDgc78AwHO/AKBzvwCAc78AYHO/AEBzvwAgc78AAHO/AOByvwDAcr8AoHK/AIByvwBgcr8AQHK/ACByvwAAcr8A4HG/AMBxvwCgcb8AgHG/AGBxvwBAcb8AIHG/AABxvwDgcL8AwHC/AKBwvwCAcL8AYHC/AEBwvwAgcL8AAHC/AOBvvwDAb78AoG+/AIBvvwBgb78AQG+/ACBvvwAAb78A4G6/AMBuvwCgbr8AgG6/AGBuvwBAbr8AIG6/AABuvwDgbb8AwG2/AKBtvwCAbb8AYG2/AEBtvwAgbb8AAG2/AOBsvwDAbL8AoGy/AIBsvwBgbL8AQGy/ACBsvwAAbL8A4Gu/AMBrvwCga78AgGu/AGBrvwBAa78AIGu/AABrvwDgar8AwGq/AKBqvwCAar8AYGq/AEBqvwAgar8AAGq/AOBpvwDAab8AoGm/AIBpvwBgab8AQGm/ACBpvwAAab8A4Gi/AMBovwCgaL8AgGi/AGBovwBAaL8AIGi/AABovwDgZ78AwGe/AKBnvwCAZ78AYGe/AEBnvwAgZ78AAGe/AOBmvwDAZr8AoGa/AIBmvwBgZr8AQGa/ACBmvwAAZr8A4GW/AMBlvwCgZb8AgGW/AGBlvwBAZb8AIGW/AABlvwDgZL8AwGS/AKBkvwCAZL8AYGS/AEBkvwAgZL8AAGS/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AKArvwCAK78AYCu/AEArvwAgK78AACu/AOAqvwDAKr8AoCq/AIAqvwBgKr8AQCq/ACAqvwAAKr8A4Cm/AMApvwCgKb8AgCm/AGApvwBAKb8AICm/AAApvwDgKL8AwCi/AKAovwCAKL8AYCi/AEAovwAgKL8AACi/AOAnvwDAJ78AoCe/AIAnvwBgJ78AQCe/ACAnvwAAJ78A4Ca/AMAmvwCgJr8AgCa/AGAmvwBAJr8AICa/AAAmvwDgJb8AwCW/AKAlvwCAJb8AYCW/AEAlvwAgJb8AACW/AOAkvwDAJL8AoCS/AIAkvwBgJL8AQCS/ACAkvwAAJL8A4CO/AMAjvwCgI78AgCO/AGAjvwBAI78AICO/AAAjvwDgIr8AwCK/AKAivwCAIr8AYCK/AEAivwAgIr8AACK/AOAhvwDAIb8AoCG/AIAhvwBgIb8AQCG/ACAhvwAAIb8A4CC/AMAgvwCgIL8AgCC/AGAgvwBAIL8AICC/AAAgvwDgH78AwB+/AKAfvwCAH78AYB+/AEAfvwAgH78AAB+/AOAevwDAHr8AoB6/AIAevwBgHr8AQB6/ACAevwAAHr8A4B2/AMAdvwCgHb8AgB2/AGAdvwBAHb8AIB2/AAAdvwDgHL8AwBy/AKAcvwCAHL8AYBy/AEAcvwAgHL8AABy/AOAbvwDAG78AoBu/AIAbvwBgG78AQBu/ACAbvwAAG78A4Bq/AMAavwCgGr8AgBq/AGAavwBAGr8AIBq/AAAavwDgGb8AwBm/AKAZvwCAGb8AYBm/AEAZvwAgGb8AABm/AOAYvwDAGL8AoBi/AIAYvwBgGL8AQBi/ACAYvwAAGL8A4Be/AMAXvwCgF78AgBe/AGAXvwBAF78AIBe/AAAXvwDgFr8AwBa/AKAWvwCAFr8AYBa/AEAWvwAgFr8AABa/AOAVvwDAFb8AoBW/AIAVvwBgFb8AQBW/ACAVvwAAFb8A4BS/AMAUvwCgFL8AgBS/AGAUvwBAFL8AIBS/AAAUvwDgE78AwBO/AKATvwCAE78AYBO/AEATvwAgE78AABO/AOASvwDAEr8AoBK/AIASvwBgEr8AQBK/ACASvwAAEr8A4BG/AMARvwCgEb8AgBG/AGARvwBAEb8AIBG/AAARvwDgEL8AwBC/AKAQvwCAEL8AYBC/AEAQvwAgEL8AABC/AOAPvwDAD78AoA+/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AAD4vQAA970AAPa9AAD1vQAA9L0AAPO9AADyvQAA8b0AAPC9AADvvQAA7r0AAO29AADsvQAA670AAOq9AADpvQAA6L0AAOe9AADmvQAA5b0AAOS9AADjvQAA4r0AAOG9AADgvQAA370AAN69AADdvQAA3L0AANu9AADavQAA2b0AANi9AADXvQAA1r0AANW9AADUvQAA070AANK9AADRvQAA0L0AAM+9AADOvQAAzb0AAMy9AADLvQAAyr0AAMm9AADIvQAAx70AAMa9AADFvQAAxL0AAxL0AAMO9AADCvQAAwb0AAMC9AAC/vQAAvr0AAL29AAC8vQAAu70AALq9AAC5vQAAuL0AALe9AAC2vQAAtb0AALS9AACzvQAAsr0AALG9AACwvQAAr70AAK69AACtvQAArL0AAKu9AACqvQAAqb0AAKi9AACnvQAApr0AAKW9AACkvQAAo70AAKK9AAChvQAAoL0AAJ+9AACevQAAnb0AAJy9AACbvQAAmr0AAJm9AACYvQAAl70AAJa9AACVvQAAlL0AAJO9AACSvQAAkb0AAJC9AACPvQAAjr0AAI29AACMvQAAi70AAIq9AACJvQAAiL0AAIe9AACGvQAAhb0AAIS9AACDvQAAgr0AAIG9AACAvQAAfr0AAHy9AAB6vQAAeL0AAHa9AAB0vQAAcr0AAHC9AABuvQAAbL0AAGq9AABovQAAZr0AAGS9AABivQAAYL0AAF69AABcvQAAWr0AAFi9AABWvQAAVL0AAFK9AABQvQAATr0AAEy9AABKvQAASL0AAEa9AABEvQAAQr0AAEC9AAA+vQAAPL0AADq9AAA4vQAANr0AADS9AAAyvQAAML0AAC69AAAsvQAAKr0AACi9AAAmvQAAJL0AACK9AAAgvQAAHr0AABy9AAAavQAAGL0AABa9AAAUvQAAEr0AABC9AAAOvQAADL0AAAq9AAAIvQAABr0AAAS9AAACvQAAAL0AAPy8AAD4vAAA9LwAAPC8AADsvAAA6LwAAOS8AADgvAAA3LwAANi8AADUvAAA0LwAAMy8AADIvAAAxLwAAMC8AAC8vAAAuLwAALS8AACwvAAArLwAAKi8AACkvAAAoLwAAJy8AACYvAAAlLwAAJC8AACMvAAAiLwAAIS8AACAvAAAeLwAAHC8AABovAAAYLwAAFi8AABQvAAASLwAAEC8AAA4vAAAMLwAACi8AAAgvAAAGLwAABC8AAAIvAAAALwAAPC7AADguwAA0LsAAMC7AACwuwAAoLsAAJC7AACAuwAAYLsAAEC7AAAguwAAALsAAMC6AACAugAAALoAAAAAAAAAOgAAgDoAAMA6AAAAOwAAIDsAAEA7AABgOwAAgDsAAJA7AACgOwAAsDsAAMA7AADQOwAA4DsAAPA7AAAAPAAACDwAABA8AAAYPAAAIDwAACg8AAAwPAAAODwAAEA8AABIPAAAUDwAAFg8AABgPAAAaDwAAHA8AAB4PAAAgDwAAIQ8AACIPAAAjDwAAJA8AACUPAAAmDwAAJw8AACgPAAApDwAAKg8AACsPAAAsDwAALQ8AAC4PAAAvDwAAMA8AADEPAAAyDwAAMw8AADQPAAA1DwAANg8AADcPAAA4DwAAOQ8AAD0PAAA7DwAAPA8AAD0PAAA7DwAAPw8AAAAPQAAAj0AAAQ9AAAGPQAACD0AAAo9AAAMPQAADj0AABA9AAASPQAAFD0AABY9AAAYPQAAGj0AABw9AAAePQAAID0AACI9AAAkPQAAJj0AACg9AAAqPQAALD0AAC49AAAwPQAAMj0AADQ9AAA2PQAAOD0AADo9AAA8PQAAPj0AAEA9AABCPQAARD0AAEY9AABIPQAASj0AAEw9AABOPQAAUD0AAFI9AABUPQAAVj0AAFg9AABaPQAAXD0AAF49AABgPQAAYj0AAGQ9AABmPQAAaD0AAGo9AABsPQAAbj0AAHA9AAByPQAAdD0AAHY9AAB4PQAAej0AAHw9AAB+PQAAgD0AAIE9AACCPQAAgz0AAIQ9AACFPQAAhj0AAIc9AACIPQAAiT0AAIo9AACLPQAAjD0AAI09AACOPQAAjz0AAJA9AACRPQAAkj0AAJM9AACUPQAAlT0AAJY9AACXPQAAmD0AAJk9AACaPQAAmz0AAJw9AACdPQAAnj0AAJ89AACgPQAAoT0AAKI9AACjPQAApD0AAKU9AACmPQAApz0AAKg9AACpPQAAqj0AAKs9AACsPQAArT0AAK49AACvPQAAsD0AALE9AACyPQAAsz0AALQ9AAC1PQAAtj0AALc9AAC4PQAAuT0AALo9AAC7PQAAvD0AAL09AAC+PQAAvz0AAMA9AADBPQAAwj0AAMM9AADEPQAAxT0AAMY9AADHPQAAyD0AAMk9AADKPQAAyz0AAMw9AADNPQAAzj0AAM89AADQPQAA0T0AANI9AADTPQAA1D0AANU9AADWPQAA1z0AANg9AADZPQAA2j0AANs9AADcPQAA3T0AAN49AADfPQAA4D0AAOE9AADiPQAA4z0AAOQ9AADlPQAA5j0AAOc9AADoPQAA6T0AAOo9AADrPQAA7D0AAO09AADuPQAA7z0AAPA9AADxPQAA8j0AAPM9AAD0PQAA9T0AAPY9AAD3PQAA4D0AAPk9AAD6PQAA+z0AAPw9AAD9PQAA/j0AAP89AAAAPgCAAD4AAAE+AIABPgAAAj4AgAI+AAADPgCAAz4AAAQ+AIAEPgAABT4AgAU+AAAGPgCABj4AAAc+AIAHPgAACD4AgAg+AAAJPgCACT4AAAo+AIAKPgAACz4AgAs+AAAMPgCADD4AAA0+AIANPgAADj4AgA4+AAAPPgCADz4AABA+AIAQPgAAET4AgBE+AAASPgCAEj4AABM+AIATPgAAFD4AgBQ+AAAVPgCAFT4AABY+AIAWPgAAFz4AgBc+AAAYPgCAGD4AABk+AIAZPgAAGj4AgBo+AAAbPgCAGz4AABw+AIAcPgAAHT4AgB0+AAAePgCAHj4AAB8+AIAfPgAAID4AgCA+AAAhPgCAIT4AACI+AIAiPgAAIz4AgCM+AAAkPgCAJD4AACU+AIAlPgAAJj4AgCY+AAAnPgCAJz4AACg+AIAoPgAAKT4AgCk+AAAqPgCAKj4AACs+AIArPgAALD4AgCw+AAAtPgCALT4AAC4+AIAuPgAALz4AgC8+AAAwPgCAMD4AADE+AIAxPgAAMj4AgDI+AAAzPgCAMz4AADQ+AIA0PgAANT4AgDU+AAA2PgCANj4AADc+AIA3PgAAOD4AgDg+AAA5PgCAOT4AADo+AIA6PgAAOz4AgDs+AAA8PgCAPD4AAD0+AIA9PgAAPj4AgD4+AAA/PgCAPz4AAEA+AIBAPgAAQT4AgEE+AABCPgCAQj4AAEM+AIBDPgAARD4AgEQ+AABFPgCART4AAEY+AIBGPgAARz4AgEc+AABIPgCASD4AAEk+AIBJPgAASj4AgEo+AABLPgCASz4AAEw+AIBMPgAATT4AgE0+AABOPgCATj4AAE8+AIBPPgAAUD4AgFA+AABRPgCAUT4AAFI+AIBSPgAAUz4AgFM+AABUPgCAVD4AAFU+AIBVPgAAVj4AgFY+AABXPgCAVz4AAFg+AIBYPgAAWT4AgFk+AABaPgCAWj4AAFs+AIBbPgAAXD4AgFw+AABdPgCAXT4AAF4+AIBePgAAXz4AgF8+AABgPgCAYD4AAGE+AIBhPgAAYj4AgGI+AABjPgCAYz4AAGQ+AIBkPgAAZT4AgGU+AABmPgCAZj4AAGc+AIBnPgAAaD4AgGg+AABpPgCAaT4AAGo+AIBqPgAAaz4AgGs+AABsPgCAbD4AAG0+AIBtPgAAbj4AgG4+AABvPgCAbz4AAHA+AIBwPgAAcT4AgHE+AAByPgCAcj4AAHM+AIBzPgAAdD4AgHQ+AAB1PgCAdT4AAHY+AIB2PgAAdz4AgHc+AAB4PgCAeD4AAHk+AIB5PgAAej4AgHo+AAB7PgCAez4AAHw+AIB8PgAAfT4AgH0+AAB+PgCAfj4AAH8+AIB/PgAAgD4AQIA+AICAPgDAgD4AAIE+AECBPgCAgT4AwIE+AACCPgBAgj4AgII+AMCCPgAAgz4AQIM+AICDPgDAgz4AAIQ+AECEPgCAhD4AwIQ+AACFPgBAhT4AgIU+AMCFPgAAhj4AQIY+AICGPgDAhj4AAIc+AECHPgCAhz4AwIc+AACIPgBAiD4AgIg+AMCIPgAAiT4AQIk+AICJPgDAiT4AAIo+AECKPgCAij4AwIo+AACLPgBAiz4AgIs+AMCLPgAAjD4AQIw+AICMPgDAjD4AAI0+AECNPgCAjT4AwI0+AACOPgBAjj4AgI4+AMCOPgAAjz4AQI8+AICPPgDAjz4AAJA+AECQPgCAkD4AwJA+AACRPgBAkT4AgJE+AMCRPgAAkj4AQJI+AICSPgDAkj4AAJM+AECTPgCAkz4AwJM+AACUPgBAlD4AgJQ+AMCUPgAAlT4AQJU+AICVPgDAlT4AAJY+AECWPgCAlj4AwJY+AACXPgBAlz4AgJc+AMCXPgAAmD4AQJg+AICYPgDAmD4AAJk+AECZPgCAmT4AwJk+AACaPgBAmj4AgJo+AMCaPgAAmz4AQJs+AICbPgDAmz4AAJw+AECcPgCAnD4AwJw+AACdPgBAnT4AgJ0+AMCdPgAAnj4AQJ4+AICePgDAnj4AAJ8+AECfPgCAnz4AwJ8+AACgPgBAoD4AgKA+AMCgPgAAoT4AQKE+AIChPgDAoT4AAKI+AECiPgCAoj4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AoAk/AMAJPwDgCT8AAAo/ACAKPwBACj8AYAo/AIAKPwCgCj8AwAo/AOAKPwAACz8AIAs/AEALPwBgCz8AgAs/AKALPwDACz8A4As/AAAMPwAgDD8AQAw/AGAMPwCADD8AoAw/AMAMPwDgDD8AAA0/ACANPwBADT8AYA0/AIANPwCgDT8AwA0/AOANPwAADj8AIA4/AEAOPwBgDj8AgA4/AKAOPwDADj8A4A4/AAAPPwAgDz8AQA8/AGAPPwCADz8AoA8/AMAPPwDgDz8AABA/ACAQPwBAED8AYBA/AIAQPwCgED8AwBA/AOAQPwAAET8AIBE/AEARPwBgET8AgBE/AKARPwDAET8A4BE/AAASPwAgEj8AQBI/AGASPwCAEj8AoBI/AMASPwDgEj8AABM/ACATPwBAEz8AYBM/AIATPwCgEz8AwBM/AOATPwAAFD8AIBQ/AEAUPwBgFD8AgBQ/AKAUPwDAFD8A4BQ/AAAVPwAgFT8AQBU/AGAVPwCAFT8AoBU/AMAVPwDgFT8AABY/ACAWPwBAFj8AYBY/AIAWPwCgFj8AwBY/AOAWPwAAFz8AIBc/AEAXPwBgFz8AgBc/AKAXPwDAFz8A4Bc/AAAYPwAgGD8AQBg/AGAYPwCAGD8AoBg/AMAYPwDgGD8AABk/ACAZPwBAGT8AYBk/AIAZPwCgGT8AwBk/AOAZPwAAGj8AIBo/AEAaPwBgGj8AgBo/AKAaPwDAGj8A4Bo/AAAbPwAgGz8AQBs/AGAbPwCAGz8AoBs/AMAbPwDgGz8AABw/ACAcPwBAHD8AYBw/AIAcPwCgHD8AwBw/AOAcPwAAHT8AIB0/AEAdPwBgHT8AgB0/AKAdPwDAHT8A4B0/AAAePwAgHj8AQB4/AGAePwCAHj8AoH4/AMAePwDgHj8AAB8/ACAfPwBAHz8AYB8/AIAfPwCgHz8AwB8/AOAfPwAAID8AICA/AEAgPwBgID8AgCA/AKAgPwDAID8A4CA/AAAhPwAgIT8AQCE/AGAhPwCAIT8AoCE/AMAhPwDgIT8AACI/ACAiPwBAIj8AYCI/AIAiPwCgIj8AwCI/AOAiPwAAIz8AICM/AEAjPwBgIz8AgCM/AKAjPwDAIz8A4CM/AAAkPwAgJD8AQCQ/AGAkPwCAJD8AoCQ/AMAkPwDgJD8AACU/ACAlPwBAJT8AYCU/AIAlPwCgJT8AwCU/AOAlPwAAJj8AICY/AEAmPwBgJj8AgCY/AKAmPwDAJj8A4CY/AAAnPwAgJz8AQCc/AGAnPwCAJz8AoCc/AMAnPwDgJz8AACg/ACAoPwBAKD8AYCg/AIAoPwCgKD8AwCg/AOAoPwAAKT8AICk/AEApPwBgKT8AgCk/AKApPwDAKT8A4Ck/AAAqPwAgKj8AQCo/AGAqPwCAKj8AoCo/AMAqPwDgKj8AACs/ACArPwBAKz8AYCs/AIArPwCgKz8AwCs/AOArPwAALD8AICw/AEAsPwBgLD8AgCw/AKAsPwDALD8A4Cw/AAAtPwAgLT8AQC0/AGAtPwCALT8AoC0/AMAtPwDgLT8AAC4/ACAuPwBALj8AYC4/AIAuPwCgLj8AwC4/AOAuPwAALz8AIC8/AEAvPwBgLz8AgC8/AKAvPwDALz8A4C8/AAAwPwAgMD8AQDA/AGAwPwCAMD8AoDA/AMAwPwDgMD8AADE/ACAxPwBAMT8AYDE/AIAxPwCgMT8AwDE/AOAxPwAAMj8AIDI/AEAyPwBgMj8AgDI/AKAyPwDAMj8A4DI/AAAzPwAgMz8AQDM/AGAzPwCAMz8AoDM/AMAzPwDgMz8AADQ/ACA0PwBAND8AYDQ/AIA0PwCgND8AwDQ/AOA0PwAANT8AIDU/AEA1PwBgNT8AgDU/AKA1PwDANT8A4DU/AAA2PwAgNj8AQDY/AGA2PwCANj8AoDY/AMA2PwDgNj8AADc/ACA3PwBANz8AYDc/AIA3PwCgNz8AwDc/AOA3PwAAOD8AIDg/AEA4PwBgOD8AgDg/AKA4PwDAOD8A4Dg/AAA5PwAgOT8AQDk/AGA5PwCAOT8AoDk/AMA5PwDgOT8AADo/ACA6PwBAOj8AYDo/AIA6PwCgOj8AwDo/AOA6PwAAOz8AIDs/AEA7PwBgOz8AgDs/AKA7PwDAOz8A4Ds/AAA8PwAgPD8AQDw/AGA8PwCAPD8AoDw/AMA8PwDgPD8AAD0/ACA9PwBAPT8AYD0/AIA9PwCgPT8AwD0/AOA9PwAAPj8AID4/AEA+PwBgPj8AgD4/AKA+PwDAPj8A4D4/AAA/PwAgPz8AQD8/AGA/PwCAPz8AoD8/AMA/PwDgPz8AAEA/ACBAPwBAQD8AYEA/AIBAPwCgQD8AwEA/AOBAPwAAQT8AIEE/AEBBPwBgQT8AgEE/AKBBPwDAQT8A4EE/AABCPwAgQj8AQEI/AGBCPwCAQj8AoEI/AMBCPwDgQj8AAEM/ACBDPwBAQz8AYEM/AIBDPwCgQz8AwEM/AOBDPwAARD8AIEQ/AEBEPwBgRD8AgEQ/AKBEPwDARD8A4EQ/AABFPwAgRT8AQEU/AGBFPwCART8AoEU/AMBFPwDgRT8AAEY/ACBGPwBARj8AYEY/AIBGPwCgRj8AwEY/AOBGPwAARz8AIEc/AEBHPwBgRz8AgEc/AKBHPwDARz8A4Ec/AABIPwAgSD8AQEg/AGBIPwCASD8AoEg/AMBIPwDgSD8AAEk/ACBJPwBAST8AYEk/AIBJPwCgST8AwEk/AOBJPwAASj8AIEo/AEBKPwBgSj8AgEo/AKBKPwDASj8A4Eo/AABLPwAgSz8AQEs/AGBLPwCASz8AoEs/AMBLPwDgSz8AAEw/ACBMPwBATD8AYEw/AIBMPwCgTD8AwEw/AOBMPwAATT8AIE0/AEBNPwBgTT8AgE0/AKBNPwDATT8A4E0/AABOPwAgTj8AQE4/AGBOPwCATj8AoE4/AMBOPwDgTj8AAE8/ACBPPwBATz8AYE8/AIBPPwCgTz8AwE8/AOBPPwAAUD8AIFA/AEBQPwBgUD8AgFA/AKBQPwDAUD8A4FA/AABRPwAgUT8AQFE/AGBRPwCAUT8AoFE/AMBRPwDgUT8AAFI/ACBSPwBAUj8AYFI/AIBSPwCgUj8AwFI/AOBSPwAAUz8AIFM/AEBTPwBgUz8AgFM/AKBTPwDAUz8A4FM/AABUPwAgVD8AQFQ/AGBUPwCAVD8AoFQ/AMBUPwDgVD8AAFU/ACBVPwBAVT8AYFU/AIBVPwCgVT8AwFU/AOBVPwAAVj8AIFY/AEBWPwBgVj8AgFY/AKBWPwDAVj8A4FY/AABXPwAgVz8AQFc/AGBXPwCAVz8AoFc/AMBXPwDgVz8AAFg/ACBYPwBAWD8AYFg/AIBYPwCgWD8AwFg/AOBYPwAAWT8AIFk/AEBZPwBgWT8AgFk/AKBZPwDABZPwDgWj8AAFs/ACBbPwBAWz8AYFs/AIBbPwCgWz8AwFs/AOBbPwAAXD8AIFw/AEBcPwBgXD8AgFw/AKBcPwDAXD8A4Fw/AABdPwAgXT8AQF0/AGBdPwCAXT8AoF0/AMBdPwDgXT8AAF4/ACBePwBAXj8AYF4/AIBePwCgXj8AwF4/AOBePwAAXz8AIF8/AEBfPwBgXz8AgF8/AKBfPwDAXz8A4F8/AABgPwAgYD8AQGA/AGBgPwCAYD8AoGA/AMBgPwDgYD8AAGE/ACBhPwBAYT8AYGE/AIBhPwCgYT8AwGE/AOBhPwAAYj8AIGI/AEBiPwBgYj8AgGI/AKBiPwDAYj8A4GI/AABjPwAgYz8AQGM/AGBjPwCAYz8AoGM/AMBjPwDgYz8AAGQ/ACBkPwBAZD8AYGQ/AIBkPwCgZD8AwGQ/AOBkPwAAZT8AIGU/AEBlPwBgZT8AgGU/AKBlPwDAZT8A4GU/AABmPwAgZj8AQGY/AGBmPwCAZj8AoGY/AMBmPwDgZj8AAGc/ACBnPwBAZz8AYGc/AIBnPwCgZz8AwGc/AOBnPwAAaD8AIGg/AEBoPwBgaD8AgGg/AKBoPwDAaD8A4Gg/AABpPwAgaT8AQGk/AGBpPwCAaT8AoGk/AMBpPwDgaT8AAGo/ACBqPwBAaj8AYGo/AIBqPwCgaj8AwGo/AOBqPwAAaz8AIGs/AEBrPwBgaz8AgGs/AKBrPwDAaz8A4Gs/AABsPwAgbD8AQGw/AGBsPwCAbD8AoGw/AMBsPwDgbD8AAG0/ACBtPwBAbT8AYG0/AIBtPwCgbT8AwG0/AOBtPwAAbj8AIG4/AEBuPwBgbj8AgG4/AKBuPwDAbj8A4G4/AABvPwAgbz8AQG8/AGBvPwCAbz8AoG8/AMBvPwDgbz8AAHA/ACBwPwBAcD8AYHA/AIBwPwCgcD8AwHA/AOBwPwAAcT8AIHE/AEBxPwBgcT8AgHE/AKBxPwDAcT8A4HE/AAByPwAgcj8AQHI/AGByPwCAcj8AoHI/AMByPwDgcj8AAHM/ACBzPwBAcz8AYHM/AIBzPwCgcz8AwHM/AOBzPwAAdD8AIHQ/AEB0PwBgdD8AgHQ/AKB0PwDAdD8A4HQ/AAB1PwAgdT8AQHU/AGB1PwCAdT8AoHU/AMB1PwDgdT8AAHY/ACB2PwBAdj8AYHY/AIB2PwCgdj8AwHY/AOB2PwAAdz8AIHc/AEB3PwBgdz8AgHc/AKB3PwDAdz8A4Hc/AAB4PwAgeD8AQHg/AGB4PwCAeD8AoHg/AMB4PwDgeD8AAHk/ACB5PwBAeT8AYHk/AIB5PwCgeT8AwHk/AOB5PwAAej8AIHo/AEB6PwBgej8AgHo/AKB6PwDAej8A4Ho/AAB7PwAgez8AQHs/AGB7PwCAez8AoHs/AMB7PwDgez8AAHw/ACB8PwBAfD8AYHw/AIB8PwCgfD8AwHw/AOB8PwAAfT8AIH0/AEB9PwBgfT8AgH0/AKB9PwDAfT8A4H0/AAB+PwAgfj8AQH4/AGB+PwCAfj8AoH4/AMB+PwDgfj8AAH8/ACB/PwBAfz8AYH8/AIB/PwCgfz8AwH8/AOB/Pw==";
// Append Base64 Chunk 2 & XML Footer
//    tmpl += R"(
//           W2="" smoothW2="0" W3="" smoothW3="0"
//           A1="1" A2="1" A3="1"
//           interpolateW1="0" interpolateW2="0" interpolateW3="0" RELTRANS="50">
//           <key/>
//        </xpressive>
//      </instrument>
//      <eldata fcut="__FCUT__" fres="__FRES__" fwet="0" ftype="8">
//        <elvol amt="1" att="__ATT__" dec="__DEC__" hold="0.088" rel="__REL__" sustain="__SUS__"
//               pdel="0" userwavefile=""
//               lspd="0.1" lspd_numerator="4" lspd_denominator="4" lspd_syncmode="0"
//               lshp="0" latt="0" lamt="0" lpdel="0" ctlenvamt="0" x100="0" />
//        <elcut amt="1" att="0.496" dec="2" hold="0.5" rel="2" sustain="1"
//               pdel="0" userwavefile=""
//               lspd="0.1" lspd_numerator="4" lspd_denominator="4" lspd_syncmode="0"
//               lshp="0" latt="0" lamt="1" lpdel="0" ctlenvamt="0" x100="0"/>
//        <elres amt="0" att="0" dec="0.5" hold="0.5" rel="0.1" sustain="0.5"
//               pdel="0" userwavefile=""
//               lspd="0.1" lspd_numerator="4" lspd_denominator="4" lspd_syncmode="0"
//               lshp="0" latt="0" lamt="0" lpdel="0" ctlenvamt="0" x100="0"/>
//      </eldata>
//      <chordcreator chord="0" chordrange="1" chord-enabled="0"/>
//      <arpeggiator arp="0" arpmode="0" arprange="1" arptime="200" arpgate="100" arpdir="0"
//                   arprepeats="1" arpskip="0" arpmiss="0" arpcycle="0"
//                   arp-enabled="0" arptime_numerator="4" arptime_denominator="4" arptime_syncmode="0"/>
//      <fxchain enabled="1" numofeffects="4">
//        <effect wet="1" on="1" name="ladspaeffect" gate="0" autoquit="1"
//                autoquit_numerator="4" autoquit_denominator="4" autoquit_syncmode="0">
//          <ladspacontrols ports="6" link="1">
//            <port00 data="0" link="1"/>
//            <port01 link="1"><data scale_type="log" id="20283" value="2000"/></port01>
//            <port02 data="0.09875" link="1"/>
//            <port10 data="0"/>
//            <port11><data scale_type="log" id="23370" value="2000"/></port11>
//            <port12 data="0.09875"/>
//          </ladspacontrols>
//          <key><attribute name="file" value="ls_filter_1908"/><attribute name="plugin" value="lsFilter"/></key>
//        </effect>
//        <effect wet="1" on="1" name="ladspaeffect" gate="0" autoquit="1"
//                autoquit_numerator="4" autoquit_denominator="4" autoquit_syncmode="0">
//          <ladspacontrols ports="4">
//            <port02 data="__REV_BW__"/>
//            <port03 data="__REV_TAIL__"/>
//            <port04 data="__REV_DAMP__"/>
//            <port05 data="__REV_MIX__"/>
//          </ladspacontrols>
//          <key><attribute name="file" value="caps"/><attribute name="plugin" value="Plate2x2"/></key>
//        </effect>
//        <effect wet="0.93" on="1" name="ladspaeffect" gate="0" autoquit="1"
//                autoquit_numerator="4" autoquit_denominator="4" autoquit_syncmode="0">
//          <ladspacontrols ports="44">
//            <port04 data="0"/><port05 data="1"/><port06 data="1"/>
//            <port015><data scale_type="log" id="25760" value="66.874"/></port015>
//            <port016><data scale_type="log" id="30780" value="447.214"/></port016>
//            <port017><data scale_type="log" id="2395" value="2990.7"/></port017>
//            <port018 data="1"/><port019 data="0.124855"/><port020 data="1"/>
//            <port021><data scale_type="log" id="934" value="5"/></port021>
//            <port022><data scale_type="log" id="29520" value="94.5742"/></port022>
//            <port023 data="1"/><port024 data="2.75"/><port025 data="0"/>
//            <port028 data="1"/><port029 data="0"/><port030 data="0.250732"/>
//            <port031 data="1"/>
//            <port032><data scale_type="log" id="15981" value="94.5742"/></port032>
//            <port033><data scale_type="log" id="4398" value="94.5742"/></port033>
//            <port034 data="1"/><port035 data="2.75"/><port036 data="0"/>
//            <port039 data="1"/><port040 data="0"/><port041 data="0.250732"/>
//            <port042 data="1"/>
//            <port043><data scale_type="log" id="16655" value="94.5742"/></port043>
//            <port044><data scale_type="log" id="11640" value="94.5742"/></port044>
//            <port045 data="1"/><port046 data="2.75"/><port047 data="0"/>
//            <port050 data="1"/><port051 data="0"/><port052 data="0.000976563"/>
//            <port053 data="1"/>
//            <port054><data scale_type="log" id="4884" value="94.5742"/></port054>
//            <port055><data scale_type="log" id="21641" value="94.5742"/></port055>
//            <port056 data="1"/><port057 data="2.75"/><port058 data="0"/>
//            <port061 data="1"/><port062 data="0"/><port063 data="0"/>
//          </ladspacontrols>
//          <key><attribute name="file" value="veal"/><attribute name="plugin" value="MultibandCompressor"/></key>
//        </effect>
//        <effect wet="1" on="1" name="ladspaeffect" gate="0" autoquit="1"
//                autoquit_numerator="4" autoquit_denominator="4" autoquit_syncmode="0">
//          <ladspacontrols ports="10">
//            <port02 data="0"/><port03 data="0"/><port04 data="0"/>
//            <port05 data="-35.136"/><port06 data="-36.288"/><port07 data="0"/>
//            <port08 data="18.144"/><port09 data="20.448"/><port010 data="0"/><port011 data="0"/>
//          </ladspacontrols>
//          <key><attribute name="file" value="caps"/><attribute name="plugin" value="Eq2x2"/></key>
//        </effect>
//      </fxchain>
//    </instrumenttrack>
//  </instrumenttracksettings>
//</lmms-project>
//)";

    // 2. GENERATE VALUES
//    double vol = pianoVolSlider->value();
//    double cut = pianoCutoffSlider->value();
//    double res = pianoResSlider->value() / 100.0;

    // Envelope (approx seconds)
//    double att = pianoAttSlider->value() / 100.0;
//    double dec = pianoDecSlider->value() / 100.0;
//    double sus = pianoSusSlider->value() / 100.0;
//    double rel = pianoRelSlider->value() / 100.0;

    // Math Mixing
//    double mSaw  = pianoMixSawSlider->value() / 100.0;
//    double mSub  = pianoMixSubSlider->value() / 100.0;
//    double mHigh = pianoMixHighSlider->value() / 100.0;

 //   QString mathExpr = QString("%1*saww(integrate(f)) + %2*saww(integrate(semitone(-12)f)) + %3*saww(integrate(semitone(12)f))")
 //                          .arg(mSaw).arg(mSub).arg(mHigh);

    // Reverb Params
 //   double revBW   = pianoRevBandwidth->value() / 100.0;
 //   double revTail = pianoRevTail->value() / 100.0;
 //   double revDamp = pianoRevDamping->value() / 100.0;
 //   double revMix  = pianoRevMix->value() / 100.0; // 0.28125 is default

    // 3. REPLACE PLACEHOLDERS
//    tmpl.replace("__VOL__", QString::number(vol));
//    tmpl.replace("__FCUT__", QString::number(cut));
//    tmpl.replace("__FRES__", QString::number(res));

//    tmpl.replace("__ATT__", QString::number(att));
//    tmpl.replace("__DEC__", QString::number(dec));
//    tmpl.replace("__SUS__", QString::number(sus));
//    tmpl.replace("__REL__", QString::number(rel));

//    tmpl.replace("__MATH_EXPR__", mathExpr); // Inject new formula

//    tmpl.replace("__REV_BW__", QString::number(revBW));
//    tmpl.replace("__REV_TAIL__", QString::number(revTail));
//    tmpl.replace("__REV_DAMP__", QString::number(revDamp));
//    tmpl.replace("__REV_MIX__", QString::number(revMix));

    // Show preview of generated logic
//    pianoSourceEdit->setText(mathExpr);

    // 4. SAVE
//    QString path = QFileDialog::getSaveFileName(this, "Save Legacy Piano", "", "LMMS Instrument (*.xpf)");
//    if (path.isEmpty()) return;
//
//    QFile file(path);
//    if (file.open(QIODevice::WriteOnly)) {
//        QTextStream stream(&file);
//        stream << tmpl;
//        file.close();
//        pianoStatusLabel->setText("Saved successfully to: " + path);
//    } else {
//        pianoStatusLabel->setText("Error saving file.");
//    }
//}
