#include "pcmeditortab.h"
#include "synthengine.h"
// #include "universalscope.h" // Uncomment this if UniversalScope has its own header file
#include <cmath>
#include <algorithm>
#include <QDebug>
#include <QInputDialog>
#include <QDebug>
#include <functional>

PCMEditorTab::PCMEditorTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth) {
    setupAmigaUI();
    m_nightlySampleRate = 8000.0;
    m_nightlyBuffer.resize(8000);
    for(size_t i = 0; i < 8000; ++i) {
        // Generates a cool decaying 808-style sine wave
        m_nightlyBuffer[i] = std::sin(i * 0.05) * std::exp(-i * 0.001);
    }
    updateNightlyPreview();
}
void PCMEditorTab::setupAmigaUI() {
    // --- AMIGA SAMPLER STYLING ---
    this->setStyleSheet(R"(
        QWidget {
            background-color: #111111; color: #39ff14;
            font-family: "Courier New", monospace; font-size: 12px; font-weight: bold;
        }
        QTextEdit, QTableWidget {
            background-color: #1a1a1a; border: 1px solid #39ff14; color: #55ff55;
        }
        QPushButton {
            background-color: #333333; border: 2px solid #555555; padding: 5px; color: #aaaaaa;
        }
        QPushButton:hover { background-color: #444444; color: #ffffff; }
        QPushButton:pressed { border: 2px inset #555555; background-color: #222222; }
        QLabel { background-color: transparent; border: none; padding: 2px; }
        QSlider::groove:horizontal { border: 1px solid #999999; height: 8px; background: #222222; }
        QSlider::handle:horizontal { background: #39ff14; width: 18px; margin: -2px 0; border: 1px solid #fff; }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    nightlyPcmInput = new QTextEdit();
    nightlyPcmInput->setPlaceholderText("Paste original var s := floor(t * 8000)... expression here");
    nightlyPcmInput->setMaximumHeight(60);
    mainLayout->addWidget(new QLabel("1. Source PCM Expression: (Must be nightly format)"));
    mainLayout->addWidget(nightlyPcmInput);

    QWidget* canvasPlaceholder = new QWidget();
    canvasPlaceholder->setObjectName("waveArea");
    canvasPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    canvasPlaceholder->setMinimumHeight(150);
    mainLayout->addWidget(new QLabel("Raw Audio (Click & Drag to Trim):"));
    mainLayout->addWidget(canvasPlaceholder);

    canvasPlaceholder->setStyleSheet("background: transparent;");
    canvasPlaceholder->setAttribute(Qt::WA_TransparentForMouseEvents);

    QHBoxLayout *midLayout = new QHBoxLayout();
    QFormLayout *slidersLayout = new QFormLayout();

    ntTrimStart = new QSlider(Qt::Horizontal);   ntTrimStart->setRange(0, 10000);   ntTrimStart->setValue(0);
    ntTrimLength = new QSlider(Qt::Horizontal);  ntTrimLength->setRange(100, 10000); ntTrimLength->setValue(10000);
    ntSpeedStretch = new QSlider(Qt::Horizontal);ntSpeedStretch->setRange(10, 300); ntSpeedStretch->setValue(100);
    ntBitcrush = new QSlider(Qt::Horizontal);    ntBitcrush->setRange(1, 64);       ntBitcrush->setValue(64);
    ntWavefold = new QSlider(Qt::Horizontal);    ntWavefold->setRange(10, 200);     ntWavefold->setValue(10);
    ntFormantShift = new QSlider(Qt::Horizontal);ntFormantShift->setRange(0, 2000); ntFormantShift->setValue(0);

    slidersLayout->addRow("Trim Start:", ntTrimStart);
    slidersLayout->addRow("Trim Length (Loop):", ntTrimLength);
    slidersLayout->addRow("Sequence BPM %:", ntSpeedStretch);
    slidersLayout->addRow("Bit Depth (Crush):", ntBitcrush);
    slidersLayout->addRow("Wavefold Drive:", ntWavefold);
    slidersLayout->addRow("Formant RM (Hz):", ntFormantShift);

    QVBoxLayout *slicerLayout = new QVBoxLayout();
    QHBoxLayout *sliceHeader = new QHBoxLayout();
    sliceHeader->addWidget(new QLabel("Segment Sequence:"));
    ntSliceCount = new QSpinBox();
    ntSliceCount->setRange(1, 32);
    ntSliceCount->setValue(1);
    sliceHeader->addWidget(new QLabel("Steps:"));
    sliceHeader->addWidget(ntSliceCount);
    slicerLayout->addLayout(sliceHeader);

    ntSliceTable = new QTableWidget();
    ntSliceTable->setColumnCount(3);
    ntSliceTable->setHorizontalHeaderLabels({"Beats", "Pitch (st)", "Glide"});
    ntSliceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ntSliceTable->setRowCount(1);
    ntSliceTable->setItem(0, 0, new QTableWidgetItem("1.0"));
    ntSliceTable->setItem(0, 1, new QTableWidgetItem("0"));
    QTableWidgetItem *cb0 = new QTableWidgetItem();
    cb0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    cb0->setCheckState(Qt::Unchecked);
    ntSliceTable->setItem(0, 2, cb0);
    slicerLayout->addWidget(ntSliceTable);

    midLayout->addLayout(slidersLayout, 2);
    midLayout->addLayout(slicerLayout, 1);
    mainLayout->addLayout(midLayout);

    nightlyPcmOutput = new QTextEdit();
    nightlyPcmOutput->setReadOnly(true);
    nightlyPcmOutput->setMaximumHeight(100);
    mainLayout->addWidget(new QLabel("2. Edited Output Expression:"));
    mainLayout->addWidget(nightlyPcmOutput);

    QHBoxLayout *amigaBtnLayout = new QHBoxLayout();
    QPushButton* btnTrim = new QPushButton("TRIM");
    QPushButton* btnPingPong = new QPushButton("PING-PONG");
    QPushButton* btnRev = new QPushButton("REVERSE");
    QPushButton* btnNorm = new QPushButton("NORMALIZE");
    QPushButton* btnPitch = new QPushButton("DETECT PITCH");
    QPushButton* btnTimeStretch = new QPushButton("TIME STRETCH");

    amigaBtnLayout->addWidget(btnTrim);
    amigaBtnLayout->addWidget(btnPingPong);
    amigaBtnLayout->addWidget(btnRev);
    amigaBtnLayout->addWidget(btnNorm);
    amigaBtnLayout->addWidget(btnPitch);
    amigaBtnLayout->addWidget(btnTimeStretch);
    mainLayout->addLayout(amigaBtnLayout);

    QHBoxLayout *akaiBtnLayout = new QHBoxLayout();
    QPushButton* btnChop = new QPushButton("CHOP (DEL INSIDE)");
    QPushButton* btnFadeIn = new QPushButton("FADE IN");
    QPushButton* btnFadeOut = new QPushButton("FADE OUT");
    QPushButton* btnGrime = new QPushButton("AKAI 12-BIT GRIME");

    akaiBtnLayout->addWidget(btnChop);
    akaiBtnLayout->addWidget(btnFadeIn);
    akaiBtnLayout->addWidget(btnFadeOut);
    akaiBtnLayout->addWidget(btnGrime);
    mainLayout->addLayout(akaiBtnLayout);


    connect(btnTrim, &QPushButton::clicked, this, &PCMEditorTab::onTrimClicked);
    connect(btnPingPong, &QPushButton::clicked, this, &PCMEditorTab::onPingPongClicked);
    connect(btnRev, &QPushButton::clicked, this, &PCMEditorTab::onReverseClicked);
    connect(btnNorm, &QPushButton::clicked, this, &PCMEditorTab::onNormalizeClicked);
    connect(btnPitch, &QPushButton::clicked, this, &PCMEditorTab::onDetectPitchClicked);
    connect(btnTimeStretch, &QPushButton::clicked, this, &PCMEditorTab::onTimeStretchClicked);

    connect(btnChop, &QPushButton::clicked, this, &PCMEditorTab::onChopClicked);
    connect(btnFadeIn, &QPushButton::clicked, this, &PCMEditorTab::onFadeInClicked);
    connect(btnFadeOut, &QPushButton::clicked, this, &PCMEditorTab::onFadeOutClicked);
    connect(btnGrime, &QPushButton::clicked, this, &PCMEditorTab::onAkaiGrimeClicked);

    QHBoxLayout *btnLayout = new QHBoxLayout();

    buildModeCombo = new QComboBox();
    buildModeCombo->addItems({"Modern (Nightly)", "Legacy (Additive/Tree)"});

    btnPlayNightly = new QPushButton("▶ Play Edited PCM");
    btnPlayNightly->setCheckable(true);
    btnCopyNightly = new QPushButton("📋 Copy Expression");

    btnLayout->addWidget(new QLabel("Build Mode:"));
    btnLayout->addWidget(buildModeCombo);
    btnLayout->addWidget(btnPlayNightly);
    btnLayout->addWidget(btnCopyNightly);
    mainLayout->addLayout(btnLayout);

    auto triggerUpdate = [=](){
        parseNightlyInput();
        generateNightlyExpression();
        updateNightlyPreview();
    };

    connect(buildModeCombo, &QComboBox::currentIndexChanged, this, triggerUpdate);
    connect(ntTrimStart, &QSlider::valueChanged, this, triggerUpdate);
    connect(ntTrimLength, &QSlider::valueChanged, this, triggerUpdate);
    connect(ntSpeedStretch, &QSlider::valueChanged, this, triggerUpdate);
    connect(ntBitcrush, &QSlider::valueChanged, this, triggerUpdate);
    connect(ntWavefold, &QSlider::valueChanged, this, triggerUpdate);
    connect(ntFormantShift, &QSlider::valueChanged, this, triggerUpdate);
    connect(nightlyPcmInput, &QTextEdit::textChanged, this, triggerUpdate);
    connect(ntSliceTable, &QTableWidget::cellChanged, this, triggerUpdate);

    connect(ntSliceCount, QOverload<int>::of(&QSpinBox::valueChanged), [=](int rows){
        int current = ntSliceTable->rowCount();
        ntSliceTable->setRowCount(rows);
        for(int i = current; i < rows; ++i) {
            ntSliceTable->setItem(i, 0, new QTableWidgetItem("1.0"));
            ntSliceTable->setItem(i, 1, new QTableWidgetItem("0"));
            QTableWidgetItem *cb = new QTableWidgetItem();
            cb->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            cb->setCheckState(Qt::Unchecked);
            ntSliceTable->setItem(i, 2, cb);
        }
        triggerUpdate();
    });

    connect(btnPlayNightly, &QPushButton::clicked, this, &PCMEditorTab::togglePlayNightly);
    connect(btnCopyNightly, &QPushButton::clicked, this, [=](){
        QApplication::clipboard()->setText(nightlyPcmOutput->toPlainText());
    });
}

int PCMEditorTab::pixelToIndex(int pixelX) {
    QWidget* waveArea = findChild<QWidget*>("waveArea");
    if (!waveArea || m_nightlyBuffer.empty() || waveArea->width() == 0) return 0;

    int localX = pixelX - waveArea->x(); // Remove the UI offset
    float ratio = static_cast<float>(m_nightlyBuffer.size()) / waveArea->width();
    int idx = static_cast<int>(localX * ratio);
    return std::clamp(idx, 0, static_cast<int>(m_nightlyBuffer.size() - 1));
}

void PCMEditorTab::mousePressEvent(QMouseEvent *event) {
    QWidget* waveArea = findChild<QWidget*>("waveArea");
    if (waveArea && waveArea->geometry().contains(event->pos())) {
        selectionStartPixel = event->x();
        selectionEndPixel = event->x();
        update();
    }
}

void PCMEditorTab::mouseMoveEvent(QMouseEvent *event) {
    QWidget* waveArea = findChild<QWidget*>("waveArea");
    if (waveArea && selectionStartPixel != -1) {
        selectionEndPixel = std::clamp(event->x(), waveArea->x(), waveArea->x() + waveArea->width());
        update();
    }
}

void PCMEditorTab::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(10, 10, 10));

    QWidget* waveArea = findChild<QWidget*>("waveArea");
    if (!waveArea || m_nightlyBuffer.empty()) return;


    QRect canvasBox = waveArea->geometry();
    painter.setClipRect(canvasBox);
    int w = canvasBox.width();
    int h = canvasBox.height();
    int startX = canvasBox.x();
    int startY = canvasBox.y();
    int midY = startY + (h / 2);

    if (selectionStartPixel != -1 && selectionEndPixel != -1) {
        int left = std::min(selectionStartPixel, selectionEndPixel);
        int right = std::max(selectionStartPixel, selectionEndPixel);
        painter.fillRect(left, startY, right - left, h, QColor(0, 100, 0, 100));
    }

    painter.setPen(QPen(QColor(57, 255, 20), 1)); // Neon Green
    float pointsPerPixel = static_cast<float>(m_nightlyBuffer.size()) / w;

    for (int px = 0; px < w; ++px) {
        int startIdx = static_cast<int>(px * pointsPerPixel);
        int endIdx = static_cast<int>((px + 1) * pointsPerPixel);
        endIdx = std::min(endIdx, static_cast<int>(m_nightlyBuffer.size()));

        float minVal = 0.0f, maxVal = 0.0f;
        for (int i = startIdx; i < endIdx; ++i) {
            float val = m_nightlyBuffer[i];
            if (val < minVal) minVal = val;
            if (val > maxVal) maxVal = val;
        }

        int drawX = startX + px;
        int y1 = midY - static_cast<int>(maxVal * (h / 2.0f) * 0.9f);
        int y2 = midY - static_cast<int>(minVal * (h / 2.0f) * 0.9f);
        if (y1 == y2) y2 += 1;

        painter.drawLine(drawX, y1, drawX, y2);
    }
}



void PCMEditorTab::parseNightlyInput() {
    QString rawCode = nightlyPcmInput->toPlainText();
    if (rawCode.isEmpty()) {
        m_nightlyBuffer.clear();
        update();
        return;
    }

    QRegularExpression srRegex("floor\\(\\s*t\\s*\\*\\s*([0-9]+(?:\\.[0-9]+)?)\\s*\\)");
    QRegularExpressionMatch srMatch = srRegex.match(rawCode);
    m_nightlySampleRate = srMatch.hasMatch() ? srMatch.captured(1).toDouble() : 8000.0;

    m_nightlyBuffer.clear();
    QRegularExpression valRegex("(-?[0-9]+\\.[0-9]{1,6})");
    QRegularExpressionMatchIterator it = valRegex.globalMatch(rawCode);

    while (it.hasNext()) {
        m_nightlyBuffer.push_back(it.next().captured(1).toDouble());
    }
    if (m_nightlyBuffer.empty()) m_nightlyBuffer.push_back(0.0);

    update();
}

void PCMEditorTab::updateNightlyPreview() {
    if (m_nightlyBuffer.empty() || !m_ghostSynth) return;

    double maxDur = m_nightlyBuffer.size() / m_nightlySampleRate;
    if (maxDur <= 0) maxDur = 1.0;

    double trimStart = (ntTrimStart->value() / 10000.0) * maxDur;
    double trimLen = (ntTrimLength->value() / 10000.0) * maxDur;
    if (trimLen < 0.001) trimLen = 0.001;


    QWidget* waveArea = findChild<QWidget*>("waveArea");
    int canvasW = waveArea ? waveArea->width() : width();
    int offsetX = waveArea ? waveArea->x() : 0;

    selectionStartPixel = offsetX + (trimStart / maxDur) * canvasW;
    selectionEndPixel = offsetX + ((trimStart + trimLen) / maxDur) * canvasW;
    update();

    double seq_speed = ntSpeedStretch->value() / 100.0;
    int bits = ntBitcrush->value();
    double foldDrive = ntWavefold->value() / 10.0;
    double formant = ntFormantShift->value();

    struct Step { double startT; double endT; double dur; double pitch; bool glide; };
    int slices = ntSliceCount->value();
    std::vector<Step> seq(slices);

    double totalBeats = 0.0;
    for(int i = 0; i < slices; ++i) {
        double beats = ntSliceTable->item(i, 0) ? ntSliceTable->item(i, 0)->text().toDouble() : 1.0;
        if (beats < 0.001) beats = 0.001;
        double st = ntSliceTable->item(i, 1) ? ntSliceTable->item(i, 1)->text().toDouble() : 0.0;
        bool gl = ntSliceTable->item(i, 2) ? (ntSliceTable->item(i, 2)->checkState() == Qt::Checked) : false;

        seq[i].dur = beats;
        seq[i].startT = totalBeats;
        seq[i].pitch = std::pow(2.0, st / 12.0);
        seq[i].glide = gl;
        totalBeats += beats;
        seq[i].endT = totalBeats;
    }
    if (totalBeats <= 0.0) totalBeats = 1.0;

    double sr = m_nightlySampleRate;
    std::vector<double> buf = m_nightlyBuffer;

    auto createAlgo = [=]() {
        return [=, phase = 0.0, last_t = -1.0](double t) mutable {
            double dt = (last_t < 0.0) ? 0.0000226 : (t - last_t);
            last_t = t;
            if (dt < 0.0 || dt > 0.1) dt = 0.0000226;

            double seq_t = std::fmod(t * seq_speed * 4.0, totalBeats);

            Step curr = seq[0], next = seq[0];
            for(int i = 0; i < slices; ++i) {
                if(seq_t >= seq[i].startT && seq_t < seq[i].endT) {
                    curr = seq[i];
                    next = seq[(i + 1) % slices];
                    break;
                }
            }

            double activePitch = curr.pitch;
            if (curr.glide) {
                double localPhase = (seq_t - curr.startT) / curr.dur;
                activePitch = curr.pitch + (next.pitch - curr.pitch) * localPhase;
            }

            phase += activePitch * dt;
            double t_mod = trimStart + std::fmod(phase, trimLen);

            int s = (int)(t_mod * sr);
            if (s < 0) s = 0;
            if (s >= (int)buf.size()) s = buf.size() - 1;

            double raw_pcm = buf[s];
            if (bits < 64) raw_pcm = std::floor(raw_pcm * bits) / bits;
            if (formant > 0) raw_pcm *= std::sin(t * formant * 2.0 * 3.14159265);
            if (foldDrive > 1.0) raw_pcm = std::sin(raw_pcm * foldDrive);

            return raw_pcm;
        };
    };

    if (btnPlayNightly->isChecked()) m_ghostSynth->setAudioSource(createAlgo());
}

void PCMEditorTab::generateNightlyExpression() {
    if (m_nightlyBuffer.empty()) return;


    if (buildModeCombo->currentIndex() == 1) {
        QString finalExpr = generateLegacyPCM(m_nightlyBuffer, m_nightlySampleRate);
        nightlyPcmOutput->setText(finalExpr);
        return;
    }


    double maxDur = m_nightlyBuffer.size() / m_nightlySampleRate;
    if (maxDur <= 0) maxDur = 1.0;

    double trimStart = (ntTrimStart->value() / 10000.0) * maxDur;
    double trimLen = (ntTrimLength->value() / 10000.0) * maxDur;
    if (trimLen < 0.001) trimLen = 0.001;

    double speed = ntSpeedStretch->value() / 100.0;
    int bits = ntBitcrush->value();
    double foldDrive = ntWavefold->value() / 10.0;
    double formant = ntFormantShift->value();

    int slices = ntSliceCount->value();
    bool isLegacy = false;


    std::function<QString(int, int)> buildTree;
    buildTree = [&](int start, int end) -> QString {
        if (start == end) {

            return QString("(%1)").arg(m_nightlyBuffer[start], 0, 'f', 3);
        }


        int mid = start + (end - start) / 2;
        QString left = buildTree(start, mid);
        QString right = buildTree(mid + 1, end);

        return QString("((s <= %1) ? %2 : %3)").arg(mid).arg(left).arg(right);
    };


    QString raw_pcm_val = buildTree(0, m_nightlyBuffer.size() - 1);

    QString seq_t_val, p_val = "1.0", nextP_val = "1.0", start_val = "0.0", dur_val = "1.0", glide_val = "0";
    QString active_pitch_val = "1.0";

    if (slices > 1) {
        std::vector<double> endT(slices);
        double tb = 0.0;
        for(int i=0; i<slices; ++i) {
            double b = std::max(0.001, ntSliceTable->item(i, 0) ? ntSliceTable->item(i, 0)->text().toDouble() : 1.0);
            tb += b;
            endT[i] = tb;
        }
        double totalBeats = tb;

        seq_t_val = QString("mod(t * %1 * 4.0, %2)").arg(speed, 0, 'f', 4).arg(totalBeats, 0, 'f', 4);

        for (int i = slices - 1; i >= 0; --i) {
            double eTime = endT[i];
            double sTime = (i == 0) ? 0.0 : endT[i-1];
            double beats = eTime - sTime;

            double st = ntSliceTable->item(i, 1) ? ntSliceTable->item(i, 1)->text().toDouble() : 0.0;
            double nextSt = ntSliceTable->item((i+1)%slices, 1) ? ntSliceTable->item((i+1)%slices, 1)->text().toDouble() : 0.0;
            bool glide = ntSliceTable->item(i, 2) ? (ntSliceTable->item(i, 2)->checkState() == Qt::Checked) : false;

            double pMult = std::pow(2.0, st / 12.0);
            double nextPMult = std::pow(2.0, nextSt / 12.0);

            QString thresh = isLegacy ? QString("(%1 < %2)").arg(seq_t_val).arg(eTime, 0, 'f', 4)
                                      : QString("(seq_t < %1)").arg(eTime, 0, 'f', 4);

            p_val = QString("(%1 ? %2 : %3)").arg(thresh).arg(pMult, 0, 'f', 4).arg(p_val);
            nextP_val = QString("(%1 ? %2 : %3)").arg(thresh).arg(nextPMult, 0, 'f', 4).arg(nextP_val);
            start_val = QString("(%1 ? %2 : %3)").arg(thresh).arg(sTime, 0, 'f', 4).arg(start_val);
            dur_val = QString("(%1 ? %2 : %3)").arg(thresh).arg(beats, 0, 'f', 4).arg(dur_val);
            glide_val = QString("(%1 ? %2 : %3)").arg(thresh).arg(glide ? "1" : "0").arg(glide_val);
        }

        if (isLegacy) {
            QString local_phase = QString("((%1 - %2) / %3)").arg(seq_t_val, start_val, dur_val);
            active_pitch_val = QString("((%1 > 0.5) ? (%2 + (%3 - %2) * %4) : %2)").arg(glide_val, p_val, nextP_val, local_phase);
        } else {
            active_pitch_val = "(glide > 0.5) ? (p + (p_next - p) * local_phase) : p";
        }
    }

    QString t_mod_val = QString("(%1 + mod(integrate(%2), %3))").arg(trimStart, 0, 'f', 4).arg(active_pitch_val).arg(trimLen, 0, 'f', 4);
    QString s_val = QString("floor(%1 * %2)").arg(isLegacy ? t_mod_val : "t_mod").arg(m_nightlySampleRate);


    if (isLegacy) {

        raw_pcm_val.replace(QRegularExpression("\\bt\\b"), t_mod_val);
        raw_pcm_val.replace(QRegularExpression("\\bs\\b"), s_val);
    }

    QString crushed_val = raw_pcm_val;
    if (bits < 64) {
        crushed_val = QString("(floor(%1 * %2) / %2)").arg(isLegacy ? raw_pcm_val : "raw_pcm").arg(bits);
    }

    QString shifted_val = crushed_val;
    if (formant > 0) {
        shifted_val = QString("(%1 * sinew(t * %2))").arg(isLegacy ? crushed_val : (bits < 64 ? "crushed" : "raw_pcm")).arg(formant, 0, 'f', 2);
    }

    QString folded_val = shifted_val;
    if (foldDrive > 1.0) {
        folded_val = QString("sin(%1 * %2)").arg(isLegacy ? shifted_val : (formant > 0 ? "shifted" : (bits < 64 ? "crushed" : "raw_pcm"))).arg(foldDrive, 0, 'f', 2);
    }

    QString final_val = QString("clamp(-1, %1, 1)").arg(isLegacy ? folded_val : (foldDrive > 1.0 ? "folded" : (formant > 0 ? "shifted" : (bits < 64 ? "crushed" : "raw_pcm"))));

    QString mutatedCode;

    if (isLegacy) {
        mutatedCode = final_val + ";";
    } else {
        if (slices > 1) {
            mutatedCode += QString("var seq_t := %1;\n").arg(seq_t_val);
            mutatedCode += QString("var p := %1;\n").arg(p_val);
            mutatedCode += QString("var p_next := %1;\n").arg(nextP_val);
            mutatedCode += QString("var s_start := %1;\n").arg(start_val);
            mutatedCode += QString("var s_dur := %1;\n").arg(dur_val);
            mutatedCode += QString("var glide := %1;\n\n").arg(glide_val);
            mutatedCode += "var local_phase := (seq_t - s_start) / s_dur;\n";
            mutatedCode += QString("var active_pitch := %1;\n\n").arg(active_pitch_val);
        } else {
            mutatedCode += "var active_pitch := 1.0;\n\n";
        }

        mutatedCode += QString("var t_mod := %1;\n").arg(t_mod_val);
        mutatedCode += QString("var s := %1;\n").arg(s_val);
        mutatedCode += QString("var raw_pcm := %1;\n\n").arg(raw_pcm_val);

        QString activeSignal = "raw_pcm";
        if (bits < 64) {
            mutatedCode += QString("var crushed := floor(raw_pcm * %1) / %1;\n").arg(bits);
            activeSignal = "crushed";
        }
        if (formant > 0) {
            mutatedCode += QString("var shifted := %1 * sinew(t * %2);\n").arg(activeSignal).arg(formant, 0, 'f', 2);
            activeSignal = "shifted";
        }
        if (foldDrive > 1.0) {
            mutatedCode += QString("var folded := sin(%1 * %2);\n").arg(activeSignal).arg(foldDrive, 0, 'f', 2);
            activeSignal = "folded";
        }
        mutatedCode += QString("clamp(-1, %1, 1);").arg(activeSignal);
    }

    nightlyPcmOutput->setText(mutatedCode);
}

void PCMEditorTab::togglePlayNightly() {
    qDebug() << "[UI] Play button clicked.";
    if (!m_ghostSynth) return;

    if (btnPlayNightly->isChecked()) {
        btnPlayNightly->setText("⏹ Stop Edited");
        btnPlayNightly->setStyleSheet("background-color: #883333; color: white; font-weight: bold; height: 35px;");

        qDebug() << "[UI] Stopping engine to prevent cross-thread collisions...";
        m_ghostSynth->stop();


        qDebug() << "[UI] Preparing to play. Parsing input...";
        parseNightlyInput();

        qDebug() << "[UI] Generating expression...";
        generateNightlyExpression();

        qDebug() << "[UI] Updating preview and setting audio lambda...";
        updateNightlyPreview();


        qDebug() << "[UI] Starting engine...";
        m_ghostSynth->start();

    } else {
        btnPlayNightly->setText("▶ Play Edited PCM");
        btnPlayNightly->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 35px;");
        m_ghostSynth->setAudioSource([](double){ return 0.0; });
        m_ghostSynth->stop();
    }
}



void PCMEditorTab::onTrimClicked() {
    if (selectionStartPixel == selectionEndPixel || m_nightlyBuffer.empty()) return;
    int p1 = pixelToIndex(selectionStartPixel);
    int p2 = pixelToIndex(selectionEndPixel);
    int startIdx = std::min(p1, p2);
    int endIdx = std::max(p1, p2);
    if (startIdx >= endIdx) return;

    std::vector<double> newData(m_nightlyBuffer.begin() + startIdx, m_nightlyBuffer.begin() + endIdx);
    m_nightlyBuffer = std::move(newData);

    selectionStartPixel = selectionEndPixel = -1; // Clear selection
    updateNightlyPreview();
    generateNightlyExpression();
    update(); // Redraw canvas
}

void PCMEditorTab::onPingPongClicked() {
    if (selectionStartPixel == selectionEndPixel || m_nightlyBuffer.empty()) return;
    int p1 = pixelToIndex(selectionStartPixel);
    int p2 = pixelToIndex(selectionEndPixel);
    int startIdx = std::min(p1, p2);
    int endIdx = std::max(p1, p2);
    if (startIdx >= endIdx) return;


    std::vector<double> loopSegment(m_nightlyBuffer.begin() + startIdx, m_nightlyBuffer.begin() + endIdx);
    std::vector<double> reversedSegment = loopSegment;
    std::reverse(reversedSegment.begin(), reversedSegment.end());
    m_nightlyBuffer.insert(m_nightlyBuffer.begin() + endIdx, reversedSegment.begin(), reversedSegment.end());

    updateNightlyPreview();
    generateNightlyExpression();
    update();
}

void PCMEditorTab::onReverseClicked() {
    if (m_nightlyBuffer.empty()) return;
    int p1 = 0, p2 = m_nightlyBuffer.size() - 1;


    if (selectionStartPixel != selectionEndPixel && selectionStartPixel != -1) {
        p1 = pixelToIndex(std::min(selectionStartPixel, selectionEndPixel));
        p2 = pixelToIndex(std::max(selectionStartPixel, selectionEndPixel));
    }

    std::reverse(m_nightlyBuffer.begin() + p1, m_nightlyBuffer.begin() + p2 + 1);

    updateNightlyPreview();
    generateNightlyExpression();
    update();
}

void PCMEditorTab::onNormalizeClicked() {
    if (m_nightlyBuffer.empty()) return;
    double maxVal = 0.0;
    for (double val : m_nightlyBuffer) {
        if (std::abs(val) > maxVal) maxVal = std::abs(val);
    }
    if (maxVal > 0.0) {
        double mult = 1.0 / maxVal;
        for (double& val : m_nightlyBuffer) val *= mult;
    }
    updateNightlyPreview();
    generateNightlyExpression();
    update();
}

void PCMEditorTab::onDetectPitchClicked() {
    if (m_nightlyBuffer.empty()) return;
    int p1 = 0, p2 = m_nightlyBuffer.size() - 1;
    if (selectionStartPixel != selectionEndPixel && selectionStartPixel != -1) {
        p1 = pixelToIndex(std::min(selectionStartPixel, selectionEndPixel));
        p2 = pixelToIndex(std::max(selectionStartPixel, selectionEndPixel));
    }

    int n = p2 - p1;
    if (n < 100) return;


    std::vector<double> correlation(n, 0.0);
    for (int lag = 0; lag < n / 2; ++lag) {
        for (int i = 0; i < n - lag; ++i) {
            correlation[lag] += m_nightlyBuffer[p1 + i] * m_nightlyBuffer[p1 + i + lag];
        }
    }

    int peakIndex = -1;
    double maxVal = 0.0;
    bool foundDip = false;
    for (int i = 1; i < n / 2; ++i) {
        if (correlation[i] < correlation[i - 1]) foundDip = true;
        if (foundDip && correlation[i] > maxVal) {
            maxVal = correlation[i];
            peakIndex = i;
        }
    }

    double hz = (peakIndex > 0) ? (m_nightlySampleRate / static_cast<double>(peakIndex)) : 0.0;


    nightlyPcmOutput->setText(QString("Detected Pitch: %1 Hz").arg(hz, 0, 'f', 2));
}

void PCMEditorTab::loadPCMExpression(const std::vector<float>& newData) {

    m_nightlyBuffer.assign(newData.begin(), newData.end());
    updateNightlyPreview();
    update();
}

void PCMEditorTab::onTimeStretchClicked() {
    if (m_nightlyBuffer.empty()) return;


    bool ok;
    double factor = QInputDialog::getDouble(this, "Time Stretch",
                                            "Enter stretch factor (e.g., 2.0 = Double Length, 0.5 = Half Length):",
                                            1.5, 0.1, 10.0, 2, &ok);
    if (!ok) return; // User clicked cancel


    int grainSize = static_cast<int>(m_nightlySampleRate * 0.05);
    if (grainSize % 2 != 0) grainSize++; // Keep it an even number
    if (grainSize > m_nightlyBuffer.size()) grainSize = m_nightlyBuffer.size();

    int halfGrain = grainSize / 2;
    double inHop = halfGrain / factor; // How fast we move through the input
    int outHop = halfGrain;            // How fast we move through the output

    std::vector<double> outBuffer;

    outBuffer.resize(static_cast<size_t>(m_nightlyBuffer.size() * factor) + grainSize, 0.0);

    std::vector<double> window(grainSize);
    for (int i = 0; i < grainSize; ++i) {
        window[i] = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (grainSize - 1)));
    }

    double inPos = 0.0;
    int outPos = 0;


    while (inPos + grainSize < m_nightlyBuffer.size()) {
        int inIdx = static_cast<int>(inPos);
        for (int i = 0; i < grainSize; ++i) {
            if (outPos + i < outBuffer.size()) {
                outBuffer[outPos + i] += m_nightlyBuffer[inIdx + i] * window[i];
            }
        }
        inPos += inHop;
        outPos += outHop;
    }


    double maxVal = 0.0;
    for (double val : outBuffer) if (std::abs(val) > maxVal) maxVal = std::abs(val);
    if (maxVal > 0.0) {
        for (double& val : outBuffer) val /= maxVal;
    }


    m_nightlyBuffer = std::move(outBuffer);


    updateNightlyPreview();
    generateNightlyExpression();
    update();
}


void PCMEditorTab::onChopClicked() {

    if (selectionStartPixel == selectionEndPixel || m_nightlyBuffer.empty()) return;

    int p1 = pixelToIndex(selectionStartPixel);
    int p2 = pixelToIndex(selectionEndPixel);
    int startIdx = std::min(p1, p2);
    int endIdx = std::max(p1, p2);

    if (startIdx >= endIdx) return;


    m_nightlyBuffer.erase(m_nightlyBuffer.begin() + startIdx, m_nightlyBuffer.begin() + endIdx);

    selectionStartPixel = selectionEndPixel = -1; // Clear selection
    updateNightlyPreview();
    generateNightlyExpression();
    update();
}

void PCMEditorTab::onFadeInClicked() {
    if (m_nightlyBuffer.empty()) return;

    int p1 = 0, p2 = m_nightlyBuffer.size() - 1;
    if (selectionStartPixel != selectionEndPixel && selectionStartPixel != -1) {
        p1 = pixelToIndex(std::min(selectionStartPixel, selectionEndPixel));
        p2 = pixelToIndex(std::max(selectionStartPixel, selectionEndPixel));
    }

    int length = p2 - p1;
    if (length <= 0) return;

    for (int i = 0; i <= length; ++i) {
        double multiplier = static_cast<double>(i) / length; // Ramps from 0.0 to 1.0
        m_nightlyBuffer[p1 + i] *= multiplier;
    }

    updateNightlyPreview();
    generateNightlyExpression();
    update();
}

void PCMEditorTab::onFadeOutClicked() {
    if (m_nightlyBuffer.empty()) return;

    int p1 = 0, p2 = m_nightlyBuffer.size() - 1;
    if (selectionStartPixel != selectionEndPixel && selectionStartPixel != -1) {
        p1 = pixelToIndex(std::min(selectionStartPixel, selectionEndPixel));
        p2 = pixelToIndex(std::max(selectionStartPixel, selectionEndPixel));
    }

    int length = p2 - p1;
    if (length <= 0) return;

    for (int i = 0; i <= length; ++i) {
        double multiplier = 1.0 - (static_cast<double>(i) / length); // Ramps from 1.0 to 0.0
        m_nightlyBuffer[p1 + i] *= multiplier;
    }

    updateNightlyPreview();
    generateNightlyExpression();
    update();
}

void PCMEditorTab::onAkaiGrimeClicked() {

    if (m_nightlyBuffer.empty()) return;

    int p1 = 0, p2 = m_nightlyBuffer.size() - 1;
    if (selectionStartPixel != selectionEndPixel && selectionStartPixel != -1) {
        p1 = pixelToIndex(std::min(selectionStartPixel, selectionEndPixel));
        p2 = pixelToIndex(std::max(selectionStartPixel, selectionEndPixel));
    }

    double bitDepth = 4096.0;

    for (int i = p1; i <= p2; ++i) {

        if (i % 3 == 0 && i > 0) {
            m_nightlyBuffer[i] = m_nightlyBuffer[i - 1];
        } else {

            m_nightlyBuffer[i] = std::floor(m_nightlyBuffer[i] * bitDepth) / bitDepth;
        }
    }

    updateNightlyPreview();
    generateNightlyExpression();
    update();
}

QString PCMEditorTab::generateLegacyPCM(const std::vector<double>& q, double sr) {
    int N = q.size();
    if (N == 0) return "0";


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
