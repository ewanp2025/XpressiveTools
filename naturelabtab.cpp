#include "naturelabtab.h"
#include <QString>
#include <cmath>

NatureLabTab::NatureLabTab(QWidget *parent) : QWidget(parent) {
    setupUI();


    avianPresetCombo->setCurrentIndex(1); // Load Magpie by default
    mammalPresetCombo->setCurrentIndex(1); // Load Dingo by default
    geoPresetCombo->setCurrentIndex(1); // Load Banjo Frog by default

    generateExpression();
}

void NatureLabTab::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- TOP BAR: Category & Build Mode ---
    QGroupBox *modeBox = new QGroupBox("Acoustic Environment Settings");
    QHBoxLayout *modeLayout = new QHBoxLayout(modeBox);

    categorySelector = new QComboBox();
    categorySelector->addItems({
        "Avian (Syrinx FM - e.g. Magpie, Whipbird)",
        "Mammalian (Larynx - e.g. Dingo)",
        "Geophony & Environment (Weather, Frogs)"
    });

    buildModeSelector = new QComboBox();
    buildModeSelector->addItems({"Nightly (Variables f, t)", "Legacy (Additive)"});

    modeLayout->addWidget(new QLabel("Category:"));
    modeLayout->addWidget(categorySelector, 1);
    modeLayout->addWidget(new QLabel("Build Mode:"));
    modeLayout->addWidget(buildModeSelector, 1);
    mainLayout->addWidget(modeBox);

    stackedWidget = new QStackedWidget();


    avianWidget = new QWidget();
    QFormLayout *avianLayout = new QFormLayout(avianWidget);

    avianPresetCombo = new QComboBox();
    avianPresetCombo->addItems({"Custom Generic", "Magpie", "Kookaburra", "Whipbird", "Emu"});
    avianLayout->addRow("Preset:", avianPresetCombo);

    avianBaseFreqSlider = new QSlider(Qt::Horizontal); avianBaseFreqSlider->setRange(10, 2000);
    avianSweepRateSlider = new QSlider(Qt::Horizontal); avianSweepRateSlider->setRange(1, 500);
    avianFmDepthSlider = new QSlider(Qt::Horizontal); avianFmDepthSlider->setRange(0, 100);
    avianDetuneSlider = new QSlider(Qt::Horizontal); avianDetuneSlider->setRange(0, 200);
    avianDecaySlider = new QSlider(Qt::Horizontal); avianDecaySlider->setRange(1, 200);
    avianModFreqSlider = new QSlider(Qt::Horizontal); avianModFreqSlider->setRange(1, 200);

    avianLayout->addRow("Base Freq:", avianBaseFreqLabel = new QLabel());
    avianLayout->addRow(avianBaseFreqSlider);
    avianLayout->addRow("FM Depth (A1):", avianFmDepthLabel = new QLabel());
    avianLayout->addRow(avianFmDepthSlider);
    avianLayout->addRow("Sweep Rate:", avianSweepRateLabel = new QLabel());
    avianLayout->addRow(avianSweepRateSlider);
    avianLayout->addRow("Mod Frequency:", avianModFreqLabel = new QLabel());
    avianLayout->addRow(avianModFreqSlider);
    avianLayout->addRow("Detune Offset:", avianDetuneLabel = new QLabel());
    avianLayout->addRow(avianDetuneSlider);
    avianLayout->addRow("Envelope Decay:", avianDecayLabel = new QLabel());
    avianLayout->addRow(avianDecaySlider);


    mammalWidget = new QWidget();
    QFormLayout *mammalLayout = new QFormLayout(mammalWidget);

    mammalPresetCombo = new QComboBox();
    mammalPresetCombo->addItems({"Custom Generic", "Dingo"});
    mammalLayout->addRow("Preset:", mammalPresetCombo);

    mammalFundFreqSlider = new QSlider(Qt::Horizontal); mammalFundFreqSlider->setRange(10, 1000);
    mammalHnrSlider = new QSlider(Qt::Horizontal); mammalHnrSlider->setRange(0, 100);
    mammalSubharmonicSlider = new QSlider(Qt::Horizontal); mammalSubharmonicSlider->setRange(0, 100);
    mammalFormantSlider = new QSlider(Qt::Horizontal); mammalFormantSlider->setRange(10, 100);
    mammalVibratoRateSlider = new QSlider(Qt::Horizontal); mammalVibratoRateSlider->setRange(1, 100);
    mammalDecaySlider = new QSlider(Qt::Horizontal); mammalDecaySlider->setRange(1, 100);

    mammalLayout->addRow("Fund. Freq:", mammalFundFreqLabel = new QLabel());
    mammalLayout->addRow(mammalFundFreqSlider);
    mammalLayout->addRow("Subharmonic Mod:", mammalSubharmonicLabel = new QLabel());
    mammalLayout->addRow(mammalSubharmonicSlider);
    mammalLayout->addRow("Vibrato Rate:", mammalVibratoRateLabel = new QLabel());
    mammalLayout->addRow(mammalVibratoRateSlider);
    mammalLayout->addRow("Formant Multiplier:", mammalFormantLabel = new QLabel());
    mammalLayout->addRow(mammalFormantSlider);
    mammalLayout->addRow("Noise HNR:", mammalHnrLabel = new QLabel());
    mammalLayout->addRow(mammalHnrSlider);
    mammalLayout->addRow("Envelope Decay:", mammalDecayLabel = new QLabel());
    mammalLayout->addRow(mammalDecaySlider);


    geoWidget = new QWidget();
    QFormLayout *geoLayout = new QFormLayout(geoWidget);

    geoPresetCombo = new QComboBox();
    geoPresetCombo->addItems({"Custom Generic", "Banjo Frog", "Rain", "Thunder", "Didgeridoo"});
    geoLayout->addRow("Preset:", geoPresetCombo);

    geoBaseFreqSlider = new QSlider(Qt::Horizontal); geoBaseFreqSlider->setRange(10, 1000);
    geoTurbulenceSlider = new QSlider(Qt::Horizontal); geoTurbulenceSlider->setRange(0, 100);
    geoIntensitySlider = new QSlider(Qt::Horizontal); geoIntensitySlider->setRange(0, 100);
    geoSweepSlider = new QSlider(Qt::Horizontal); geoSweepSlider->setRange(0, 500);
    geoDecaySlider = new QSlider(Qt::Horizontal); geoDecaySlider->setRange(1, 200);

    geoLayout->addRow("Base Freq:", geoBaseFreqLabel = new QLabel());
    geoLayout->addRow(geoBaseFreqSlider);
    geoLayout->addRow("Intensity (A1):", geoIntensityLabel = new QLabel());
    geoLayout->addRow(geoIntensitySlider);
    geoLayout->addRow("Turbulence/Noise:", geoTurbulenceLabel = new QLabel());
    geoLayout->addRow(geoTurbulenceSlider);
    geoLayout->addRow("Sweep Rate:", geoSweepLabel = new QLabel());
    geoLayout->addRow(geoSweepSlider);
    geoLayout->addRow("Envelope Decay:", geoDecayLabel = new QLabel());
    geoLayout->addRow(geoDecaySlider);

    stackedWidget->addWidget(avianWidget);
    stackedWidget->addWidget(mammalWidget);
    stackedWidget->addWidget(geoWidget);
    mainLayout->addWidget(stackedWidget, 2);


    QGroupBox *outputBox = new QGroupBox("Xpressive Formula Output");
    QVBoxLayout *outLayout = new QVBoxLayout(outputBox);
    outputExpressionBox = new QTextEdit();
    outputExpressionBox->setReadOnly(true);
    outputExpressionBox->setMaximumHeight(80);
    outLayout->addWidget(outputExpressionBox);
    btnPlayNature = new QPushButton("Play Environment");
    btnPlayNature->setCheckable(true);
    outLayout->addWidget(btnPlayNature);
    mainLayout->addWidget(outputBox);


    connect(categorySelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NatureLabTab::changeCategory);
    connect(btnPlayNature, &QPushButton::toggled, this, &NatureLabTab::togglePlay);

    connect(avianPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NatureLabTab::loadAvianPreset);
    connect(mammalPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NatureLabTab::loadMammalPreset);
    connect(geoPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NatureLabTab::loadGeoPreset);


    const QList<QSlider*> allSliders = {
        avianBaseFreqSlider, avianSweepRateSlider, avianFmDepthSlider, avianDetuneSlider, avianDecaySlider, avianModFreqSlider,
        mammalFundFreqSlider, mammalHnrSlider, mammalSubharmonicSlider, mammalFormantSlider, mammalVibratoRateSlider, mammalDecaySlider,
        geoTurbulenceSlider, geoIntensitySlider, geoBaseFreqSlider, geoSweepSlider, geoDecaySlider
    };
    for(QSlider* slider : allSliders) {
        connect(slider, &QSlider::valueChanged, this, &NatureLabTab::updateLabels);
    }
}

