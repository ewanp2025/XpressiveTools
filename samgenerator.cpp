#include "samgenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRegularExpression>
#include <QApplication>
#include <QClipboard>

SamGeneratorTab::SamGeneratorTab(QWidget *parent) : QWidget(parent) {
    initSamLibrary();
    initEnglishDictionary();
    setupUI();

    connect(translateBtn, &QPushButton::clicked, this, &SamGeneratorTab::translateEnglishToPhonemes);
    connect(generateBtn, &QPushButton::clicked, this, &SamGeneratorTab::generateXpressiveMath);

    connect(mouthSlider, &QSlider::valueChanged, this, &SamGeneratorTab::updateLabels);
    connect(throatSlider, &QSlider::valueChanged, this, &SamGeneratorTab::updateLabels);
    connect(pitchSlider, &QSlider::valueChanged, this, &SamGeneratorTab::updateLabels);
    connect(speedSlider, &QSlider::valueChanged, this, &SamGeneratorTab::updateLabels);
    connect(presetCombo, &QComboBox::currentIndexChanged, this, &SamGeneratorTab::applyPreset);

    updateLabels(); // Set initial labels
}

void SamGeneratorTab::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);


    QGroupBox *translateGroup = new QGroupBox("Native English to Phoneme Translator", this);
    QVBoxLayout *translateLayout = new QVBoxLayout(translateGroup);

    QHBoxLayout *englishLayout = new QHBoxLayout();
    englishLayout->addWidget(new QLabel("Type English:"));
    englishInput = new QLineEdit(this);
    englishInput->setPlaceholderText("e.g., HELLO COMPUTER");
    translateBtn = new QPushButton("Translate", this);
    englishLayout->addWidget(englishInput);
    englishLayout->addWidget(translateBtn);
    translateLayout->addLayout(englishLayout);
    mainLayout->addWidget(translateGroup);


    QGroupBox *inputGroup = new QGroupBox("Phonetic Editor & Voice Control", this);
    QVBoxLayout *inputLayout = new QVBoxLayout(inputGroup);

    phonemeInput = new QTextEdit(this);
    phonemeInput->setPlaceholderText("Phonemes appear here...");
    phonemeInput->setMaximumHeight(60);
    inputLayout->addWidget(phonemeInput);


    QHBoxLayout *presetLayout = new QHBoxLayout();
    presetLayout->addWidget(new QLabel("Character Preset:"));
    presetCombo = new QComboBox(this);
    presetCombo->addItems({"Default SAM", "Elf (High/Bright)", "Giant (Low/Dark)", "Little Robot", "Extra-Terrestrial"});
    presetLayout->addWidget(presetCombo);
    inputLayout->addLayout(presetLayout);


    QGridLayout *slidersLayout = new QGridLayout();

    mouthLabel = new QLabel("Mouth (F1): 128", this);
    mouthSlider = new QSlider(Qt::Horizontal, this);
    mouthSlider->setRange(50, 200); mouthSlider->setValue(128);

    throatLabel = new QLabel("Throat (F2): 128", this);
    throatSlider = new QSlider(Qt::Horizontal, this);
    throatSlider->setRange(50, 200); throatSlider->setValue(128);

    pitchLabel = new QLabel("Pitch: 100%", this);
    pitchSlider = new QSlider(Qt::Horizontal, this);
    pitchSlider->setRange(50, 200); pitchSlider->setValue(100);

    speedLabel = new QLabel("Speed: 100%", this);
    speedSlider = new QSlider(Qt::Horizontal, this);
    speedSlider->setRange(50, 200); speedSlider->setValue(100);

    slidersLayout->addWidget(mouthLabel, 0, 0); slidersLayout->addWidget(mouthSlider, 0, 1);
    slidersLayout->addWidget(throatLabel, 0, 2); slidersLayout->addWidget(throatSlider, 0, 3);
    slidersLayout->addWidget(pitchLabel, 1, 0); slidersLayout->addWidget(pitchSlider, 1, 1);
    slidersLayout->addWidget(speedLabel, 1, 2); slidersLayout->addWidget(speedSlider, 1, 3);
    inputLayout->addLayout(slidersLayout);


    QHBoxLayout *settingsLayout = new QHBoxLayout();
    settingsLayout->addWidget(new QLabel("Render Mode:"));
    parserModeCombo = new QComboBox(this);
    parserModeCombo->addItems({"High Quality (Filtered & Smooth)", "Retro (8-bit Grit)"});
    settingsLayout->addWidget(parserModeCombo);

    nightlyCheckBox = new QCheckBox("Nightly Build (ExprTk Radians)", this);
    nightlyCheckBox->setChecked(true);
    settingsLayout->addWidget(nightlyCheckBox);

    inputLayout->addLayout(settingsLayout);

    generateBtn = new QPushButton("Generate Advanced Math Strings", this);
    generateBtn->setStyleSheet("font-weight: bold; height: 35px;");
    inputLayout->addWidget(generateBtn);
    mainLayout->addWidget(inputGroup);


    QGroupBox *outputGroup = new QGroupBox("Xpressive Output", this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);

    outputLayout->addWidget(new QLabel("W1 Field (Master Glottal Pitch Oscillator):"));
    w1Output = new QTextEdit(this);
    w1Output->setMaximumHeight(45);
    w1Output->setReadOnly(true);
    outputLayout->addWidget(w1Output);

    outputLayout->addWidget(new QLabel("O1 Field (FOF Synthesis, AM Noise, & Master Gate):"));
    o1Output = new QTextEdit(this);
    o1Output->setReadOnly(true);
    outputLayout->addWidget(o1Output);

    mainLayout->addWidget(outputGroup);
}

