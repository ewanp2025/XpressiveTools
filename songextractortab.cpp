#include "songextractortab.h"
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QtXml>
#include <cmath>

SongExtractorTab::SongExtractorTab(QWidget *parent) : QWidget(parent) {
    setupUI();
}

SongExtractorTab::~SongExtractorTab() {}

void SongExtractorTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    this->setStyleSheet(R"(
        QWidget { background-color: #121212; color: #00ffcc; font-family: "Consolas", monospace; }
        QTextEdit, QLineEdit, QTableWidget { background-color: #1a1a1a; border: 1px solid #005555; color: #00ffcc; padding: 4px; }
        QPushButton { background-color: #004444; border: 1px solid #00ffcc; padding: 6px; font-weight: bold; }
        QPushButton:hover { background-color: #006666; }
        QHeaderView::section { background-color: #111111; color: #00ffcc; border: 1px solid #005555; }
    )");

    QGroupBox* grpInput = new QGroupBox("1. Xpressive Track Architecture", this);
    QVBoxLayout* inputLayout = new QVBoxLayout(grpInput);

    QHBoxLayout* wLayout = new QHBoxLayout();
    m_inW1 = new QLineEdit(this); m_inW1->setPlaceholderText("W1 Waveform...");
    m_inW2 = new QLineEdit(this); m_inW2->setPlaceholderText("W2 Waveform...");
    m_inW3 = new QLineEdit(this); m_inW3->setPlaceholderText("W3 Waveform...");
    wLayout->addWidget(new QLabel("W1:")); wLayout->addWidget(m_inW1);
    wLayout->addWidget(new QLabel("W2:")); wLayout->addWidget(m_inW2);
    wLayout->addWidget(new QLabel("W3:")); wLayout->addWidget(m_inW3);
    inputLayout->addLayout(wLayout);

    m_inO1 = new QTextEdit(this);
    m_inO1->setPlaceholderText("Paste your  O1 sequenced expression here...");
    m_inO1->setMinimumHeight(100);
    inputLayout->addWidget(m_inO1);

    QHBoxLayout* analyzeRow = new QHBoxLayout();
    m_btnAutoSplit = new QPushButton("Auto-Split Legacy String", this);
    analyzeRow->addWidget(m_btnAutoSplit);

    analyzeRow->addSpacing(20);
    analyzeRow->addWidget(new QLabel("Project BPM:"));
    m_spinBpm = new QSpinBox(this);
    m_spinBpm->setRange(20, 300);
    m_spinBpm->setValue(116);
    analyzeRow->addWidget(m_spinBpm);

    m_btnAnalyze = new QPushButton("Decompile VM & Extract Tracks", this);
    analyzeRow->addWidget(m_btnAnalyze);
    analyzeRow->addStretch();
    inputLayout->addLayout(analyzeRow);

    mainLayout->addWidget(grpInput);

    QGroupBox* grpOutput = new QGroupBox("2. Extracted Tracks & Step Logic", this);
    QVBoxLayout* outputLayout = new QVBoxLayout(grpOutput);

    m_trackTable = new QTableWidget(0, 4, this);
    m_trackTable->setHorizontalHeaderLabels({"Track", "Active Steps", "Length", "Cleaned Core Synth"});
    m_trackTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    outputLayout->addWidget(m_trackTable);

    mainLayout->addWidget(grpOutput);

    QHBoxLayout* exportRow = new QHBoxLayout();
    m_lblStatus = new QLabel("Ready.", this);
    exportRow->addWidget(m_lblStatus);
    exportRow->addStretch();

    m_btnExport = new QPushButton("Export to LMMS Project (.mmp)", this);
    m_btnExport->setEnabled(false);
    exportRow->addWidget(m_btnExport);

    mainLayout->addLayout(exportRow);

    connect(m_btnAutoSplit, &QPushButton::clicked, this, &SongExtractorTab::onAutoSplitClicked);
    connect(m_btnAnalyze, &QPushButton::clicked, this, &SongExtractorTab::onAnalyzeClicked);
    connect(m_btnExport, &QPushButton::clicked, this, &SongExtractorTab::onExportClicked);
}

void SongExtractorTab::onAutoSplitClicked() {
    QString input = m_inO1->toPlainText().replace("\n", " ").replace("\r", "");
    QRegularExpression reW1("(?i)w1:\\s*(.*?)(?=(?:w2:|w3:|o1:|o2:|$))");
    QRegularExpression reW2("(?i)w2:\\s*(.*?)(?=(?:w1:|w3:|o1:|o2:|$))");
    QRegularExpression reW3("(?i)w3:\\s*(.*?)(?=(?:w1:|w2:|o1:|o2:|$))");
    QRegularExpression reO1("(?i)o1:\\s*(.*?)(?=(?:w1:|w2:|w3:|o2:|$))");

    QRegularExpressionMatch match = reW1.match(input);
    if (match.hasMatch()) m_inW1->setText(match.captured(1).trimmed());
    match = reW2.match(input);
    if (match.hasMatch()) m_inW2->setText(match.captured(1).trimmed());
    match = reW3.match(input);
    if (match.hasMatch()) m_inW3->setText(match.captured(1).trimmed());
    match = reO1.match(input);
    if (match.hasMatch()) m_inO1->setPlainText(match.captured(1).trimmed());
}

QString SongExtractorTab::stripOuterParens(QString expr) {
    expr = expr.trimmed();
    while (expr.startsWith("(") && expr.endsWith(")")) {
        int depth = 0;
        bool wrapped = true;
        for (int i = 0; i < expr.length() - 1; ++i) {
            if (expr[i] == '(') depth++;
            else if (expr[i] == ')') depth--;
            if (depth == 0) { wrapped = false; break; }
        }
        if (wrapped) {
            expr = expr.mid(1, expr.length() - 2).trimmed();
        } else {
            break;
        }
    }
    return expr;
}

std::vector<QString> SongExtractorTab::splitAtRoot(const QString& expr, QChar delimiter) {
    std::vector<QString> tokens;
    int depth = 0;
    QString currentToken = "";
    for (int i = 0; i < expr.length(); ++i) {
        QChar c = expr[i];
        if (c == '(' || c == '[') depth++;
        else if (c == ')' || c == ']') depth--;

        if (c == delimiter && depth == 0) {
            tokens.push_back(currentToken.trimmed());
            currentToken = "";
        } else {
            currentToken += c;
        }
    }
    if (!currentToken.trimmed().isEmpty()) tokens.push_back(currentToken.trimmed());
    return tokens;
}



bool SongExtractorTab::evalBooleanExpr(QString expr) {
    expr = stripOuterParens(expr);

    std::vector<QString> orTerms = splitAtRoot(expr, '|');
    if (orTerms.size() == 1) orTerms = splitAtRoot(expr, '+');
    if (orTerms.size() > 1) {
        for (const QString& term : orTerms) {
            if (evalBooleanExpr(term)) return true;
        }
        return false;
    }

    std::vector<QString> andTerms = splitAtRoot(expr, '*');
    if (andTerms.size() == 1) andTerms = splitAtRoot(expr, '&');
    if (andTerms.size() > 1) {
        for (const QString& term : andTerms) {
            if (!evalBooleanExpr(term)) return false;
        }
        return true;
    }

    QRegularExpression reComp("(-?\\d+)\\s*(==|!=|<=|>=|<|>)\\s*(-?\\d+)");
    QRegularExpressionMatch m = reComp.match(expr);
    if (m.hasMatch()) {
        int a = m.captured(1).toInt();
        QString op = m.captured(2);
        int b = m.captured(3).toInt();
        if (op == "==") return a == b;
        if (op == "!=") return a != b;
        if (op == "<=") return a <= b;
        if (op == ">=") return a >= b;
        if (op == "<")  return a < b;
        if (op == ">")  return a > b;
    }

    bool ok; int val = expr.toInt(&ok);
    if (ok) return val > 0;

    return false;
}

bool SongExtractorTab::evaluateStepLogic(QString logicStr, int s) {
    QRegularExpression reModT("floor\\s*\\(\\s*mod\\s*\\([^,]+,\\s*(\\d+)\\s*\\)\\s*\\)");
    QRegularExpressionMatch match;
    while ((match = reModT.match(logicStr)).hasMatch()) {
        int seq = match.captured(1).toInt();
        int val = s % seq;
        logicStr.replace(match.captured(0), QString::number(val));
    }

    QRegularExpression reMod("mod\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)");
    while ((match = reMod.match(logicStr)).hasMatch()) {
        int a = match.captured(1).toInt();
        int b = match.captured(2).toInt();
        int val = (b == 0) ? 0 : (a % b);
        logicStr.replace(match.captured(0), QString::number(val));
    }

    return evalBooleanExpr(logicStr);
}


void SongExtractorTab::onAnalyzeClicked() {
    m_extractedTracks.clear();
    m_masterSequenceLength = 16;

    QString o1String = m_inO1->toPlainText().replace("\n", " ").replace("\r", "");

    if (o1String.startsWith("clamp(-1,") || o1String.startsWith("clamp(-1.0,")) {
        int firstComma = o1String.indexOf(',');
        int lastComma = o1String.lastIndexOf(',');
        if (firstComma != -1 && lastComma != -1 && lastComma > firstComma) {
            o1String = o1String.mid(firstComma + 1, lastComma - firstComma - 1).trimmed();
        }
    }

    std::vector<QString> trackExpressions = splitAtRoot(o1String, '+');

    int trackId = 1;
    for (QString tExpr : trackExpressions) {
        ExtractedTrack track;
        track.id = trackId++;
        track.rawExpression = tExpr;


        tExpr = stripOuterParens(tExpr);

        int trackMaxLen = 16;
        QRegularExpression reAnySeqLen("floor\\s*\\(\\s*mod\\s*\\([^,]+,\\s*(\\d+)\\s*\\)\\s*\\)");
        QRegularExpressionMatchIterator itSeq = reAnySeqLen.globalMatch(tExpr);
        while (itSeq.hasNext()) {
            int l = itSeq.next().captured(1).toInt();
            if (l > trackMaxLen) trackMaxLen = l;
        }
        track.sequenceLength = trackMaxLen;
        m_masterSequenceLength = std::max(m_masterSequenceLength, trackMaxLen);

        QString coreWaveform = "";
        QStringList logicBlocks;
        std::vector<QString> factors = splitAtRoot(tExpr, '*');

        for (QString factor : factors) {

            bool isLogic = (factor.contains("==") || factor.contains("!=") || factor.contains("<") || factor.contains(">"));
            bool isAudio = (factor.contains("W1", Qt::CaseInsensitive) || factor.contains("W2", Qt::CaseInsensitive) ||
                            factor.contains("W3", Qt::CaseInsensitive) || factor.contains("sinew") ||
                            factor.contains("saww") || factor.contains("squarew") || factor.contains("randv"));


            if (isLogic && !isAudio) {
                logicBlocks.append(factor);
            } else {
                if (!coreWaveform.isEmpty()) coreWaveform += " * ";
                coreWaveform += factor;
            }
        }

        if (!logicBlocks.isEmpty()) {
            QString fullLogic = logicBlocks.join(" * ");
            for (int s = 0; s < track.sequenceLength; ++s) {
                if (evaluateStepLogic(fullLogic, s)) {
                    track.activeSteps.insert(s);
                }
            }
        } else {
            if (tExpr.contains("15/tempo") || tExpr.contains("tempo/15")) {
                for (int s = 0; s < track.sequenceLength; ++s) track.activeSteps.insert(s);
            } else {
                track.activeSteps.insert(0);
            }
        }

        track.cleanedCore = cleanCore(coreWaveform);
        m_extractedTracks.push_back(track);
    }

    m_trackTable->setRowCount(m_extractedTracks.size());
    for (size_t i = 0; i < m_extractedTracks.size(); ++i) {
        auto& trk = m_extractedTracks[i];
        m_trackTable->setItem(i, 0, new QTableWidgetItem(QString("Track %1").arg(trk.id)));
        m_trackTable->setItem(i, 1, new QTableWidgetItem(QString::number(trk.activeSteps.size()) + " Hits"));
        m_trackTable->setItem(i, 2, new QTableWidgetItem(QString::number(trk.sequenceLength)));
        m_trackTable->setItem(i, 3, new QTableWidgetItem(trk.cleanedCore));
    }

    m_btnExport->setEnabled(true);
    m_lblStatus->setText(QString("Parsed %1 tracks. Max Project Length: %2 Steps.").arg(m_extractedTracks.size()).arg(m_masterSequenceLength));
}

QString SongExtractorTab::cleanCore(QString expr) {
    QRegularExpression reEnvReset("mod\\s*\\(\\s*t\\s*,\\s*15\\s*/\\s*tempo\\s*\\)");
    expr.replace(reEnvReset, "t");

    QRegularExpression reEnvReset2("mod\\s*\\(\\s*t\\s*\\*\\s*\\(\\s*tempo\\s*/\\s*15\\s*\\)\\s*,\\s*1(?:\\.0)?\\s*\\)");
    expr.replace(reEnvReset2, "t");

    if (expr.trimmed().startsWith("*")) expr = expr.mid(expr.indexOf("*") + 1).trimmed();
    if (expr.trimmed().endsWith("*")) expr.chop(1);

    return expr.trimmed();
}

void SongExtractorTab::onExportClicked() {
    QString savePath = QFileDialog::getSaveFileName(this, "Save Extracted LMMS Project", "", "LMMS Project (*.mmp)");
    if (savePath.isEmpty()) return;
    writeMmpFile(savePath);
}



void SongExtractorTab::writeMmpFile(const QString& filePath) {
    QDomDocument doc;
    QDomProcessingInstruction instr = doc.createProcessingInstruction("xml", "version=\"1.0\"");
    doc.appendChild(instr);

    QDomElement root = doc.createElement("lmms-project");
    root.setAttribute("version", "20");
    root.setAttribute("type", "song");
    doc.appendChild(root);

    QDomElement head = doc.createElement("head");
    head.setAttribute("bpm", QString::number(m_spinBpm->value()));
    root.appendChild(head);

    QDomElement song = doc.createElement("song");
    QDomElement trackContainer = doc.createElement("trackcontainer");
    trackContainer.setAttribute("type", "song");
    song.appendChild(trackContainer);
    root.appendChild(song);

    int ticksPerStep = 12;

    for (const auto& trk : m_extractedTracks) {
        QDomElement track = doc.createElement("track");
        track.setAttribute("type", "0");
        track.setAttribute("name", QString("Extracted T%1").arg(trk.id));
        trackContainer.appendChild(track);

        QDomElement instTrack = doc.createElement("instrumenttrack");
        instTrack.setAttribute("vol", "100");
        instTrack.setAttribute("pan", "0");
        track.appendChild(instTrack);

        QDomElement instrument = doc.createElement("instrument");
        instrument.setAttribute("name", "xpressive");

        QDomElement xpressive = doc.createElement("xpressive");

        xpressive.setAttribute("W1", m_inW1->text().trimmed());
        xpressive.setAttribute("W2", m_inW2->text().trimmed());
        xpressive.setAttribute("W3", m_inW3->text().trimmed());

        QString safeO1 = trk.cleanedCore;
        safeO1.replace("\n", " ").replace("\r", "");
        xpressive.setAttribute("O1", safeO1);
        xpressive.setAttribute("O2", "");

        xpressive.setAttribute("W1sample", "AA==");
        xpressive.setAttribute("W2sample", "AA==");
        xpressive.setAttribute("W3sample", "AA==");
        xpressive.setAttribute("smoothW1", "0");
        xpressive.setAttribute("smoothW2", "0");
        xpressive.setAttribute("smoothW3", "0");
        xpressive.setAttribute("interpolateW1", "0");
        xpressive.setAttribute("interpolateW2", "0");
        xpressive.setAttribute("interpolateW3", "0");
        xpressive.setAttribute("A1", "1");
        xpressive.setAttribute("A2", "1");
        xpressive.setAttribute("A3", "1");
        xpressive.setAttribute("PAN1", "0");
        xpressive.setAttribute("PAN2", "-1");
        xpressive.setAttribute("RELTRANS", "50");
        xpressive.setAttribute("version", "0.1");

        QDomElement keyNode = doc.createElement("key");
        xpressive.appendChild(keyNode);

        instrument.appendChild(xpressive);
        instTrack.appendChild(instrument);

        QDomElement pattern = doc.createElement("pattern");
        pattern.setAttribute("type", "1");
        pattern.setAttribute("pos", "0");
        pattern.setAttribute("len", QString::number(trk.sequenceLength * ticksPerStep * 4));
        pattern.setAttribute("steps", QString::number(trk.sequenceLength));
        track.appendChild(pattern);

        for (int step : trk.activeSteps) {
            QDomElement note = doc.createElement("note");

            note.setAttribute("pos", QString::number(step * ticksPerStep));


            int noteLen = 24;
            if (trk.activeSteps.size() == 1 && trk.activeSteps.count(0)) {
                noteLen = trk.sequenceLength * ticksPerStep;
            }
            note.setAttribute("len", QString::number(noteLen));

            int midiKey = 60;
            if (trk.cleanedCore.contains("integrate(55")) midiKey = 45;

            note.setAttribute("key", QString::number(midiKey));
            note.setAttribute("vol", "100");
            pattern.appendChild(note);
        }
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << doc.toString(4);
        file.close();
        QMessageBox::information(this, "Success", "");
    }
}