void NatureLabTab::loadAvianPreset(int index) {
    avianBaseFreqSlider->blockSignals(true);
    avianFmDepthSlider->blockSignals(true);
    avianSweepRateSlider->blockSignals(true);
    avianDecaySlider->blockSignals(true);
    avianModFreqSlider->blockSignals(true);

    if (index == 1) { // Magpie
        avianBaseFreqSlider->setValue(440);
        avianFmDepthSlider->setValue(26);  // A1: 0.26
        avianSweepRateSlider->setValue(30);
        avianModFreqSlider->setValue(70);  // 7.0
        avianDecaySlider->setValue(12);    // 1.2
    } else if (index == 2) { // Kookaburra
        avianBaseFreqSlider->setValue(440);
        avianFmDepthSlider->setValue(30);  // A1: 0.3
        avianSweepRateSlider->setValue(30);
        avianDecaySlider->setValue(20);    // 2.0
    } else if (index == 3) { // Whipbird
        avianBaseFreqSlider->setValue(440);
        avianFmDepthSlider->setValue(5);   // A1: 0.05
        avianSweepRateSlider->setValue(12);
        avianDecaySlider->setValue(6);     // 0.6
    } else if (index == 4) { // Emu
        avianBaseFreqSlider->setValue(440);
        avianFmDepthSlider->setValue(15);  // A1: 0.15
        avianSweepRateSlider->setValue(200); // 20.0
        avianDecaySlider->setValue(30);    // 3.0
    }

    avianBaseFreqSlider->blockSignals(false);
    avianFmDepthSlider->blockSignals(false);
    avianSweepRateSlider->blockSignals(false);
    avianDecaySlider->blockSignals(false);
    avianModFreqSlider->blockSignals(false);
    updateLabels();
}