void SamGeneratorTab::updateLabels() {
    mouthLabel->setText(QString("Mouth (F1): %1").arg(mouthSlider->value()));
    throatLabel->setText(QString("Throat (F2): %1").arg(throatSlider->value()));
    pitchLabel->setText(QString("Pitch: %1%").arg(pitchSlider->value()));
    speedLabel->setText(QString("Speed: %1%").arg(speedSlider->value()));
}

void SamGeneratorTab::applyPreset(int index) {
    switch(index) {
    case 0: // Default
        mouthSlider->setValue(128); throatSlider->setValue(128);
        pitchSlider->setValue(100); speedSlider->setValue(100); break;
    case 1: // Elf
        mouthSlider->setValue(160); throatSlider->setValue(110);
        pitchSlider->setValue(150); speedSlider->setValue(120); break;
    case 2: // Giant
        mouthSlider->setValue(100); throatSlider->setValue(160);
        pitchSlider->setValue(60); speedSlider->setValue(80); break;
    case 3: // Little Robot
        mouthSlider->setValue(128); throatSlider->setValue(128);
        pitchSlider->setValue(120); speedSlider->setValue(100); break;
    case 4: // E.T.
        mouthSlider->setValue(100); throatSlider->setValue(150);
        pitchSlider->setValue(80); speedSlider->setValue(70); break;
    }
}


QList<SamNRLRule> nrlRules;

QString compileNRLRegex(QString context, bool isLeft) {
    if (context.isEmpty() || context == " ") return ".*";
    QString rx = context;
    rx.replace("#", "[AEIOUY]+");
    rx.replace(":", "[B-DF-HJ-NP-TV-Z]*");
    rx.replace("^", "[B-DF-HJ-NP-TV-Z]");
    rx.replace(".", "[BDGJLMNRVWZ]");
    rx.replace("%", "(ER|E|ES|ED|ING|ELY)");
    if (isLeft) return rx + "$";
    return "^" + rx;
}

