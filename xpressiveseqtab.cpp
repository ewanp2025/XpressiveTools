#include "xpressiveseqtab.h"
#include <QRegularExpression>

XpressiveSeqTab::XpressiveSeqTab(QWidget *parent) : QWidget(parent), isUpdatingUI(false)
{
    setupUi();
    generateString();
}

XpressiveSeqTab::~XpressiveSeqTab() {}

void XpressiveSeqTab::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);


    QHBoxLayout* topLayout = new QHBoxLayout();
    modeSelector = new QComboBox(this);
    modeSelector->addItems({"Legacy (Inline String)", "Nightly (Variables)"});
    topLayout->addWidget(new QLabel("Syntax Mode:", this));
    topLayout->addWidget(modeSelector);

    numStepsSpin = new QSpinBox(this);
    numStepsSpin->setRange(2, 32);
    numStepsSpin->setValue(16);
    topLayout->addWidget(new QLabel("Steps:", this));
    topLayout->addWidget(numStepsSpin);

      toggleHardGate = new QCheckBox("Hard Gate Rests", this);
    toggleHardGate->setToolTip("When checked, audio is instantly chopped on empty steps. When unchecked, audio bleeds through rests.");
    toggleHardGate->setChecked(false);
    topLayout->addWidget(toggleHardGate);

    connect(toggleHardGate, &QCheckBox::toggled, this, &XpressiveSeqTab::onUiChanged);

    topLayout->addStretch();
    mainLayout->addLayout(topLayout);


    QGroupBox* modGroup = new QGroupBox("Macro Modulators & Math FX", this);
    QGridLayout* modLayout = new QGridLayout();

    auto createDial = [this, modLayout](int col, const QString& title, int min, int max, int defaultVal, QDial*& dial, QLabel*& lbl) {
        dial = new QDial(this);
        dial->setRange(min, max);
        dial->setValue(defaultVal);

        QLabel* titleLbl = new QLabel(title, this);
        titleLbl->setWordWrap(true);
        titleLbl->setAlignment(Qt::AlignCenter);
        titleLbl->setMinimumWidth(70);

        lbl = new QLabel(QString::number(defaultVal), this);
        lbl->setAlignment(Qt::AlignCenter);

        modLayout->addWidget(titleLbl, 0, col, Qt::AlignCenter);
        modLayout->addWidget(dial, 1, col);
        modLayout->addWidget(lbl, 2, col);
        connect(dial, &QDial::valueChanged, this, &XpressiveSeqTab::onUiChanged);
    };

    createDial(0, "Drive Base", 1, 40, 2, dialDriveBase, lblDriveBase);
    createDial(1, "Drive Sweep", 0, 40, 10, dialDriveSweep, lblDriveSweep);
    createDial(2, "Decay Base", 1, 30, 20, dialDecayBase, lblDecayBase);

    createDial(3, "Decay Sweep", -30, 30, -16, dialDecaySweep, lblDecaySweep);

    createDial(4, "Clock (Bars)", 120, 3840, 1920, dialMacroClock, lblMacroClock);
    createDial(5, "O2 Detune", 1000, 1010, 1002, dialDetune, lblDetune);

    createDial(6, "Squelch Decay", 1, 50, 8, dialSquDecay, lblSquDecay);
    modGroup->setLayout(modLayout);
    mainLayout->addWidget(modGroup);


    QGroupBox* psychGroup = new QGroupBox("Acoustics & Architecture", this);
    QGridLayout* psychLayout = new QGridLayout();

    toggleNSCLathe = new QCheckBox("NSC Lathe Constraint", this);
    comboFilterType = new QComboBox(this);
    comboFilterType->addItems({"Woody Wavefold (Sine)", "Warm Overdrive (Tanh)", "Hollow Tube (Cos)", "Digital Tear (Abs)"});

    psychLayout->addWidget(toggleNSCLathe, 0, 0);
    psychLayout->addWidget(comboFilterType, 0, 1);


    toggleWobble = new QCheckBox("Enable Resonant Wobble", this);
    toggleWobble->setChecked(true);
    createDial(2, "Timbral Smear", 0, 100, 15, dialTimbralSmear, lblTimbralSmear);
    psychLayout->addWidget(toggleWobble, 0, 2, Qt::AlignCenter);
    psychLayout->addWidget(dialTimbralSmear, 1, 2);
    psychLayout->addWidget(lblTimbralSmear, 2, 2);


    toggleEcho = new QCheckBox("Math Echo (3/16ths)", this);
    toggleEcho->setChecked(false);
    createDial(3, "Echo Amount", 0, 100, 40, dialEchoAmt, lblEchoAmt);
    psychLayout->addWidget(toggleEcho, 0, 3, Qt::AlignCenter);
    psychLayout->addWidget(dialEchoAmt, 1, 3);
    psychLayout->addWidget(lblEchoAmt, 2, 3);


    toggleEvolution = new QCheckBox("Evolution Trigger", this);
    spinEvolutionBar = new QSpinBox(this);
    spinEvolutionBar->setRange(2, 128);
    spinEvolutionBar->setValue(33);

    QHBoxLayout* evolveLayout = new QHBoxLayout();
    evolveLayout->addWidget(toggleEvolution);
    evolveLayout->addWidget(new QLabel("Bar:", this));
    evolveLayout->addWidget(spinEvolutionBar);
    psychLayout->addLayout(evolveLayout, 1, 0, 1, 2);

    connect(toggleNSCLathe, &QCheckBox::toggled, this, &XpressiveSeqTab::onUiChanged);
    connect(comboFilterType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &XpressiveSeqTab::onUiChanged);
    connect(toggleWobble, &QCheckBox::toggled, this, &XpressiveSeqTab::onUiChanged);
    connect(toggleEcho, &QCheckBox::toggled, this, &XpressiveSeqTab::onUiChanged);
    connect(toggleEvolution, &QCheckBox::toggled, this, &XpressiveSeqTab::onUiChanged);
    connect(spinEvolutionBar, QOverload<int>::of(&QSpinBox::valueChanged), this, &XpressiveSeqTab::onUiChanged);

    psychGroup->setLayout(psychLayout);
    mainLayout->addWidget(psychGroup);


    QGroupBox* seqGroup = new QGroupBox("Step Sequencer (Trig | Accent | Pitch)", this);
    QGridLayout* seqLayout = new QGridLayout();
    for(int i = 0; i < 32; i++) {
        QVBoxLayout* cellLayout = new QVBoxLayout();

        stepToggles[i] = new QCheckBox(QString::number(i), this);
        stepToggles[i]->setChecked(true);

        stepAccents[i] = new QCheckBox("Acc", this);

        stepPitches[i] = new QComboBox(this);
        stepPitches[i]->addItems({"-1 Oct", "Root", "Min 3rd", "Perf 5th", "+1 Oct"});
        stepPitches[i]->setCurrentIndex(1);


        stepSquelch[i] = new QCheckBox("Squ", this);
        stepSquelch[i]->setChecked(false);

        stepSquAmt[i] = new QSpinBox(this);
        stepSquAmt[i]->setRange(1, 30);
        stepSquAmt[i]->setValue(15);
        stepSquAmt[i]->setEnabled(false);

        QFont smallFont = stepPitches[i]->font();
        smallFont.setPointSize(8);
        stepToggles[i]->setFont(smallFont);
        stepAccents[i]->setFont(smallFont);
        stepPitches[i]->setFont(smallFont);
        stepSquelch[i]->setFont(smallFont);
        stepSquAmt[i]->setFont(smallFont);

        cellLayout->addWidget(stepToggles[i]);
        cellLayout->addWidget(stepAccents[i]);
        cellLayout->addWidget(stepPitches[i]);
        cellLayout->addWidget(stepSquelch[i]);
        cellLayout->addWidget(stepSquAmt[i]);


        connect(stepSquelch[i], &QCheckBox::toggled, [=](bool checked){
            stepSquAmt[i]->setEnabled(checked);
            onUiChanged();
        });
        connect(stepSquAmt[i], QOverload<int>::of(&QSpinBox::valueChanged), this, &XpressiveSeqTab::onUiChanged);
        cellLayout->setSpacing(0);
        cellLayout->setContentsMargins(0,0,0,0);

        seqLayout->addLayout(cellLayout, i / 16, i % 16);

        connect(stepToggles[i], &QCheckBox::toggled, this, &XpressiveSeqTab::onUiChanged);
        connect(stepAccents[i], &QCheckBox::toggled, this, &XpressiveSeqTab::onUiChanged);
        connect(stepPitches[i], QOverload<int>::of(&QComboBox::currentIndexChanged), this, &XpressiveSeqTab::onUiChanged);
    }
    seqGroup->setLayout(seqLayout);
    mainLayout->addWidget(seqGroup);


    codeOutputO1 = new QTextEdit(this);
    codeOutputO2 = new QTextEdit(this);

    mainLayout->addWidget(new QLabel("Oscillator 1 (Left):", this));
    mainLayout->addWidget(codeOutputO1);
    mainLayout->addWidget(new QLabel("Oscillator 2 (Right):", this));
    mainLayout->addWidget(codeOutputO2);

    connect(modeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &XpressiveSeqTab::onUiChanged);
    connect(numStepsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &XpressiveSeqTab::onUiChanged);
}

