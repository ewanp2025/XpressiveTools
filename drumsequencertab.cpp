#include "drumsequencertab.h"
#include <QClipboard>
#include <QApplication>
#include <QRegularExpression>

DrumSequencerTab::DrumSequencerTab(QWidget *parent)
    : QWidget(parent), isUpdatingPreset(false)
{
    setupUi();
    onPresetChanged(0);
}

DrumSequencerTab::~DrumSequencerTab() {}

void DrumSequencerTab::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // --- Top Control Bar ---
    QHBoxLayout* topLayout = new QHBoxLayout();

    topLayout->addWidget(new QLabel("Syntax Mode:", this));
    modeSelector = new QComboBox(this);
    modeSelector->addItems({"Legacy (Inline String)", "Nightly (Variables)"});
    topLayout->addWidget(modeSelector);

    topLayout->addSpacing(20);

    topLayout->addWidget(new QLabel("Preset Pattern:", this));
    presetSelector = new QComboBox(this);


    presetSelector->addItems({
        "TEST1",
        "TEST2",
        "TEST3",
        "TEST4",
        "TEST5",
        "TEST6",
        "TEST7",
        "TEST8",
        "TEST9"
    });
    topLayout->addWidget(presetSelector);

    topLayout->addStretch();

    enableDistortion = new QCheckBox("Enable Master Overdrive (Clamp)", this);
    enableDistortion->setChecked(true);
    topLayout->addWidget(enableDistortion);

    mainLayout->addLayout(topLayout);


    QGroupBox* gridGroup = new QGroupBox("16-Step Sequencer Grid", this);
    QGridLayout* gridLayout = new QGridLayout(gridGroup);
    gridLayout->setSpacing(4);


    for (int s = 0; s < NUM_STEPS; ++s) {
        QLabel* stepLbl = new QLabel(QString::number(s), this);
        stepLbl->setAlignment(Qt::AlignCenter);
        if (s % 4 == 0) {
            stepLbl->setStyleSheet("font-weight: bold; color: #00ffff;");
        }
        gridLayout->addWidget(stepLbl, 0, s + 1);
    }


    QStringList soundNames = {
        "None",
        "Punchy Kick 1", "Deep Kick 2",
        "Punchy Snare 1", "Splash Snare 2",
        "Closed Hat", "Open Hat",
        "High Zap", "Low Zap",
        "Low Percussion", "High Percussion", "Mid Percussion",
        "Metallic", "Woodblock",
        "Deep Pitch-Drop Drum", "High Resonant Drum", "Triangle Pot Ping",
        "Synthetic Jaw Harp", "FM Water Chime", "Booming Resonant Drum",
        "Square Wood Clack", "Saw/Sine Pluck", "Metallic Rattle"
    };

    for (int t = 0; t < NUM_TRACKS; ++t) {
        trackSounds[t] = new QComboBox(this);
        trackSounds[t]->addItems(soundNames);
        trackSounds[t]->setMinimumWidth(160);
        gridLayout->addWidget(trackSounds[t], t + 1, 0);

        connect(trackSounds[t], &QComboBox::currentIndexChanged, this, &DrumSequencerTab::onUiChanged);

        for (int s = 0; s < NUM_STEPS; ++s) {
            stepGrid[t][s] = new QCheckBox(this);
            if (s % 4 == 0) {
                stepGrid[t][s]->setStyleSheet("background-color: #224444; border-radius: 2px;");
            }
            gridLayout->addWidget(stepGrid[t][s], t + 1, s + 1, Qt::AlignCenter);
            connect(stepGrid[t][s], &QCheckBox::toggled, this, &DrumSequencerTab::onUiChanged);
        }
    }

    mainLayout->addWidget(gridGroup);


    QGroupBox* outGroup = new QGroupBox("Generated Math Expression (O1 / O2)", this);
    QVBoxLayout* outLayout = new QVBoxLayout(outGroup);

    codeOutput = new QTextEdit(this);
    codeOutput->setFontFamily("Monospace");
    codeOutput->setMinimumHeight(150);
    outLayout->addWidget(codeOutput);

    btnCopy = new QPushButton("Copy to Clipboard", this);
    outLayout->addWidget(btnCopy);
    connect(btnCopy, &QPushButton::clicked, this, &DrumSequencerTab::copyToClipboard);

    mainLayout->addWidget(outGroup);

    // Wire up main top controls
    connect(modeSelector, &QComboBox::currentIndexChanged, this, &DrumSequencerTab::onUiChanged);
    connect(enableDistortion, &QCheckBox::toggled, this, &DrumSequencerTab::onUiChanged);
    connect(presetSelector, &QComboBox::currentIndexChanged, this, &DrumSequencerTab::onPresetChanged);
}