void SamGeneratorTab::initEnglishDictionary() {
    auto addRule = [&](QString l, QString m, QString r, QString out) {
        nrlRules.append({compileNRLRegex(l, true), m, compileNRLRegex(r, false), out});
    };






    // RULES FOR 'A'
    addRule("", "A", "RE", "EH R*");
    addRule("", "A", "R#", "EH R*");
    addRule("", "A", "R", "AA R*");
    addRule("", "A", "S#", "EY S*");
    addRule("", "A", "WA", "AX W AX");
    addRule("", "A", "^I#", "EY");
    addRule(" ", "A", "^E%", "EY");
    addRule("", "A", "Y", "EY");
    addRule(":", "A", "L", "AO L*");
    addRule("", "A", "", "AE");

    // RULES FOR 'C'
    addRule("", "CH", "", "CH");          // 'CH' -> CH (e.g. Chat)
    addRule("", "C", "E", "S*");          // Soft C before E (e.g. Cent)
    addRule("", "C", "I", "S*");          // Soft C before I (e.g. City)
    addRule("", "C", "Y", "S*");          // Soft C before Y (e.g. Cylinder)
    addRule("", "C", "", "K*");           // Hard C default (e.g. Cat)

    // RULES FOR 'E'
    addRule("", "EE", "", "IY");          // 'EE' -> IY (e.g. See)
    addRule("", "EA", "", "IY");          // 'EA' -> IY (e.g. Meat)
    addRule("", "ER", "", "ER");          // 'ER' -> ER (e.g. Her)
    addRule("", "E", "", "EH");           // Default E (e.g. Bed)

    // RULES FOR 'G'
    addRule("", "G", "E", "J*");          // Soft G before E (e.g. Gem)
    addRule("", "G", "I", "J*");          // Soft G before I (e.g. Giant)
    addRule("", "G", "", "G*");           // Hard G default (e.g. Got)

    // RULES FOR 'I'
    addRule("", "I", "R", "ER");          // 'IR' -> ER (e.g. Bird)
    addRule("", "I", "GH", "AY");         // 'IGH' -> AY (e.g. High)
    addRule("", "I", "", "IH");           // Default I (e.g. Bit)

    // RULES FOR 'O'
    addRule("", "OO", "", "UW");          // 'OO' -> UW (e.g. Boot)
    addRule("", "OR", "", "AO R*");       // 'OR' -> AO R* (e.g. For)
    addRule("", "OU", "", "AW");          // 'OU' -> AW (e.g. Out)
    addRule("", "O", "", "AA");           // Default O (e.g. Cot)

    // RULES FOR 'P'
    addRule("", "PH", "", "F*");          // 'PH' -> F (e.g. Phone)
    addRule("", "P", "", "P*");           // Default P

    // RULES FOR 'S'
    addRule("", "SH", "", "SH");          // 'SH' -> SH (e.g. Ship)
    addRule("", "S", "ION", "ZH");        // 'SION' -> ZH (e.g. Vision)
    addRule("", "S", "", "S*");           // Default S

    // RULES FOR 'T'
    addRule("", "TH", "", "TH");          // 'TH' -> TH (e.g. Think)
    addRule("", "T", "ION", "SH");        // 'TION' -> SH (e.g. Action)
    addRule("", "T", "", "T*");           // Default T

    // RULES FOR 'U'
    addRule("", "UR", "", "ER");          // 'UR' -> ER (e.g. Burn)
    addRule("", "U", "", "AH");           // Default U (e.g. But)

    // RULES FOR 'W'
    addRule("", "WH", "", "W*");          // 'WH' -> W (e.g. When)
    addRule("", "W", "", "W*");           // Default W

    // BASIC CONSONANTS (Catch-alls)
    addRule("", "B", "", "B*");
    addRule("", "D", "", "D*");
    addRule("", "F", "", "F*");
    addRule("", "H", "", "H");
    addRule("", "J", "", "J*");
    addRule("", "K", "", "K*");
    addRule("", "L", "", "L*");
    addRule("", "M", "", "M*");
    addRule("", "N", "", "N*");
    addRule("", "R", "", "R*");
    addRule("", "V", "", "V*");
    addRule("", "X", "", "K* S*");        // 'X' -> KS (e.g. Box)
    addRule("", "Y", "", "Y*");
    addRule("", "Z", "", "Z*");
}