void XpressiveSeqTab::onUiChanged()
{
    if (isUpdatingUI) return;

    lblDriveBase->setText(QString::number(dialDriveBase->value()));
    lblDriveSweep->setText(QString::number(dialDriveSweep->value()));
    lblDecayBase->setText(QString::number(dialDecayBase->value()));
    lblDecaySweep->setText(QString::number(dialDecaySweep->value()));
    lblMacroClock->setText(QString::number(dialMacroClock->value()));
    lblDetune->setText(QString::number(dialDetune->value() / 1000.0, 'f', 3));
    lblSquDecay->setText(QString::number(dialSquDecay->value()));
    lblTimbralSmear->setText(QString("%1%").arg(dialTimbralSmear->value()));
    lblEchoAmt->setText(QString("%1%").arg(dialEchoAmt->value()));

    generateString();
}

void XpressiveSeqTab::generateString()
{
    isUpdatingUI = true;

    float drBase = dialDriveBase->value();
    float drSweep = dialDriveSweep->value();
    float decBase = dialDecayBase->value();
    float decSweep = dialDecaySweep->value();
    int macro = dialMacroClock->value();
    float detune = dialDetune->value() / 1000.0f;
    float smear = dialTimbralSmear->value() / 100.0f;
    float echoAmt = dialEchoAmt->value() / 100.0f;
    int steps = numStepsSpin->value();
    float squDecayBase = dialSquDecay->value();

    QString trigStr = "(";
    QString accLogic = "";
    QString pitchLogic = "";
    QString squLogic = "";
    bool hasAcc = false;
    bool hasPitch = false;
    bool hasSqu = false;

    for(int i = 0; i < steps; i++) {
        if(stepToggles[i]->isChecked()) {
            trigStr += QString("floor(mod(t*(tempo/15), %1))==%2 | ").arg(steps).arg(i);

            if(stepAccents[i]->isChecked()) {
                accLogic += QString("floor(mod(t*(tempo/15), %1))==%2 ? 2.5 : ").arg(steps).arg(i);
                hasAcc = true;
            }

            if(stepSquelch[i]->isChecked()) {
                squLogic += QString("floor(mod(t*(tempo/15), %1))==%2 ? %3 : ").arg(steps).arg(i).arg(stepSquAmt[i]->value());
                hasSqu = true;
            }
        }


        if(stepPitches[i]->currentIndex() != 1) {
            float mult = 1.0f;
            int idx = stepPitches[i]->currentIndex();
            if(idx == 0) mult = 0.5f; else if(idx == 2) mult = 1.189f;
            else if(idx == 3) mult = 1.498f; else if(idx == 4) mult = 2.0f;
            pitchLogic += QString("floor(mod(t*(tempo/15), %1))==%2 ? %3 : ").arg(steps).arg(i).arg(mult);
            hasPitch = true;
        }
    }

    if (trigStr.length() > 2) trigStr.chop(3);
    trigStr += ")";
    if (trigStr == "()") trigStr = "0";

    accLogic = hasAcc ? ("(" + accLogic + "1.0)") : "1.0";
    pitchLogic = hasPitch ? ("(" + pitchLogic + "1.0)") : "1.0";
    squLogic = hasSqu ? ("(" + squLogic + "0.0)") : "0.0";

    QString evolveMathO1 = "";
    QString evolveMathO2 = "";
    if (toggleEvolution->isChecked()) {
        QString evolveGate = QString("(t > %1 * (240/tempo))").arg(spinEvolutionBar->value());
        evolveMathO1 = QString(" + %1 * (saww(integrate(f*1.498)) + saww(integrate(f*2.0)))").arg(evolveGate);
        evolveMathO2 = QString(" + %1 * (saww(integrate(f*1.501)) + saww(integrate(f*1.996)))").arg(evolveGate);
    }


    QString foldFunc = "sin";
    int fType = comboFilterType->currentIndex();
    if(fType == 1) foldFunc = "tanh"; else if(fType == 2) foldFunc = "cos"; else if(fType == 3) foldFunc = "abs";


    QString driftMath = toggleWobble->isChecked() ? QString(" + (%1 * (sinew(t*0.113) + sinew(t*0.197)))").arg(smear) : "";
    QString decayStr = QString("(%1 - %2 * mod(t*(tempo/%3), 1.0))").arg(decBase).arg(decSweep).arg(macro);

    QString finalO1, finalO2;

    if (modeSelector->currentIndex() == 0) {

        QString driveStrLegacy = QString("((%1 + %2 * mod(t*(tempo/%3), 1.0)%4) * %5 * (1.0 + %6 * exp(-mod(t*(tempo/15), 1.0) * %7)))").arg(drBase).arg(drSweep).arg(macro).arg(driftMath).arg(accLogic).arg(squLogic).arg(squDecayBase);

        QString noise1Legacy = QString("(randsv(t*srate,0) * (%1 * 0.005))").arg(squLogic);
        QString noise2Legacy = QString("(randsv(t*srate*1.1,0) * (%1 * 0.005))").arg(squLogic);

        QString coreO1Legacy = QString("(saww(integrate(f * %1))%2)").arg(pitchLogic, evolveMathO1);
        QString coreO2Legacy = QString("(saww(integrate(f * %1 * %2))%3)").arg(pitchLogic).arg(detune).arg(evolveMathO2);


        QString gateLogic = toggleHardGate->isChecked() ? QString(" * %1").arg(trigStr) : "";

        if(toggleNSCLathe->isChecked()) {
            QString monoSubLegacy = QString("sinew(integrate(f * %1)) * 0.5").arg(pitchLogic);
            finalO1 = QString("clamp(-1.0, (%1 + %2(%3 * %4) + %5) * exp(-mod(t*(tempo/15), 1.0) * %6)%7, 1.0)").arg(monoSubLegacy, foldFunc, coreO1Legacy, driveStrLegacy, noise1Legacy, decayStr, gateLogic);
            finalO2 = QString("clamp(-1.0, (%1 + %2(%3 * %4) + %5) * exp(-mod(t*(tempo/15), 1.0) * %6)%7, 1.0)").arg(monoSubLegacy, foldFunc, coreO2Legacy, driveStrLegacy, noise2Legacy, decayStr, gateLogic);
        } else {
            finalO1 = QString("clamp(-1.0, (%1(%2 * %3) + %4) * exp(-mod(t*(tempo/15), 1.0) * %5)%6, 1.0)").arg(foldFunc, coreO1Legacy, driveStrLegacy, noise1Legacy, decayStr, gateLogic);
            finalO2 = QString("clamp(-1.0, (%1(%2 * %3) + %4) * exp(-mod(t*(tempo/15), 1.0) * %5)%6, 1.0)").arg(foldFunc, coreO2Legacy, driveStrLegacy, noise2Legacy, decayStr, gateLogic);
        }

    } else {

        QString driveStrNightly = QString("(%1 + %2 * mod(t*(tempo/%3), 1.0)%4) * acc").arg(drBase).arg(drSweep).arg(macro).arg(driftMath);
        QString coreO1Nightly = QString("(saww(integrate(f*pitch))%1)").arg(evolveMathO1);
        QString coreO2Nightly = QString("(saww(integrate(f*pitch*%1))%2)").arg(detune).arg(evolveMathO2);

        QString varSetup = QString("var trig = %1;\nvar pitch = %2;\nvar acc = %3;\nvar squAmt = %4;\nvar macro = mod(t*(tempo/%5), 1.0);\nvar drive = %6;\nvar decay = %7;\nvar squEnv = exp(-mod(t*(tempo/15), 1.0) * %8);\n")
                               .arg(trigStr, pitchLogic, accLogic, squLogic).arg(macro).arg(driveStrNightly, decayStr).arg(squDecayBase);

        QString gateLogic = toggleHardGate->isChecked() ? " * trig" : "";

        QString synthO1 = QString("(%1(core * (drive * (1.0 + squAmt * squEnv))) + randsv(t*srate,0)*(squAmt*0.005)) * exp(-mod(t*(tempo/15), 1.0) * decay)%2").arg(foldFunc, gateLogic);
        QString synthO2 = QString("(%1(core * (drive * (1.0 + squAmt * squEnv))) + randsv(t*srate*1.1,0)*(squAmt*0.005)) * exp(-mod(t*(tempo/15), 1.0) * decay)%2").arg(foldFunc, gateLogic);

        if(toggleNSCLathe->isChecked()) {
            synthO1 = QString("(sinew(integrate(f*pitch))*0.5 + %1(core * (drive * (1.0 + squAmt * squEnv))) + randsv(t*srate,0)*(squAmt*0.005)) * exp(-mod(t*(tempo/15), 1.0) * decay)%2").arg(foldFunc, gateLogic);
            synthO2 = QString("(sinew(integrate(f*pitch))*0.5 + %1(core * (drive * (1.0 + squAmt * squEnv))) + randsv(t*srate*1.1,0)*(squAmt*0.005)) * exp(-mod(t*(tempo/15), 1.0) * decay)%2").arg(foldFunc, gateLogic);
        }

        QString echoO1 = "0", echoO2 = "0";
        if (toggleEcho->isChecked()) {
            QRegularExpression re("\\bt\\b");
            QString tE = "max(0, t - (45/tempo))";

            QString trigE = trigStr;   trigE.replace(re, tE);
            QString pitchE = pitchLogic; pitchE.replace(re, tE);
            QString accE = accLogic;    accE.replace(re, tE);

            QString evolveMathO1E = evolveMathO1; evolveMathO1E.replace(re, tE);

            echoO1 = QString("\n// Echo Engine\nvar trigE = %1;\nvar pitchE = %2;\nvar accE = %3;\nvar coreE = saww(integrate(f*pitchE))%4;\n"
                             "var echo = %5(coreE * (%6 * accE)) * exp(-mod(%7*(tempo/15), 1.0) * decay) * trigE * %8;\n")
                         .arg(trigE, pitchE, accE, evolveMathO1E, foldFunc, driveStrNightly.remove(" * acc"), tE).arg(echoAmt);

            echoO2 = echoO1;
        }

        finalO1 = varSetup + QString("var core = %1;\nvar main = %2;").arg(coreO1Nightly, synthO1) +
                  (toggleEcho->isChecked() ? echoO1 : "") +
                  QString("\nclamp(-1.0, main + %1, 1.0);").arg(toggleEcho->isChecked() ? "echo" : "0");

        finalO2 = varSetup + QString("var core = %1;\nvar main = %2;").arg(coreO2Nightly, synthO2) +
                  (toggleEcho->isChecked() ? echoO2 : "") +
                  QString("\nclamp(-1.0, main + %1, 1.0);").arg(toggleEcho->isChecked() ? "echo" : "0");
    }

    codeOutputO1->setText(finalO1);
    codeOutputO2->setText(finalO2);
    isUpdatingUI = false;
}

void XpressiveSeqTab::onTextChanged() { }
