#include "samgenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRegularExpression>
#include <QApplication>
#include <QClipboard>

// SAM V2 NOT USED


SamGeneratorTab::SamGeneratorTab(QWidget *parent) : QWidget(parent) {
    initSamLibrary();
    initEnglishDictionary();
    setupUI();

    connect(translateBtn, &QPushButton::clicked, this, &SamGeneratorTab::translateEnglishToPhonemes);
    connect(generateBtn, &QPushButton::clicked, this, &SamGeneratorTab::generateXpressiveMath);
    connect(mouthSlider, &QSlider::valueChanged, this, &SamGeneratorTab::updateLabels);
    connect(throatSlider, &QSlider::valueChanged, this, &SamGeneratorTab::updateLabels);
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

    QHBoxLayout *slidersLayout = new QHBoxLayout();
    mouthLabel = new QLabel("Mouth (F1) Scale: 128", this);
    mouthSlider = new QSlider(Qt::Horizontal, this);
    mouthSlider->setRange(50, 200); mouthSlider->setValue(128);

    throatLabel = new QLabel("Throat (F2) Scale: 128", this);
    throatSlider = new QSlider(Qt::Horizontal, this);
    throatSlider->setRange(50, 200); throatSlider->setValue(128);

    slidersLayout->addWidget(mouthLabel); slidersLayout->addWidget(mouthSlider);
    slidersLayout->addWidget(throatLabel); slidersLayout->addWidget(throatSlider);
    inputLayout->addLayout(slidersLayout);

    QHBoxLayout *settingsLayout = new QHBoxLayout();
    settingsLayout->addWidget(new QLabel("Render Mode:"));
    parserModeCombo = new QComboBox(this);
    parserModeCombo->addItems({"High Quality (Smooth)", "Retro (8-bit Grit)"});
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

    outputLayout->addWidget(new QLabel("W1 Field (Master Glottal Oscillator):"));
    w1Output = new QTextEdit(this);
    w1Output->setMaximumHeight(45);
    w1Output->setReadOnly(true);
    outputLayout->addWidget(w1Output);

    outputLayout->addWidget(new QLabel("O1 Field (Formants, AM Noise, and Sequencer):"));
    o1Output = new QTextEdit(this);
    o1Output->setReadOnly(true);
    outputLayout->addWidget(o1Output);

    mainLayout->addWidget(outputGroup);
}

void SamGeneratorTab::updateLabels() {
    mouthLabel->setText(QString("Mouth (F1) Scale: %1").arg(mouthSlider->value()));
    throatLabel->setText(QString("Throat (F2) Scale: %1").arg(throatSlider->value()));
}

void SamGeneratorTab::initEnglishDictionary() {
    englishDictionary["HELLO"] = "H EH L OW";
    englishDictionary["COMPUTER"] = "K AX M P Y UW T ER";
    englishDictionary["SAM"] = "S AE M";
    englishDictionary["IS"] = "IH Z*";
    englishDictionary["HERE"] = "H IY R*";
    englishDictionary["TEST"] = "T EH S* T*";
    englishDictionary["I"] = "AY";
    englishDictionary["AM"] = "AE M*";
    englishDictionary["ROBOT"] = "R* OW B* AA T*";
}

QString SamGeneratorTab::textToPhonemes(const QString& englishText) {
    QString cleanText = englishText.toUpper();
    cleanText.remove(QRegularExpression("[^A-Z\\s]"));
    QStringList words = cleanText.split(" ", Qt::SkipEmptyParts);
    QStringList resultPhonemes;

    for (const QString& word : words) {
        if (englishDictionary.contains(word)) {
            resultPhonemes.append(englishDictionary[word]);
        } else {
            for (QChar c : word) {
                QString letter(c);
                if(letter == "A") resultPhonemes.append("EY");
                else if(letter == "E") resultPhonemes.append("IY");
                else if(letter == "I") resultPhonemes.append("AY");
                else if(letter == "O") resultPhonemes.append("OW");
                else if(letter == "U") resultPhonemes.append("UW");
                else resultPhonemes.append(letter + "*");
            }
        }
        resultPhonemes.append(".*"); // Add a tiny pause between words
    }
    return resultPhonemes.join(" ");
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

    if (nightly) w1Output->setText("(0.5 + 0.5 * cos(6.28318 * t))");
    else w1Output->setText("(0.5 + 0.5 * sinew(t + 0.25))");
}