QString SamGeneratorTab::textToPhonemes(const QString& englishText) {
    QString word = englishText.toUpper();
    word.remove(QRegularExpression("[^A-Z]"));

    QString phonemes = "";
    int pos = 0;

    while(pos < word.length()) {
        bool matched = false;

        for(const SamNRLRule& rule : nrlRules) {
            if(word.mid(pos).startsWith(rule.match)){
                QString leftStr = word.left(pos);
                QString rightStr = word.mid(pos + rule.match.length());

                if(QRegularExpression(rule.leftContextRegex).match(leftStr).hasMatch() &&
                    QRegularExpression(rule.rightContextRegex).match(rightStr).hasMatch()) {

                    phonemes += rule.phonemes + " ";
                    pos += rule.match.length();
                    matched = true;
                    break;
                }
            }
        }
        if(!matched) {

            QString letter = word.mid(pos, 1);
            if(letter == "E") phonemes += "IY ";
            else if(letter == "I") phonemes += "AY ";
            else if(letter == "O") phonemes += "OW ";
            else if(letter == "U") phonemes += "UW ";
            else phonemes += letter + "* ";
            pos++;
        }
    }
    return phonemes.simplified();
}

void SamGeneratorTab::translateEnglishToPhonemes() {
    phonemeInput->setText(textToPhonemes(englishInput->text()));
}

void SamGeneratorTab::generateXpressiveMath() {
    QStringList tokens = phonemeInput->toPlainText().split(" ", Qt::SkipEmptyParts);
    if(tokens.isEmpty()) return;

    bool nightly = nightlyCheckBox->isChecked();
    bool lofi = (parserModeCombo->currentIndex() == 1);

    QString result = parseAndGenerateString(tokens, nightly, lofi);
    o1Output->setText(result);
    QApplication::clipboard()->setText(result);

    double pitchMult = pitchSlider->value() / 100.0;
    if (nightly) w1Output->setText(QString("(0.5 + 0.5 * cos(6.28318 * t * %1))").arg(pitchMult));
    else w1Output->setText(QString("(0.5 + 0.5 * sinew((t * %1) + 0.25))").arg(pitchMult));
}