void NatureLabTab::loadMammalPreset(int index) {
    mammalFundFreqSlider->blockSignals(true);
    mammalSubharmonicSlider->blockSignals(true);
    mammalVibratoRateSlider->blockSignals(true);
    mammalDecaySlider->blockSignals(true);
    mammalHnrSlider->blockSignals(true);

    if (index == 1) { // Dingo
        mammalFundFreqSlider->setValue(440);
        mammalSubharmonicSlider->setValue(20); // 0.2
        mammalVibratoRateSlider->setValue(8);  // 0.8
        mammalDecaySlider->setValue(4);        // 0.4
        mammalHnrSlider->setValue(2);          // 0.02
    }

    mammalFundFreqSlider->blockSignals(false);
    mammalSubharmonicSlider->blockSignals(false);
    mammalVibratoRateSlider->blockSignals(false);
    mammalDecaySlider->blockSignals(false);
    mammalHnrSlider->blockSignals(false);
    updateLabels();
}

void NatureLabTab::loadGeoPreset(int index) {
    geoBaseFreqSlider->blockSignals(true);
    geoIntensitySlider->blockSignals(true);
    geoSweepSlider->blockSignals(true);
    geoDecaySlider->blockSignals(true);
    geoTurbulenceSlider->blockSignals(true);

    if (index == 1) { // Banjo Frog
        geoBaseFreqSlider->setValue(440);
        geoIntensitySlider->setValue(26); // 0.26
        geoSweepSlider->setValue(400);    // 40.0
        geoDecaySlider->setValue(60);     // 6.0
    } else if (index == 2) { // Rain
        geoBaseFreqSlider->setValue(440);
        geoTurbulenceSlider->setValue(8); // 0.8
        geoDecaySlider->setValue(150);    // 15.0
    } else if (index == 3) { // Thunder
        geoBaseFreqSlider->setValue(440);
        geoTurbulenceSlider->setValue(6); // 0.6
        geoDecaySlider->setValue(150);    // 15.0
    } else if (index == 4) { // Didgeridoo
        geoBaseFreqSlider->setValue(69);
        geoIntensitySlider->setValue(15); // 0.15
    }

    geoBaseFreqSlider->blockSignals(false);
    geoIntensitySlider->blockSignals(false);
    geoSweepSlider->blockSignals(false);
    geoDecaySlider->blockSignals(false);
    geoTurbulenceSlider->blockSignals(false);
    updateLabels();
}