QString SamGeneratorTab::parseAndGenerateString(const QStringList& inputTokens, bool nightly, bool lofi) {
    double frameTime = 0.012;
    double hzScale = 19.5;

    double mouthFactor = mouthSlider->value() / 128.0;
    double throatFactor = throatSlider->value() / 128.0;

    QList<SequenceNode> nodes;

    for(const QString& rawToken : inputTokens) {
        QRegularExpression re("([A-Z\\*\\/\\.\\?\\,\\-]+)(\\d*)");
        QRegularExpressionMatch match = re.match(rawToken.toUpper());
        QString key = match.captured(1);
        QString stressStr = match.captured(2);

        if(!samLibrary.contains(key)) continue;

        SequenceNode node;
        node.data = samLibrary[key];
        node.stress = stressStr.isEmpty() ? 4 : stressStr.toInt();
        node.duration = (node.data.length * frameTime) * (0.8 + (node.stress * 0.05));
        nodes.append(node);
    }

    for (int i = 0; i < nodes.size() - 1; ++i) {
        PhonemeClass current = nodes[i].data.pClass;
        PhonemeClass next = nodes[i+1].data.pClass;

        if (current == CLASS_VOWEL && next == CLASS_VOICED_CONS) {
            nodes[i].duration *= 1.25;
        }

        else if (current == CLASS_STOP_CONS && next == CLASS_STOP_CONS) {
            nodes[i].duration = (nodes[i].duration * 0.5) + frameTime;
            nodes[i+1].duration = (nodes[i+1].duration * 0.5) + frameTime;
        }
        else if (current == CLASS_VOWEL && next == CLASS_PUNCT) {
            nodes[i].duration *= 1.5;
        }
    }


    QString o1Matrix = "";
    double currentTime = 0.0;
    QString piMult = nightly ? "6.28318 * " : "";
    QString sinFunc = nightly ? "sin" : "sinew";

    for(int i = 0; i < nodes.size(); ++i) {
        const SAMPhoneme& p = nodes[i].data;
        QString phonemeMath;

        if (p.voiced && p.pClass != CLASS_UNVOICED_CONS && p.pClass != CLASS_STOP_CONS && p.pClass != CLASS_PUNCT) {

            double f1Hz = (p.f1 * hzScale) * mouthFactor;
            double f2Hz = (p.f2 * hzScale) * throatFactor;
            double f3Hz = p.f3 * hzScale; // F3 is usually static

            double a1Lin = p.a1 / 15.0;
            double a2Lin = p.a2 / 15.0;
            double a3Lin = p.a3 / 15.0;

            QString f1Str = QString("%1 * %2(%3integrate(%4))").arg(a1Lin).arg(sinFunc).arg(piMult).arg(f1Hz);
            QString f2Str = QString("%1 * %2(%3integrate(%4))").arg(a2Lin).arg(sinFunc).arg(piMult).arg(f2Hz);
            QString f3Str = QString("%1 * %2(%3integrate(%4))").arg(a3Lin).arg(sinFunc).arg(piMult).arg(f3Hz);

            phonemeMath = QString("((%1 + %2 + %3) * W1(integrate(f)))").arg(f1Str, f2Str, f3Str);

        } else if (p.pClass == CLASS_UNVOICED_CONS) {
            double noiseBase = (p.name == "SH" || p.name == "CH") ? 1500.0 : 2000.0;
            double carrierHz = (p.name == "SH" || p.name == "CH") ? 3500.0 : 5500.0;
            phonemeMath = QString("(randv(t * %1) * %2(%3integrate(%4)))").arg(noiseBase).arg(sinFunc).arg(piMult).arg(carrierHz);

        } else if (p.pClass == CLASS_STOP_CONS) {
            double noiseBase = 1000.0;
            double carrierHz = 1500.0; // Default to mid-range

            if (p.name == "P*" || p.name == "B*") {
                noiseBase = 800.0;
                carrierHz = 1000.0;
            } else if (p.name == "T*" || p.name == "D*") {
                noiseBase = 3000.0;
                carrierHz = 4000.0;
            } else if (p.name == "K*" || p.name == "G*" || p.name == "KX" || p.name == "GX") {
                noiseBase = 1500.0;
                carrierHz = 2000.0;
            }

            QString decayStr = QString("exp(-(t - %1) * 150.0)").arg(currentTime);

            phonemeMath = QString("(randv(t * %1) * %2(%3integrate(%4)) * %5)")
                              .arg(noiseBase).arg(sinFunc).arg(piMult).arg(carrierHz).arg(decayStr);

            if (p.voiced) {
                QString voiceBar = QString("(0.3 * W1(integrate(f)) * exp(-(t - %1) * 50.0))").arg(currentTime);
                phonemeMath = QString("(%1 + %2)").arg(phonemeMath, voiceBar);
            }

        } else {

            phonemeMath = "0.0";
        }

        double endTime = currentTime + nodes[i].duration;
        QString timeGate = (i == nodes.size() - 1) ?
                               QString("(t >= %1)").arg(currentTime) :
                               QString("(t >= %1 & t < %2)").arg(currentTime).arg(endTime);

        if (!o1Matrix.isEmpty()) o1Matrix += " + \n    ";
        o1Matrix += QString("%1 * %2").arg(timeGate, phonemeMath);

        currentTime = endTime;
    }

    QString finalFormula = "clamp(-1.0, \n    " + o1Matrix + "\n, 1.0)";
    if (lofi) {
        finalFormula = QString("clamp(-1.0, floor((%1) * 16)/16, 1.0)").arg(finalFormula);
    }
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