QString SamGeneratorTab::parseAndGenerateString(const QStringList& inputTokens, bool nightly, bool lofi) {
    double speedFactor = speedSlider->value() / 100.0;
    double frameTime = 0.012 / speedFactor;
    double hzScale = 19.5;

    double mouthFactor = mouthSlider->value() / 128.0;
    double throatFactor = throatSlider->value() / 128.0;

    QList<SequenceNode> nodes;

    for(const QString& rawToken : inputTokens) {
        QRegularExpression re("([A-Z\\*\\/\\.\\?\\,\\-]+)(\\d*)");
        QRegularExpressionMatch match = re.match(rawToken.toUpper());
        if(!samLibrary.contains(match.captured(1))) continue;

        SequenceNode node;
        node.data = samLibrary[match.captured(1)];
        node.stress = match.captured(2).isEmpty() ? 4 : match.captured(2).toInt();
        node.duration = (node.data.length * frameTime) * (0.8 + (node.stress * 0.05));
        nodes.append(node);
    }


    for (int i = 0; i < nodes.size() - 1; ++i) {
        PhonemeClass current = nodes[i].data.pClass;
        PhonemeClass next = nodes[i+1].data.pClass;

        if (current == CLASS_VOWEL && next == CLASS_VOICED_CONS) nodes[i].duration *= 1.25;
        else if (current == CLASS_STOP_CONS && next == CLASS_STOP_CONS) {
            nodes[i].duration = (nodes[i].duration * 0.5) + frameTime;
            nodes[i+1].duration = (nodes[i+1].duration * 0.5) + frameTime;
        }
        else if (current == CLASS_VOWEL && next == CLASS_PUNCT) nodes[i].duration *= 1.5;
    }

    QString o1Matrix = "";
    double currentTime = 0.0;
    QString piMult = nightly ? "6.28318 * " : "";
    QString sinFunc = nightly ? "sin" : "sinew";


    for(int i = 0; i < nodes.size(); ++i) {
        const SAMPhoneme& p = nodes[i].data;
        QString phonemeMath;

        if (p.voiced && p.pClass != CLASS_UNVOICED_CONS && p.pClass != CLASS_STOP_CONS && p.pClass != CLASS_PUNCT) {

            double f1Start = (p.f1 * hzScale) * mouthFactor;
            double f2Start = (p.f2 * hzScale) * throatFactor;
            double f3Start = p.f3 * hzScale;

            double f1End = f1Start, f2End = f2Start, f3End = f3Start;

            if (i < nodes.size() - 1 && nodes[i+1].data.voiced && nodes[i+1].data.pClass != CLASS_STOP_CONS) {
                f1End = (nodes[i+1].data.f1 * hzScale) * mouthFactor;
                f2End = (nodes[i+1].data.f2 * hzScale) * throatFactor;
                f3End = nodes[i+1].data.f3 * hzScale;
            }

            double a1Lin = p.a1 / 15.0; double a2Lin = p.a2 / 15.0; double a3Lin = p.a3 / 15.0;

            QString f1Dyn = QString("(%1 + (%2 - %1) * clamp(0.0, (t - %3) / %4, 1.0))").arg(f1Start).arg(f1End).arg(currentTime).arg(nodes[i].duration);
            QString f2Dyn = QString("(%1 + (%2 - %1) * clamp(0.0, (t - %3) / %4, 1.0))").arg(f2Start).arg(f2End).arg(currentTime).arg(nodes[i].duration);
            QString f3Dyn = QString("(%1 + (%2 - %1) * clamp(0.0, (t - %3) / %4, 1.0))").arg(f3Start).arg(f3End).arg(currentTime).arg(nodes[i].duration);


            QString glottalTime = nightly ? "(mod(integrate(f), 1.0) / f)" : "(mod(integrate(f), 1.0) / f)";
            QString glottalDecay = QString("exp(-%1 * 50.0)").arg(glottalTime);

            QString f1Str = QString("%1 * %2(%3%4 * %5)").arg(a1Lin).arg(sinFunc).arg(piMult).arg(f1Dyn).arg(glottalTime);
            QString f2Str = QString("%1 * %2(%3%4 * %5)").arg(a2Lin).arg(sinFunc).arg(piMult).arg(f2Dyn).arg(glottalTime);
            QString f3Str = QString("%1 * %2(%3%4 * %5)").arg(a3Lin).arg(sinFunc).arg(piMult).arg(f3Dyn).arg(glottalTime);

            phonemeMath = QString("((%1 + %2 + %3) * %4)").arg(f1Str, f2Str, f3Str, glottalDecay);

        } else if (p.pClass == CLASS_UNVOICED_CONS) {
            double noiseBase = (p.name == "SH" || p.name == "CH") ? 1500.0 : 2000.0;
            double carrierHz = (p.name == "SH" || p.name == "CH") ? 3500.0 : 5500.0;
            phonemeMath = QString("(randv(t * %1) * %2(%3integrate(%4)))").arg(noiseBase).arg(sinFunc).arg(piMult).arg(carrierHz);

        } else if (p.pClass == CLASS_STOP_CONS) {
            double noiseBase = 1000.0, carrierHz = 1500.0;
            if (p.name == "P*" || p.name == "B*") { noiseBase = 800.0; carrierHz = 1000.0; }
            else if (p.name == "T*" || p.name == "D*") { noiseBase = 3000.0; carrierHz = 4000.0; }
            else if (p.name == "K*" || p.name == "G*" || p.name == "KX" || p.name == "GX") { noiseBase = 1500.0; carrierHz = 2000.0; }

            double burstTime = currentTime + (nodes[i].duration * 0.4);
            QString decayStr = QString("exp(-(t - %1) * 150.0) * (t >= %1)").arg(burstTime);
            phonemeMath = QString("(randv(t * %1) * %2(%3integrate(%4)) * %5)").arg(noiseBase).arg(sinFunc).arg(piMult).arg(carrierHz).arg(decayStr);

            if (p.voiced) {
                QString voiceBar = QString("(0.3 * W1(integrate(f)) * exp(-(t - %1) * 20.0))").arg(currentTime);
                phonemeMath = QString("(%1 + %2)").arg(phonemeMath, voiceBar);
            }
        } else {
            phonemeMath = "0.0";
        }

        double endTime = currentTime + nodes[i].duration;
        QString timeGate = (i == nodes.size() - 1) ? QString("(t >= %1)").arg(currentTime) : QString("(t >= %1 & t < %2)").arg(currentTime).arg(endTime);

        if (!o1Matrix.isEmpty()) o1Matrix += " + \n    ";
        o1Matrix += QString("%1 * %2").arg(timeGate, phonemeMath);

        currentTime = endTime;
    }

    QString masterEnvelope = QString("exp(-max(0.0, t - %1) * 20.0)").arg(currentTime);
    QString finalFormula = QString("clamp(-1.0, \n    (%1) * %2\n, 1.0)").arg(o1Matrix, masterEnvelope);

    if (lofi) finalFormula = QString("clamp(-1.0, floor((%1) * 16)/16, 1.0)").arg(finalFormula);
    else finalFormula = QString("clamp(-1.0, ((%1) * 0.4) + (last(1) * 0.6), 1.0)").arg(finalFormula);

    return finalFormula;
}

