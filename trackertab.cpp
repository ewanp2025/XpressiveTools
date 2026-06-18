#include "trackertab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QFormLayout>
#include <QRegularExpression>
#include <cmath>
#include <QApplication>
#include <QClipboard>

TrackerTab::TrackerTab(QWidget *parent) : QWidget(parent) {
    setupUI();
    onInstrumentChanged(0);
}

void TrackerTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    

    this->setStyleSheet(R"(
        QWidget { background-color: #000000; color: #00ff00; font-family: "Courier New", monospace; font-weight: bold; }
        QTableWidget { background-color: #0a0a0a; color: #55ff55; gridline-color: #004400; selection-background-color: #005500; }
        QHeaderView::section { background-color: #111111; color: #00ff00; border: 1px solid #004400; }
        QTextEdit, QComboBox, QSpinBox { background-color: #111111; border: 1px solid #00ff00; color: #00ff00; }
        QPushButton { background-color: #003300; border: 2px solid #00ff00; padding: 5px; }
        QPushButton:hover { background-color: #005500; }
        QGroupBox { border: 1px solid #00aa00; margin-top: 10px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; color: #00ff00; }
        QSlider::groove:horizontal { border: 1px solid #00ff00; height: 8px; background: #111111; }
        QSlider::handle:horizontal { background: #00ff00; width: 14px; margin: -4px 0; }
    )");


    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("BPM (Tempo):"));
    m_spinBPM = new QSpinBox();
    m_spinBPM->setRange(60, 200);
    m_spinBPM->setValue(125);
    topLayout->addWidget(m_spinBPM);
    
    topLayout->addWidget(new QLabel("Syntax:"));
    m_cmbSyntax = new QComboBox();
    m_cmbSyntax->addItems({"Nightly (Variables)", "Legacy (Inline)"});
    topLayout->addWidget(m_cmbSyntax);
    
    QPushButton* btnDemo = new QPushButton("LOAD DEMO SEQUENCE");
    topLayout->addWidget(btnDemo);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);


    QGroupBox* instGroup = new QGroupBox("Instrument Editor");
    QHBoxLayout* instLayout = new QHBoxLayout(instGroup);
    
    QVBoxLayout* instSelectLayout = new QVBoxLayout();
    instSelectLayout->addWidget(new QLabel("Select Inst:"));
    m_cmbInstSelect = new QComboBox();
    for(int i=0; i<8; i++) m_cmbInstSelect->addItem(QString("Inst 0%1").arg(i));
    instSelectLayout->addWidget(m_cmbInstSelect);
    instSelectLayout->addStretch();
    instLayout->addLayout(instSelectLayout);
    
    QFormLayout* mathLayout = new QFormLayout();
    m_txtW1 = new QTextEdit(); m_txtW1->setMaximumHeight(40);
    m_txtW2 = new QTextEdit(); m_txtW2->setMaximumHeight(40);
    m_txtO1 = new QTextEdit(); m_txtO1->setMaximumHeight(50);
    mathLayout->addRow("W1(t):", m_txtW1);
    mathLayout->addRow("W2(t):", m_txtW2);
    mathLayout->addRow("O1(f):", m_txtO1);
    instLayout->addLayout(mathLayout, 3);
    
    QVBoxLayout* paramLayout = new QVBoxLayout();
    paramLayout->addWidget(new QLabel("Decay Envelope:"));
    m_sldDecay = new QSlider(Qt::Horizontal);
    m_sldDecay->setRange(1, 50);
    paramLayout->addWidget(m_sldDecay);
    paramLayout->addWidget(new QLabel("Master Volume:"));
    m_sldVolume = new QSlider(Qt::Horizontal);
    m_sldVolume->setRange(0, 100);
    paramLayout->addWidget(m_sldVolume);
    instLayout->addLayout(paramLayout, 1);
    
    mainLayout->addWidget(instGroup);


    m_grid = new QTableWidget(64, 8);
    QStringList headers;
    for(int i=1; i<=8; i++) headers << QString("Track %1").arg(i);
    m_grid->setHorizontalHeaderLabels(headers);
    m_grid->verticalHeader()->setFixedWidth(40);
    
    for(int r=0; r<64; r++) {
        m_grid->setVerticalHeaderItem(r, new QTableWidgetItem(QString("%1").arg(r, 2, 10, QChar('0'))));
        for(int c=0; c<8; c++) {
            QTableWidgetItem* item = new QTableWidgetItem("--- --");
            item->setTextAlignment(Qt::AlignCenter);
            m_grid->setItem(r, c, item);
        }
    }
    m_grid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_grid, 1); // 1 = stretch factor


    QHBoxLayout* bottomLayout = new QHBoxLayout();
    QPushButton* btnGen = new QPushButton("COMPILE TRACKER TO XPRESSIVE");
    btnGen->setMinimumHeight(40);
    m_txtOutput = new QTextEdit();
    m_txtOutput->setMaximumHeight(80);
    bottomLayout->addWidget(btnGen);
    bottomLayout->addWidget(m_txtOutput, 1);
    mainLayout->addLayout(bottomLayout);


    connect(m_cmbInstSelect, &QComboBox::currentIndexChanged, this, &TrackerTab::onInstrumentChanged);
    connect(m_txtW1, &QTextEdit::textChanged, this, &TrackerTab::saveCurrentInstrument);
    connect(m_txtW2, &QTextEdit::textChanged, this, &TrackerTab::saveCurrentInstrument);
    connect(m_txtO1, &QTextEdit::textChanged, this, &TrackerTab::saveCurrentInstrument);
    connect(m_sldDecay, &QSlider::valueChanged, this, &TrackerTab::saveCurrentInstrument);
    connect(m_sldVolume, &QSlider::valueChanged, this, &TrackerTab::saveCurrentInstrument);
    connect(btnDemo, &QPushButton::clicked, this, &TrackerTab::onLoadDemoClicked);
    connect(btnGen, &QPushButton::clicked, this, &TrackerTab::onGenerateClicked);
    connect(m_grid, &QTableWidget::cellChanged, this, &TrackerTab::onCellChanged);
}

void TrackerTab::onInstrumentChanged(int index) {
    m_isUpdatingUI = true;
    m_currentInstIndex = index;
    m_txtW1->setText(m_instruments[index].w1);
    m_txtW2->setText(m_instruments[index].w2);
    m_txtO1->setText(m_instruments[index].o1);
    m_sldDecay->setValue(m_instruments[index].decay);
    m_sldVolume->setValue(m_instruments[index].volume * 100);
    m_isUpdatingUI = false;
}

void TrackerTab::saveCurrentInstrument() {
    if (m_isUpdatingUI) return;
    m_instruments[m_currentInstIndex].w1 = m_txtW1->toPlainText();
    m_instruments[m_currentInstIndex].w2 = m_txtW2->toPlainText();
    m_instruments[m_currentInstIndex].o1 = m_txtO1->toPlainText();
    m_instruments[m_currentInstIndex].decay = m_sldDecay->value();
    m_instruments[m_currentInstIndex].volume = m_sldVolume->value() / 100.0;
}

void TrackerTab::onCellChanged(int row, int column) {
    if (m_isUpdatingUI) return;
    QString text = m_grid->item(row, column)->text().toUpper().trimmed();
    
    QRegularExpression re("^([A-G][#-]?\\d)\\s*(\\d{1,2})$");
    QRegularExpressionMatch match = re.match(text);
    
    m_isUpdatingUI = true;
    if (match.hasMatch()) {
        QString note = match.captured(1);
        if(!note.contains("-") && !note.contains("#")) note.insert(1, "-");
        QString inst = QString("%1").arg(match.captured(2).toInt(), 2, 10, QChar('0'));
        m_grid->item(row, column)->setText(note + " " + inst);
    } else {
        m_grid->item(row, column)->setText("--- --");
    }
    m_isUpdatingUI = false;
}

double TrackerTab::noteToFreq(const QString& noteStr) {
    if (noteStr.length() < 3) return 0.0;
    QString note = noteStr.left(2);
    int octave = noteStr.mid(2, 1).toInt();
    QStringList notes = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
    int index = notes.indexOf(note);
    if (index == -1) return 0.0;
    int keyNumber = index + (octave + 1) * 12;
    return 440.0 * std::pow(2.0, (keyNumber - 69) / 12.0);
}

QString TrackerTab::parseMacro(QString expr, const QString& w1, const QString& w2) {

    auto replaceMacro = [](QString& target, const QString& macroName, const QString& replacement) {
        int idx = 0;
        while ((idx = target.indexOf(macroName + "(", idx)) != -1) {
            int start = idx + macroName.length() + 1;
            int parenCount = 1;
            int end = start;


            while (end < target.length() && parenCount > 0) {
                if (target[end] == '(') parenCount++;
                else if (target[end] == ')') parenCount--;
                end++;
            }

            if (parenCount == 0) {
                QString inner = target.mid(start, end - start - 1);
                QString sub = replacement;

                sub.replace(QRegularExpression("\\bt\\b"), "(" + inner + ")");

                target.replace(idx, end - idx, "(" + sub + ")");
                idx += sub.length() + 2;
            } else {
                idx++;
            }
        }
    };

    replaceMacro(expr, "W1", w1);
    replaceMacro(expr, "W2", w2);
    return expr;
}

void TrackerTab::onGenerateClicked() {
    std::vector<TrackerEvent> instEvents[8];

    for(int r=0; r<64; r++) {
        for(int c=0; c<8; c++) {
            QString text = m_grid->item(r, c)->text();
            if (text == "--- --") continue;
            QString note = text.left(3);
            int instID = text.right(2).toInt();
            if (instID >= 0 && instID < 8) {
                instEvents[instID].push_back({r, noteToFreq(note)});
            }
        }
    }

    QString nightlyVars = QString("var T = mod(t*(tempo/15), 64);\n");
    QStringList nightlyOutputs;
    QStringList legacyOutputs;

    for(int i=0; i<8; i++) {
        if(instEvents[i].empty()) continue;

        QString pitchExpr, envExpr;
        int numEvents = instEvents[i].size();

        for(int e=0; e<numEvents; e++) {
            int start = instEvents[i][e].step;
            int end = (e == numEvents - 1) ? instEvents[i][0].step : instEvents[i][e+1].step;
            double freq = instEvents[i][e].freq;

            QString cond;
            if (start < end) {
                cond = QString("(T >= %1 & T < %2)").arg(start).arg(end);
            } else {

                cond = QString("((T >= %1) + (T < %2))").arg(start).arg(end);
                if (numEvents == 1) cond = "1";
            }

            QString localEnv = QString("exp(-mod((T - %1 + 64), 64) * %2)").arg(start).arg(m_instruments[i].decay);

            pitchExpr += QString("(%1 * %2)").arg(cond).arg(freq);
            envExpr += QString("(%1 * %2)").arg(cond).arg(localEnv);

            if (e < numEvents - 1) {
                pitchExpr += " + ";
                envExpr += " + ";
            }
        }

        QString id = QString("%1").arg(i, 2, 10, QChar('0'));
        QString parsedO1 = parseMacro(m_instruments[i].o1, m_instruments[i].w1, m_instruments[i].w2);


        nightlyVars += QString("var pitch_%1 = %2;\n").arg(id).arg(pitchExpr);
        nightlyVars += QString("var env_%1 = %2;\n").arg(id).arg(envExpr);

        QString nightlyO1 = parsedO1;
        nightlyO1.replace(QRegularExpression("\\bf\\b"), QString("pitch_%1").arg(id));
        nightlyVars += QString("var out_%1 = (%2) * env_%1 * %3;\n")
                           .arg(id).arg(nightlyO1).arg(m_instruments[i].volume);
        nightlyOutputs << QString("out_%1").arg(id);


        QString legacyO1 = parsedO1;

        legacyO1.replace(QRegularExpression("\\bf\\b"), QString("(%1)").arg(pitchExpr));

        QString legacyInstFull = QString("((%1) * (%2) * %3)").arg(legacyO1).arg(envExpr).arg(m_instruments[i].volume);


        legacyInstFull.replace(QRegularExpression("\\bT\\b"), "(mod(t*(tempo/15), 64))");

        legacyOutputs << legacyInstFull;
    }

    if (nightlyOutputs.isEmpty()) {
        m_txtOutput->setText("Grid is empty!");
        return;
    }

    QString finalExpr;
    if (m_cmbSyntax->currentIndex() == 0) {

        finalExpr = nightlyVars + "clamp(-1.0, " + nightlyOutputs.join(" + ") + ", 1.0)";
    } else {

        finalExpr = "clamp(-1.0, " + legacyOutputs.join(" + ") + ", 1.0)";
    }

    m_txtOutput->setText(finalExpr);
    QApplication::clipboard()->setText(finalExpr);
    m_txtOutput->append("\n// COPIED TO CLIPBOARD");
}

void TrackerTab::onLoadDemoClicked() {
    m_isUpdatingUI = true;
    m_spinBPM->setValue(125);


    m_instruments[0].w1 = "squarew(t) + 0.5*sinew(t*2)";
    m_instruments[0].w2 = "0";
    m_instruments[0].o1 = "W1(integrate(f))";
    m_instruments[0].decay = 8.0; m_instruments[0].volume = 0.8;


    m_instruments[1].w1 = "0"; m_instruments[1].w2 = "0";
    m_instruments[1].o1 = "sinew(integrate(f*(1+2*exp(-mod((T+64),64)*60))))";
    m_instruments[1].decay = 12.0; m_instruments[1].volume = 1.0;


    m_instruments[2].w1 = "0"; m_instruments[2].w2 = "0";
    m_instruments[2].o1 = "randv(t*srate)";
    m_instruments[2].decay = 40.0; m_instruments[2].volume = 0.3;


    m_instruments[3].w1 = "saww(t)";
    m_instruments[3].w2 = "0";
    m_instruments[3].o1 = "W1(integrate(f)) + W1(integrate(f*1.189)) + W1(integrate(f*1.498))";
    m_instruments[3].decay = 2.0; m_instruments[3].volume = 0.5;

    onInstrumentChanged(m_currentInstIndex);


    for(int r=0; r<64; r++)
        for(int c=0; c<8; c++)
            m_grid->item(r, c)->setText("--- --");


    for(int i=0; i<64; i+=4) {
        m_grid->item(i, 0)->setText("C-2 01");
        if(i%8 != 0) m_grid->item(i, 2)->setText("C-4 02");
        if(i%2 == 0) m_grid->item(i+2, 2)->setText("C-4 02");
    }


    m_grid->item(0, 1)->setText("C-2 00");
    m_grid->item(3, 1)->setText("C-2 00");
    m_grid->item(8, 1)->setText("C-2 00");
    m_grid->item(11, 1)->setText("D#2 00");
    m_grid->item(16, 1)->setText("C-2 00");
    m_grid->item(19, 1)->setText("C-2 00");
    m_grid->item(24, 1)->setText("C-2 00");
    m_grid->item(27, 1)->setText("A#1 00");
    

    for(int r=0; r<32; r++) {
        if(m_grid->item(r, 1)->text() != "--- --") 
            m_grid->item(r+32, 1)->setText(m_grid->item(r, 1)->text());
    }


    m_grid->item(0, 3)->setText("C-4 03");
    m_grid->item(32, 3)->setText("G#3 03");

    m_isUpdatingUI = false;
}