QString DrumSequencerTab::getSoundMath(int index)
{
    switch (index) {
    case 1:  return "sinew(integrate(60*(1+2*exp(-mod(t,15/tempo)*100))))*exp(-mod(t,15/tempo)*15)";
    case 2:  return "sinew(integrate(55*(1+2*exp(-mod(t,15/tempo)*80))))*exp(-mod(t,15/tempo)*12)";
    case 3:  return "(sinew(integrate(200))*exp(-mod(t,15/tempo)*40)+0.6*randv(t*srate)*exp(-mod(t,15/tempo)*20))";
    case 4:  return "(sinew(integrate(180))*exp(-mod(t,15/tempo)*50)+0.8*randv(t*srate)*exp(-mod(t,15/tempo)*12))";
    case 5:  return "0.1*randv(t*srate)*exp(-mod(t,15/tempo)*40)";
    case 6:  return "0.4*randv(t*srate)*exp(-mod(t,15/tempo)*15)";
    case 7:  return "sinew(integrate(f*2*(1+5*exp(-mod(t,15/tempo)*80))))*exp(-mod(t,15/tempo)*12)";
    case 8:  return "sinew(integrate(f*0.25*(1+4*exp(-mod(t,15/tempo)*60))))*exp(-mod(t,15/tempo)*15)";
    case 9:  return "sinew(integrate(f*0.25*(1+1.5*exp(-mod(t,15/tempo)*40))))*exp(-mod(t,15/tempo)*12)";
    case 10: return "sinew(integrate(f*1.5*(1+0.5*exp(-mod(t,15/tempo)*60))))*exp(-mod(t,15/tempo)*15)";
    case 11: return "sinew(integrate(f*0.8*(1+0.5*exp(-mod(t,15/tempo)*50))))*exp(-mod(t,15/tempo)*12)";
    case 12: return "trianglew(integrate(f*3.5))*exp(-mod(t,15/tempo)*25)";
    case 13: return "trianglew(integrate(f*1.5))*exp(-mod(t,15/tempo)*15)";
    case 14: return "sinew(integrate(f*0.25*(1+1.5*exp(-mod(t,15/tempo)*40))))*exp(-mod(t,15/tempo)*12)";
    case 15: return "sinew(integrate(f*1.5*(1+0.5*exp(-mod(t,15/tempo)*60))))*exp(-mod(t,15/tempo)*15)";
    case 16: return "trianglew(integrate(f*3.5))*exp(-mod(t,15/tempo)*25)";
    case 17: return "(0.5*squarew(integrate(f*0.25*(1+6*exp(-mod(t,15/tempo)*40))))+0.5*saww(integrate(f*0.25)))*exp(-mod(t,15/tempo)*12)";
    case 18: return "sinew(integrate(f*4+1.5*sinew(integrate(f*1.5))))*exp(-mod(t,15/tempo)*10)";
    case 19: return "sinew(integrate(f*0.3*(1+2.5*exp(-mod(t,15/tempo)*60))))*exp(-mod(t,15/tempo)*12)";
    case 20: return "squarew(integrate(f*3))*exp(-mod(t,15/tempo)*30)";
    case 21: return "(0.7*saww(integrate(f*0.5))+0.3*sinew(integrate(f*0.5)))*exp(-mod(t,15/tempo)*18)";
    case 22: return "0.15*randv(t*srate)*exp(-mod(t,15/tempo)*25)";
    default: return "";
    }
}

void DrumSequencerTab::generateMath()
{
    if (isUpdatingPreset) return;

    bool isNightly = (modeSelector->currentIndex() == 1);
    bool doClamp = enableDistortion->isChecked();

    QStringList trackOutputs;
    QString variablesBlock = "";

    if (isNightly) {
        variablesBlock += "var step := floor(mod(t*(tempo/15), 16));\n";
        variablesBlock += "var tStep := mod(t, 15/tempo);\n";
    }

    for (int t = 0; t < NUM_TRACKS; ++t) {
        int soundIdx = trackSounds[t]->currentIndex();
        if (soundIdx == 0) continue; // "None" selected

        QStringList activeSteps;
        for (int s = 0; s < NUM_STEPS; ++s) {
            if (stepGrid[t][s]->isChecked()) {
                if (isNightly) {
                    activeSteps << QString("step==%1").arg(s);
                } else {
                    activeSteps << QString("floor(mod(t*(tempo/15), 16))==%1").arg(s);
                }
            }
        }

        if (activeSteps.isEmpty()) continue;

        QString condition = "(" + activeSteps.join(" | ") + ")";
        QString rawMath = getSoundMath(soundIdx);

        if (isNightly) {
            rawMath.replace("mod(t,15/tempo)", "tStep");
            QString trkVar = QString("trk%1").arg(t + 1);
            variablesBlock += QString("var %1 := %2 * %3;\n").arg(trkVar, rawMath, condition);
            trackOutputs << trkVar;
        } else {
            trackOutputs << QString("%1 * %2").arg(rawMath, condition);
        }
    }

    QString finalExpression = "";

    if (trackOutputs.isEmpty()) {
        finalExpression = "0.0";
    } else {
        QString combined = trackOutputs.join(" + ");

        if (isNightly) {
            finalExpression = variablesBlock + "\n";
            if (doClamp) finalExpression += QString("clamp(-1.0, %1, 1.0);").arg(combined);
            else         finalExpression += combined + ";";
        } else {
            if (doClamp) finalExpression = QString("clamp(-1, %1, 1)").arg(combined);
            else         finalExpression = combined;
        }
    }

    codeOutput->setText(finalExpression);
}