void SamGeneratorTab::initSamLibrary() {
    // Vowels
    samLibrary["IY"] = {"IY", CLASS_VOWEL, 10, 84, 110, true, 15, 10, 5, 18};
    samLibrary["IH"] = {"IH", CLASS_VOWEL, 14, 73, 93,  true, 15, 10, 5, 15};
    samLibrary["EH"] = {"EH", CLASS_VOWEL, 19, 67, 91,  true, 15, 10, 5, 16};
    samLibrary["AE"] = {"AE", CLASS_VOWEL, 24, 63, 88,  true, 15, 10, 5, 18};
    samLibrary["AA"] = {"AA", CLASS_VOWEL, 27, 40, 89,  true, 15, 10, 5, 18};
    samLibrary["AH"] = {"AH", CLASS_VOWEL, 23, 44, 87,  true, 15, 10, 5, 16};
    samLibrary["AO"] = {"AO", CLASS_VOWEL, 21, 31, 88,  true, 15, 10, 5, 18};
    samLibrary["UH"] = {"UH", CLASS_VOWEL, 16, 37, 82,  true, 15, 10, 5, 15};
    samLibrary["AX"] = {"AX", CLASS_VOWEL, 20, 45, 89,  true, 15, 10, 5, 12};
    samLibrary["IX"] = {"IX", CLASS_VOWEL, 14, 73, 93,  true, 15, 10, 5, 12};
    samLibrary["ER"] = {"ER", CLASS_VOWEL, 18, 49, 62,  true, 15, 10, 5, 18};
    samLibrary["UX"] = {"UX", CLASS_VOWEL, 14, 36, 82,  true, 15, 10, 5, 15};
    samLibrary["OH"] = {"OH", CLASS_VOWEL, 18, 30, 88,  true, 15, 10, 5, 18};
    samLibrary["EY"] = {"EY", CLASS_VOWEL, 19, 72, 90,  true, 15, 10, 5, 20};
    samLibrary["AY"] = {"AY", CLASS_VOWEL, 27, 39, 88,  true, 15, 10, 5, 22};
    samLibrary["OY"] = {"OY", CLASS_VOWEL, 21, 31, 88,  true, 15, 10, 5, 22};
    samLibrary["AW"] = {"AW", CLASS_VOWEL, 27, 43, 88,  true, 15, 10, 5, 22};
    samLibrary["OW"] = {"OW", CLASS_VOWEL, 18, 30, 88,  true, 15, 10, 5, 20};
    samLibrary["UW"] = {"UW", CLASS_VOWEL, 13, 34, 82,  true, 15, 10, 5, 18};

    // Voiced Consonants
    samLibrary["M*"] = {"M*", CLASS_VOICED_CONS, 6,  46, 81,  true, 12, 8, 4, 15};
    samLibrary["N*"] = {"N*", CLASS_VOICED_CONS, 6,  54, 121, true, 12, 8, 4, 15};
    samLibrary["NX"] = {"NX", CLASS_VOICED_CONS, 6,  86, 101, true, 12, 8, 4, 15};
    samLibrary["R*"] = {"R*", CLASS_VOICED_CONS, 18, 50, 60,  true, 12, 8, 4, 14};
    samLibrary["L*"] = {"L*", CLASS_VOICED_CONS, 14, 30, 110, true, 12, 8, 4, 14};
    samLibrary["W*"] = {"W*", CLASS_VOICED_CONS, 11, 24, 90,  true, 12, 8, 4, 12};
    samLibrary["Y*"] = {"Y*", CLASS_VOICED_CONS, 9,  83, 110, true, 12, 8, 4, 12};
    samLibrary["Z*"] = {"Z*", CLASS_VOICED_CONS, 9,  51, 93,  true, 10, 6, 3, 10};
    samLibrary["ZH"] = {"ZH", CLASS_VOICED_CONS, 10, 66, 103, true, 10, 6, 3, 10};
    samLibrary["V*"] = {"V*", CLASS_VOICED_CONS, 8,  40, 76,  true, 10, 6, 3, 8};
    samLibrary["DH"] = {"DH", CLASS_VOICED_CONS, 10, 47, 93,  true, 10, 6, 3, 8};
    samLibrary["J*"] = {"J*", CLASS_VOICED_CONS, 6,  66, 121, true, 10, 6, 3, 8};

    // Unvoiced Consonants / Fricatives
    samLibrary["S*"] = {"S*", CLASS_UNVOICED_CONS, 6,  73, 99,  false, 8, 0, 0, 12};
    samLibrary["SH"] = {"SH", CLASS_UNVOICED_CONS, 6,  79, 106, false, 8, 0, 0, 12};
    samLibrary["F*"] = {"F*", CLASS_UNVOICED_CONS, 6,  26, 81,  false, 8, 0, 0, 10};
    samLibrary["TH"] = {"TH", CLASS_UNVOICED_CONS, 6,  66, 121, false, 8, 0, 0, 10};
    samLibrary["/H"] = {"/H", CLASS_UNVOICED_CONS, 14, 73, 93,  false, 8, 0, 0, 10};
    samLibrary["CH"] = {"CH", CLASS_UNVOICED_CONS, 6,  79, 101, false, 8, 0, 0, 10};

    // Stop Consonants
    samLibrary["P*"] = {"P*", CLASS_STOP_CONS, 6,  26, 81,  false, 10, 0, 0, 5};
    samLibrary["T*"] = {"T*", CLASS_STOP_CONS, 6,  66, 121, false, 10, 0, 0, 5};
    samLibrary["K*"] = {"K*", CLASS_STOP_CONS, 6,  85, 101, false, 10, 0, 0, 6};
    samLibrary["KX"] = {"KX", CLASS_STOP_CONS, 6,  84, 94,  false, 10, 0, 0, 6};
    samLibrary["B*"] = {"B*", CLASS_STOP_CONS, 6,  26, 81,  true, 10, 6, 3, 6};
    samLibrary["D*"] = {"D*", CLASS_STOP_CONS, 6,  66, 121, true, 10, 6, 3, 6};
    samLibrary["G*"] = {"G*", CLASS_STOP_CONS, 6,  110, 112, true, 10, 6, 3, 6};
    samLibrary["GX"] = {"GX", CLASS_STOP_CONS, 6,  84, 94,  true, 10, 6, 3, 6};

    // Punctuation & Silence
    samLibrary[" *"] = {" *", CLASS_PUNCT, 0, 0, 0, false, 0, 0, 0, 5};
    samLibrary[".*"] = {".*", CLASS_PUNCT, 19, 67, 91, false, 0, 0, 0, 10};
    for(int b=43; b<=77; ++b) {
        QString key = QString("**%1").arg(b);
        if(!samLibrary.contains(key)) samLibrary[key] = {"**", CLASS_UNKNOWN, 6, 60, 100, true, 10, 5, 2, 8};
    }
}