void NatureLabTab::changeCategory(int index) {
    stackedWidget->setCurrentIndex(index);
    updateLabels();
}

void NatureLabTab::updateLabels() {
    avianBaseFreqLabel->setText(QString::number(avianBaseFreqSlider->value()) + " Hz");
    avianFmDepthLabel->setText(QString::number(avianFmDepthSlider->value() / 100.0, 'f', 2));
    avianSweepRateLabel->setText(QString::number(avianSweepRateSlider->value() / 10.0, 'f', 1));
    avianModFreqLabel->setText(QString::number(avianModFreqSlider->value() / 10.0, 'f', 1));
    avianDetuneLabel->setText(QString::number(avianDetuneSlider->value() / 100.0, 'f', 2));
    avianDecayLabel->setText(QString::number(avianDecaySlider->value() / 10.0, 'f', 1));

    mammalFundFreqLabel->setText(QString::number(mammalFundFreqSlider->value()) + " Hz");
    mammalSubharmonicLabel->setText(QString::number(mammalSubharmonicSlider->value() / 100.0, 'f', 2));
    mammalVibratoRateLabel->setText(QString::number(mammalVibratoRateSlider->value() / 10.0, 'f', 1));
    mammalFormantLabel->setText(QString::number(mammalFormantSlider->value() / 10.0, 'f', 1));
    mammalHnrLabel->setText(QString::number(mammalHnrSlider->value() / 100.0, 'f', 2));
    mammalDecayLabel->setText(QString::number(mammalDecaySlider->value() / 10.0, 'f', 1));

    geoBaseFreqLabel->setText(QString::number(geoBaseFreqSlider->value()) + " Hz");
    geoIntensityLabel->setText(QString::number(geoIntensitySlider->value() / 100.0, 'f', 2));
    geoTurbulenceLabel->setText(QString::number(geoTurbulenceSlider->value() / 10.0, 'f', 1));
    geoSweepLabel->setText(QString::number(geoSweepSlider->value() / 10.0, 'f', 1));
    geoDecayLabel->setText(QString::number(geoDecaySlider->value() / 10.0, 'f', 1));

    generateExpression();
}