void DrumSequencerTab::onUiChanged()
{
    generateMath();
}

void DrumSequencerTab::copyToClipboard()
{
    QApplication::clipboard()->setText(codeOutput->toPlainText());
}

void DrumSequencerTab::onPresetChanged(int index)
{
    isUpdatingPreset = true;

    for (int t = 0; t < NUM_TRACKS; ++t) {
        trackSounds[t]->setCurrentIndex(0);
        for (int s = 0; s < NUM_STEPS; ++s) {
            stepGrid[t][s]->setChecked(false);
        }
    }

    auto setTrack = [&](int trkIdx, int soundIdx, std::vector<int> steps) {
        trackSounds[trkIdx]->setCurrentIndex(soundIdx);
        for (int s : steps) stepGrid[trkIdx][s]->setChecked(true);
    };

    switch (index) {
    case 0:
        setTrack(0, 1, {0, 4, 8, 12});           // Kick 1
        setTrack(1, 3, {4, 12});                 // Snare 1
        setTrack(2, 5, {2, 6, 10, 14});          // Closed Hat
        break;

    case 1:
        setTrack(0, 1, {0, 4, 8, 12});           // Kick 1
        setTrack(1, 3, {4, 12});                 // Snare 1
        setTrack(2, 9, {3, 7, 11, 14});          // Low Percussion
        setTrack(3, 10, {2, 5, 9, 13, 15});      // High Percussion
        setTrack(4, 5, {1, 3, 5, 7, 9, 11, 13, 15}); // Closed Hat
        break;

    case 2:
        setTrack(0, 1, {0, 4, 8, 12});           // Kick 1
        setTrack(1, 14, {2, 7, 10, 15});         // Deep Pitch-Drop Drum
        setTrack(2, 15, {3, 5, 9, 11, 13});      // High Resonant Drum
        setTrack(3, 16, {1, 6, 14});             // Triangle Pot Ping
        setTrack(4, 22, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}); // Metallic Rattle
        break;

    case 3:
        setTrack(0, 1, {0, 4, 8, 12});           // Kick 1
        setTrack(1, 17, {2, 5, 9, 13, 14});      // Synthetic Jaw Harp
        setTrack(2, 15, {3, 7, 11, 15});         // High Resonant Drum
        setTrack(3, 22, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}); // Metallic Rattle
        break;

    case 4:
        setTrack(0, 2, {0, 8, 11});              // Deep Kick 2 (Electro Bounce)
        setTrack(1, 15, {3, 7, 10, 14, 15});     // High Resonant Drum
        setTrack(2, 14, {2, 5, 9, 13});          // Deep Pitch-Drop Drum
        setTrack(3, 16, {1, 4, 6, 12});          // Triangle Pot Ping
        setTrack(4, 22, {2, 6, 10, 14});         // Metallic Rattle (Upbeat)
        break;

    case 5:
        setTrack(0, 2, {0, 2, 7, 10});           // Deep Kick 2
        setTrack(1, 4, {4, 9, 11, 12, 13, 14});  // Splash Snare 2
        setTrack(2, 5, {0, 2, 4, 6, 8, 10, 12, 14}); // Closed Hat
        setTrack(3, 6, {1, 3, 5, 7, 9, 11, 13, 15}); // Open Hat
        setTrack(4, 13, {10, 14});               // Woodblock
        break;

    case 6:
        setTrack(0, 1, {0, 4, 8, 12});           // Kick 1
        setTrack(1, 6, {2, 6, 10, 14});          // Open Hat
        setTrack(2, 14, {3, 5, 9, 11});          // Deep Pitch-Drop Drum
        setTrack(3, 3, {4, 12});                 // Snare 1
        setTrack(4, 5, {1, 3, 5, 7, 9, 11, 13, 15}); // Closed Hat
        break;

    case 7:
        setTrack(0, 2, {0, 8, 11});              // Deep Kick 2
        setTrack(1, 4, {4, 12});                 // Splash Snare 2
        setTrack(2, 5, {0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15}); // Closed Hat
        setTrack(3, 6, {2, 6, 10, 14});          // Open Hat (Upbeat)
        setTrack(4, 10, {2, 5, 9, 13});          // High Percussion (Cowbell syncopation)
        break;

    case 8:
        setTrack(0, 1, {0, 4, 8, 12});           // Punchy Kick 1
        setTrack(1, 3, {4, 12});                 // Punchy Snare 1
        setTrack(2, 6, {2, 6, 10, 14});          // Open Hat
        setTrack(3, 8, {14, 15});                // Low Zap (Turnaround)
        setTrack(4, 7, {3, 7, 11});              // High Zap (Syncopated)
        break;
    }

    isUpdatingPreset = false;
    generateMath();
}