void NatureLabTab::generateExpression() {
    int cat = categorySelector->currentIndex();
    QString expr = "";

    if (cat == 0) { // AVIAN
        double freq = avianBaseFreqSlider->value();
        double depth = avianFmDepthSlider->value() / 100.0;
        double sweep = avianSweepRateSlider->value() / 10.0;
        double modF = avianModFreqSlider->value() / 10.0;
        double detune = avianDetuneSlider->value() / 100.0;
        double decay = avianDecaySlider->value() / 10.0;
        int preset = avianPresetCombo->currentIndex();

        if (preset == 1) { // Magpie Exact
            expr = QString("((0.6 * sinew(integrate(%1 * (1.2 + %2 * sinew(t * %3 + 2 * cos(t * %4)))))) + (0.5 * sinew(integrate(%1 * (0.8 + %2 * 0.7 * cos(t * 5 + 1.5 * sinew(t * 2.2))))))) * exp(-t * %5)")
                       .arg(freq).arg(depth).arg(modF).arg(sweep).arg(decay);
        } else if (preset == 2) { // Kookaburra Exact
            expr = QString("sinew(integrate(%1 * %3) + %2 * 3 * randv(t * srate)) * (1 - %2 * 0.6 * randv(t * srate)) * exp(-t * %4)")
                       .arg(freq).arg(depth).arg(sweep).arg(decay);
        } else if (preset == 3) { // Whipbird Exact
            expr = QString("sinew(integrate(%1 * (0.8 + 1.5 * t * exp(-t * %3))) + %2 * 0.5 * randv(t * srate)) * clamp(0, sinew(t * 20 * exp(-t * 0.4)), 1) * exp(-t * %4)")
                       .arg(freq).arg(depth).arg(sweep).arg(decay);
        } else if (preset == 4) { // Emu Exact
            expr = QString("(0.1 * randv(t * srate) * exp(-t * 80)) + (sinew(integrate(%1 * (1 + %2 * 3 * exp(-t * %3)))) * exp(-t * %4))")
                       .arg(freq).arg(depth).arg(sweep).arg(decay);
        } else { // Custom Generic
            expr = QString("(sinew(t * %1 + sinew(t * %2) * %3) + sinew(t * %1 * %4)) * 0.5 * exp(-t * %5)")
                       .arg(freq).arg(sweep).arg(depth).arg(detune).arg(decay);
        }
    }
    else if (cat == 1) { // MAMMALIAN
        double freq = mammalFundFreqSlider->value();
        double subAmt = mammalSubharmonicSlider->value() / 100.0;
        double vib = mammalVibratoRateSlider->value() / 10.0;
        double formant = mammalFormantSlider->value() / 10.0;
        double hnr = mammalHnrSlider->value() / 100.0;
        double decay = mammalDecaySlider->value() / 10.0;
        int preset = mammalPresetCombo->currentIndex();

        if (preset == 1) { // Dingo Exact
            expr = QString("sinew(integrate(%1 * (1 + %2 * sinew(t * %3)))) * (1 - exp(-t * 3)) * exp(-t * %4) * (1 + %5 * randv(t * srate))")
                       .arg(freq).arg(subAmt).arg(vib).arg(decay).arg(hnr);
        } else { // Custom Generic
            QString source = QString("(sinew(t * %1) + %2 * sinew(t * %1 * 0.5))").arg(freq).arg(subAmt);
            expr = QString("((%1 * (1.0 + 0.5 * sinew(t * %2 * %4))) * %3 + randv(t*srate) * %5) * exp(-t * %6)")
                       .arg(source).arg(freq).arg(1.0 - hnr).arg(formant).arg(hnr).arg(decay);
        }
    }
    else if (cat == 2) { // GEOPHONY
        double freq = geoBaseFreqSlider->value();
        double inten = geoIntensitySlider->value() / 100.0;
        double turb = geoTurbulenceSlider->value() / 10.0;
        double sweep = geoSweepSlider->value() / 10.0;
        double decay = geoDecaySlider->value() / 10.0;
        int preset = geoPresetCombo->currentIndex();

        if (preset == 1) { // Banjo Frog Exact
            expr = QString("sinew(integrate(%1 * (1 + %2 * exp(-t * %3)))) * exp(-t * %4)")
                       .arg(freq).arg(inten).arg(sweep).arg(decay);
        } else if (preset == 2) { // Rain Exact
            expr = QString("(randv(t * srate) * exp(-t * %1)) + (randv(t * srate) * sinew(t * %2) * exp(-t * %3))")
                       .arg(decay).arg(freq).arg(turb);
        } else if (preset == 3) { // Thunder Exact
            expr = QString("(sinew(integrate(%1 * (1 + t * %2))) * t * (t < 1.2)) + (randv(t * srate) * (t >= 1.2) * exp(-(t - 1.2) * %3))")
                       .arg(freq).arg(turb).arg(decay);
        } else if (preset == 4) { // Didgeridoo Exact
            expr = QString("sinew(t * %1 + (1.5 + sinew(t * %2 * 8)) * sinew(t * %1)) * (0.8 + 0.2 * sinew(t * 6)) * (0.95 + 0.05 * randv(t * srate))")
                       .arg(freq).arg(inten);
        } else { // Custom Generic
            expr = QString("randv(t*srate) * (sinew(t * %1) * 0.5 + 0.5) * %2 * exp(-t * %3)")
                       .arg(turb).arg(inten).arg(decay);
        }
    }

    expr = QString("clamp(-1, %1, 1)").arg(expr);
    outputExpressionBox->setText(expr);

    if(btnPlayNature->isChecked()) {
        emit playRequested(expr);
    }
}

void NatureLabTab::togglePlay(bool checked) {
    if(checked) {
        btnPlayNature->setText("Stop Environment");
        emit playRequested(outputExpressionBox->toPlainText());
    } else {
        btnPlayNature->setText("Play Environment");
        emit stopRequested();
    }
}
