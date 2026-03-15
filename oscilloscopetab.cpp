#include "oscilloscopetab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPainter>
#include <cmath>
#include <algorithm>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QStringList>
#include <QLabel>
#include <QLineEdit>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

OscilloscopePlot::OscilloscopePlot(QWidget *parent) : QWidget(parent) {
    setMinimumSize(400, 300);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void OscilloscopePlot::setParameters(int fx, int fy, float phase) {
    m_fx = fx;
    m_fy = fy;
    m_phase = phase;
    m_useCustom = false;
    update();
}

void OscilloscopePlot::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::black);
    painter.setPen(QPen(QColor(50, 255, 50), 2));

    int width = this->width();
    int height = this->height();
    int cx = width / 2;
    int cy = height / 2;
    int radius = std::min(width, height) / 2 - 10;

    QPointF lastPoint;
    bool first = true;

    if (m_useCustom) {

        if (m_customX.empty() || m_customY.empty()) return;

        int points = std::min(m_customX.size(), m_customY.size());
        for (int i = 0; i < points; ++i) {

            QPointF currentPoint(cx + m_customX[i] * radius, cy - m_customY[i] * radius);

            if (!first) {
                painter.drawLine(lastPoint, currentPoint);
            }
            lastPoint = currentPoint;
            first = false;
        }
    } else {

        int numVisualPoints = 2000;
        float t_max = 2.0f * M_PI * std::max(m_fx, m_fy);

        for (int i = 0; i <= numVisualPoints; ++i) {
            float t = (static_cast<float>(i) / numVisualPoints) * t_max;
            float x = std::sin(m_fx * t);
            float y = std::sin(m_fy * t + m_phase);
            QPointF currentPoint(cx + x * radius, cy - y * radius);

            if (!first) {
                painter.drawLine(lastPoint, currentPoint);
            }
            lastPoint = currentPoint;
            first = false;
        }
    }
}


OscilloscopeTab::OscilloscopeTab(QWidget *parent) : QWidget(parent) {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);


    plotWidget = new OscilloscopePlot(this);
    mainLayout->addWidget(plotWidget, 2);


    QVBoxLayout* controlLayout = new QVBoxLayout();

    presetCombo = new QComboBox(this);
    presetCombo->addItems({"Custom / Manual", "Perfect Circle (Unison)", "Diagonal Line (Unison)",
                           "Parabola (Octave)", "Figure-Eight (Octave)",
                           "3-Lobe Interlace (Perfect 5th)", "4-Lobe Interlace (Perfect 4th)"});
    // REMOVED: "--- ADVANCED SHAPES ---", "Heart", "5-Point Star", "Square", "Spiral"
    controlLayout->addWidget(presetCombo);

    controlLayout->addWidget(new QLabel("String Build Mode:", this));
    buildModeCombo = new QComboBox(this);
    buildModeCombo->addItems({"Nightly (Nested Variables)", "Legacy (Ternary Tree)"});
    controlLayout->addWidget(buildModeCombo);


    QFormLayout* formLayout = new QFormLayout();

    waveformCombo = new QComboBox(this);
    waveformCombo->addItems({"sinew", "trianglew", "saww", "squarew", "Kick Drum", "Snare Drum", "Custom Expression"});
    formLayout->addRow("Waveform:", waveformCombo);

    sharedExprEdit = new QLineEdit(this);
    sharedExprEdit->setPlaceholderText("e.g., sinew(t*100) * 0.5");
    formLayout->addRow("Shared Expr:", sharedExprEdit);

    mixSharedExprCheck = new QCheckBox("Mix Shared Expr (Add to Output)", this);
    formLayout->addRow("", mixSharedExprCheck);

    freqXSpin = new QSpinBox(this);
    freqXSpin->setRange(1, 20); freqXSpin->setValue(3);

    freqYSpin = new QSpinBox(this);
    freqYSpin->setRange(1, 20); freqYSpin->setValue(2);

    phaseStartSlider = new QSlider(Qt::Horizontal, this);
    phaseStartSlider->setRange(0, 360); phaseStartSlider->setValue(0);

    phaseEndSlider = new QSlider(Qt::Horizontal, this);
    phaseEndSlider->setRange(0, 360); phaseEndSlider->setValue(180);

    timeSpin = new QDoubleSpinBox(this);
    timeSpin->setRange(0.1, 20.0); timeSpin->setValue(4.0); timeSpin->setSuffix(" s");

    pingPongCheck = new QCheckBox("Tempo Sync Ping-Pong", this);
    loopSweepCheck = new QCheckBox("Loop Sweep (Math Mode)", this);
    vibrateCheck = new QCheckBox("Add Vector Vibration", this);
    rotateCustomCheck = new QCheckBox("Spin Custom Vector in LMMS", this);
    rotate3dCheck = new QCheckBox("TRUE 3D Spin in LMMS (WARNING: HUGE STRING)", this);

    formLayout->addRow("Freq Ratio X:", freqXSpin);
    formLayout->addRow("Freq Ratio Y:", freqYSpin);
    formLayout->addRow("Phase Start (°):", phaseStartSlider);
    formLayout->addRow("Phase End (°):", phaseEndSlider);
    formLayout->addRow("Sweep Time:", timeSpin);
    formLayout->addRow("Animation:", pingPongCheck);
    formLayout->addRow("", loopSweepCheck);
    formLayout->addRow("Effects:", vibrateCheck);
    formLayout->addRow("", rotateCustomCheck);
    formLayout->addRow("", rotate3dCheck);

    controlLayout->addLayout(formLayout);



    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    controlLayout->addWidget(line);

    controlLayout->addWidget(new QLabel("2D Vector Scale (Zoom):", this));
    csvScaleSlider = new QSlider(Qt::Horizontal, this);
    csvScaleSlider->setRange(1, 200); // 1% to 200%
    csvScaleSlider->setValue(100);
    controlLayout->addWidget(csvScaleSlider);

    connect(csvScaleSlider, &QSlider::valueChanged, this, &OscilloscopeTab::updateCsvScale);


    importBtn = new QPushButton("Import Vectors (CSV)", this);
    controlLayout->addWidget(importBtn);


    controlLayout->addWidget(new QLabel("3D Object Scale (Zoom):", this));
    objectScaleSlider = new QSlider(Qt::Horizontal, this);
    objectScaleSlider->setRange(1, 200); // 1% to 200% scale
    objectScaleSlider->setValue(50);     // Default to 50%
    controlLayout->addWidget(objectScaleSlider);

    importObjBtn = new QPushButton("Import 3D Object (.obj)", this);
    controlLayout->addWidget(importObjBtn);
    connect(importObjBtn, &QPushButton::clicked, this, &OscilloscopeTab::importObj);

    exportWavBtn = new QPushButton("Export 3D Rotation to .WAV", this);
    controlLayout->addWidget(exportWavBtn);
    connect(exportWavBtn, &QPushButton::clicked, this, &OscilloscopeTab::exportWav);



    rotationTimer = new QTimer(this);
    connect(rotationTimer, &QTimer::timeout, this, &OscilloscopeTab::updateRotation);

    QLabel* usageNote = new QLabel(this);
    usageNote->setWordWrap(true);
    usageNote->setTextFormat(Qt::RichText);
    usageNote->setText(
        "<b>Note on Custom Expressions & Mixing:</b><br>"
        "• Selecting 'Custom Expression' as a waveform bypasses the frequency dials.<br>"
        "• Checking 'Mix Shared Expr' with an imported CSV or 3D object "
        "will mix your typed math into the final generated string.<br>"
        "<i style='color: #888;'> (Live screen preview shows basic waves and raw arrays; "
        "typed math only applies to the generated text box).</i>"
        );
    // Optional: Make the font slightly smaller to keep the UI clean
    usageNote->setStyleSheet("font-size: 11px; margin-top: 10px;");
    controlLayout->addWidget(usageNote);

    generateStringBtn = new QPushButton("Generate Xpressive String", this);
    controlLayout->addWidget(generateStringBtn);

    stringOutput = new QTextEdit(this);
    stringOutput->setReadOnly(true);
    stringOutput->setMinimumHeight(150);
    controlLayout->addWidget(stringOutput);

    mainLayout->addLayout(controlLayout, 1);


    connect(presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OscilloscopeTab::loadPreset);
    connect(freqXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &OscilloscopeTab::updateParameters);
    connect(freqYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &OscilloscopeTab::updateParameters);
    connect(phaseStartSlider, &QSlider::valueChanged, this, &OscilloscopeTab::updateParameters);

    connect(generateStringBtn, &QPushButton::clicked, this, &OscilloscopeTab::generateString);
    connect(importBtn, &QPushButton::clicked, this, &OscilloscopeTab::importVectors);

    updateParameters();
}

void OscilloscopeTab::loadPreset(int index) {
    freqXSpin->blockSignals(true); freqYSpin->blockSignals(true); phaseStartSlider->blockSignals(true);

    switch (index) {
    case 1: freqXSpin->setValue(1); freqYSpin->setValue(1); phaseStartSlider->setValue(90); break;
    case 2: freqXSpin->setValue(1); freqYSpin->setValue(1); phaseStartSlider->setValue(0); break;
    case 3: freqXSpin->setValue(1); freqYSpin->setValue(2); phaseStartSlider->setValue(90); break;
    case 4: freqXSpin->setValue(1); freqYSpin->setValue(2); phaseStartSlider->setValue(0); break;
    case 5: freqXSpin->setValue(3); freqYSpin->setValue(2); phaseStartSlider->setValue(90); break;
    case 6: freqXSpin->setValue(3); freqYSpin->setValue(4); phaseStartSlider->setValue(90); break;
    }

    freqXSpin->blockSignals(false); freqYSpin->blockSignals(false); phaseStartSlider->blockSignals(false);
    updateParameters();
}

void OscilloscopeTab::updateParameters() {

    m_isCustomMode = false;


    if (rotationTimer && rotationTimer->isActive()) {
        rotationTimer->stop();
    }

    int fx = freqXSpin->value();
    int fy = freqYSpin->value();
    float phase = phaseStartSlider->value() * M_PI / 180.0f;
    plotWidget->setParameters(fx, fy, phase);
}



void OscilloscopeTab::generateString() {
    QString vibCode = vibrateCheck->isChecked() ? "+ (randv(t * 10000) * 0.02)" : "";

    QString sharedExpr = sharedExprEdit->text().trimmed();
    QString mixStr = (mixSharedExprCheck->isChecked() && !sharedExpr.isEmpty()) ? QString(" + (%1)").arg(sharedExpr) : "";


    if (rotate3dCheck->isChecked() && !m_vertices3D.empty() && !m_drawPath.empty()) {
        bool isNightly = (buildModeCombo->currentIndex() == 0);
        int size = m_drawPath.size();
        float hz = 50.0f;
        float scale = objectScaleSlider->value() / 100.0f;


        std::vector<float> statX(size), statY(size), statZ(size);
        for (int i = 0; i < size; ++i) {
            Vec3 v = m_vertices3D[m_drawPath[i]];
            statX[i] = v.x * scale;
            statY[i] = v.y * scale;
            statZ[i] = v.z * scale;
        }

        QString treeX, treeY, treeZ;

        if (isNightly) {
            std::function<QString(int, int, const std::vector<float>&)> buildNightly = [&](int start, int end, const std::vector<float>& arr) -> QString {
                if (start == end) return QString::number(arr[start], 'f', 3);
                int mid = start + (end - start) / 2;
                return QString("((s <= %1) ? (%2) : (%3))").arg(mid).arg(buildNightly(start, mid, arr)).arg(buildNightly(mid + 1, end, arr));
            };

            QString sDecl = QString("var s := floor(mod(t * %1, %2));").arg(hz * size).arg(size);
            treeX = QString("(%1 %2)").arg(sDecl, buildNightly(0, size - 1, statX));
            treeY = QString("(%1 %2)").arg(sDecl, buildNightly(0, size - 1, statY));
            treeZ = QString("(%1 %2)").arg(sDecl, buildNightly(0, size - 1, statZ));
        } else {
            float totalTime = 1.0f / hz;
            std::function<QString(int, int, const std::vector<float>&)> buildLegacy = [&](int start, int end, const std::vector<float>& arr) -> QString {
                if (start == end) return QString::number(arr[start], 'f', 3);
                int mid = start + (end - start) / 2;
                double midTime = (double)(mid + 1) / (hz * size);
                return QString("(mod(t, %1) < %2 ? %3 : %4)")
                    .arg(totalTime, 0, 'f', 6).arg(midTime, 0, 'f', 6)
                    .arg(buildLegacy(start, mid, arr)).arg(buildLegacy(mid + 1, end, arr));
            };
            treeX = buildLegacy(0, size - 1, statX);
            treeY = buildLegacy(0, size - 1, statY);
            treeZ = buildLegacy(0, size - 1, statZ);
        }


        QString angleY = "(t * 0.5)";
        QString angleX = "(t * 0.25)";


        QString X1 = QString("((%1) * cos(%3) - (%2) * sin(%3))").arg(treeX, treeZ, angleY);

        QString Z1 = QString("((%1) * sin(%3) + (%2) * cos(%3))").arg(treeX, treeZ, angleY);


        QString finalX = QString("%1 %2%3").arg(X1, vibCode, mixStr).trimmed();
        QString finalY = QString("((%1) * cos(%3) - (%2) * sin(%3)) %4%5").arg(treeY, Z1, angleX, vibCode, mixStr).trimmed();

        stringOutput->setText(QString("--- TRUE 3D DATA (%1 MODE) ---\nO1: %2\n\nO2: %3")
                                  .arg(isNightly ? "NIGHTLY" : "LEGACY")
                                  .arg(finalX, finalY));
        return;
    }

    else if (m_isCustomMode && !m_activeX.empty() && !m_activeY.empty()) {
        bool isNightly = (buildModeCombo->currentIndex() == 0);
        int size = std::min(m_activeX.size(), m_activeY.size());

        float hz = 50.0f;
        float totalTime = 1.0f / hz;

        QString treeX, treeY;

        if (isNightly) {
            std::function<QString(int, int, const std::vector<float>&)> buildNightly = [&](int start, int end, const std::vector<float>& arr) -> QString {
                if (start == end) return QString::number(arr[start], 'f', 3);
                int mid = start + (end - start) / 2;
                return QString("((s <= %1) ? (%2) : (%3))").arg(mid).arg(buildNightly(start, mid, arr)).arg(buildNightly(mid + 1, end, arr));
            };
            treeX = QString("(var s := floor(mod(t * %1, %2)); %3)").arg(hz * size).arg(size).arg(buildNightly(0, size - 1, m_activeX));
            treeY = QString("(var s := floor(mod(t * %1, %2)); %3)").arg(hz * size).arg(size).arg(buildNightly(0, size - 1, m_activeY));
        } else {
            std::function<QString(int, int, const std::vector<float>&)> buildLegacy = [&](int start, int end, const std::vector<float>& arr) -> QString {
                if (start == end) return QString::number(arr[start], 'f', 3);
                int mid = start + (end - start) / 2;
                double midTime = (double)(mid + 1) / (hz * size);
                return QString("(mod(t, %1) < %2 ? %3 : %4)")
                    .arg(totalTime, 0, 'f', 6).arg(midTime, 0, 'f', 6)
                    .arg(buildLegacy(start, mid, arr)).arg(buildLegacy(mid + 1, end, arr));
            };
            treeX = buildLegacy(0, size - 1, m_activeX);
            treeY = buildLegacy(0, size - 1, m_activeY);
        }

        QString finalX, finalY;


        if (rotateCustomCheck->isChecked()) {
            QString angle = "(t * 0.5)";
            finalX = QString("((%1) * cos(%3) - (%2) * sin(%3)) %4%5").arg(treeX, treeY, angle, vibCode, mixStr).trimmed();
            finalY = QString("((%1) * sin(%3) + (%2) * cos(%3)) %4%5").arg(treeX, treeY, angle, vibCode, mixStr).trimmed();
        } else {
            finalX = QString("%1 %2%3").arg(treeX, vibCode, mixStr).trimmed();
            finalY = QString("%1 %2%3").arg(treeY, vibCode, mixStr).trimmed();
        }

        stringOutput->setText(QString("--- CUSTOM DATA (%1 MODE) ---\nO1: %2\n\nO2: %3")
                                  .arg(isNightly ? "NIGHTLY" : "LEGACY")
                                  .arg(finalX, finalY));
        return;
    }


    QString wave = waveformCombo->currentText();
    int fx = freqXSpin->value();
    int fy = freqYSpin->value();

    float pStart = phaseStartSlider->value() / 360.0f;
    float pEnd = phaseEndSlider->value() / 360.0f;
    float time = timeSpin->value();

    QString exprX, exprY;


    QString sweepLogic;
    if (pingPongCheck->isChecked()) {
        sweepLogic = "((trianglew(t * (tempo / 120.0)) + 1.0) / 2.0)";
    } else if (loopSweepCheck->isChecked()) {
        sweepLogic = QString("(mod(t, %1) / %1)").arg(time);
    } else {
        sweepLogic = QString("min(t / %1, 1.0)").arg(time);
    }


    if (wave == "Custom Expression") {
        QString base = sharedExpr.isEmpty() ? "0" : sharedExpr;
        exprX = QString("(%1) %2%3").arg(base, vibCode, mixStr).trimmed();
        exprY = QString("(%1) %2%3").arg(base, vibCode, mixStr).trimmed();
    } else if (wave == "Kick Drum") {
        exprX = QString("sinew(integrate(f * %1.0 * (1 + 6 * exp(-t * 100))) + (%2 + (%3 - %2) * %4)) * exp(-t * 15) %5%6")
        .arg(fx).arg(pStart).arg(pEnd).arg(sweepLogic).arg(vibCode).arg(mixStr).trimmed();
        exprY = QString("sinew(integrate(f * %1.0 * (1 + 6 * exp(-t * 100)))) * exp(-t * 15) %2%3")
                    .arg(fy).arg(vibCode).arg(mixStr).trimmed();
    } else if (wave == "Snare Drum") {
        exprX = QString("((randsv(t*srate,0)* exp(-t * 50)) + (sinew(integrate(%1.0 * 200 * (1 + 2 * exp(-t * 120))) + (%2 + (%3 - %2) * %4)) * exp(-t * 60))) %5%6")
        .arg(fx).arg(pStart).arg(pEnd).arg(sweepLogic).arg(vibCode).arg(mixStr).trimmed();
        exprY = QString("((randsv(t*srate,0)* exp(-t * 50)) + (sinew(integrate(%1.0 * 200 * (1 + 2 * exp(-t * 120)))) * exp(-t * 60))) %2%3")
                    .arg(fy).arg(vibCode).arg(mixStr).trimmed();
    } else {
        exprX = QString("%1(integrate(f * %2.0) + (%3 + (%4 - %3) * %5)) %6%7")
        .arg(wave).arg(fx).arg(pStart).arg(pEnd).arg(sweepLogic).arg(vibCode).arg(mixStr).trimmed();
        exprY = QString("%1(integrate(f * %2.0)) %3%4").arg(wave).arg(fy).arg(vibCode).arg(mixStr).trimmed();
    }
    QString finalText = QString("--- XPRESSIVE SETUP NOTES ---\n"
                                "PN1 -> Pan Hard Left (100% L)\n"
                                "PN2 -> Pan Hard Right (100% R)\n"
                                "-----------------------------\n\n"
                                "O1 (X-Axis Left):\n%1\n\n"
                                "O2 (Y-Axis Right):\n%2")
                            .arg(exprX).arg(exprY);

    stringOutput->setText(finalText);
}

void OscilloscopeTab::importVectors() {

    QString fileName = QFileDialog::getOpenFileName(this, "Open Vector Data", "", "CSV Files (*.csv);;All Files (*)");

    if (fileName.isEmpty()) {
        return; // User canceled the dialog
    }


    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file for reading.");
        return;
    }

    std::vector<float> importedX;
    std::vector<float> importedY;
    QTextStream in(&file);


    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();


        if (line.isEmpty()) {
            continue;
        }


        QStringList values = line.split(',');

        if (values.size() >= 2) {
            bool okX, okY;
            float xVal = values[0].toFloat(&okX);
            float yVal = values[1].toFloat(&okY);


            if (okX && okY) {
                importedX.push_back(xVal);
                importedY.push_back(yVal);
            }
        }
    }
    file.close();


    if (importedX.empty() || importedY.empty()) {
        QMessageBox::warning(this, "Error", "No valid vector data found in the CSV. Ensure it has at least two columns of numbers separated by a comma.");
        return;
    }


    m_originalX = importedX;
    m_originalY = importedY;
    m_isCustomMode = true;


    updateCsvScale();


    emit timbreGenerated(importedX, importedY);
}


void OscilloscopePlot::setCustomVectors(const std::vector<float>& x, const std::vector<float>& y) {
    m_customX = x;
    m_customY = y;
    m_useCustom = true;
    update();
}
void OscilloscopeTab::importObj() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open 3D Model", "", "OBJ Files (*.obj);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open OBJ file.");
        return;
    }

    m_vertices3D.clear();
    m_drawPath.clear();
    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));


        if (parts[0] == "v" && parts.size() >= 4) {
            Vec3 v;
            v.x = parts[1].toFloat();
            v.y = parts[2].toFloat();
            v.z = parts[3].toFloat();
            m_vertices3D.push_back(v);
        }

        else if (parts[0] == "f" && parts.size() >= 4) {

            int v1 = parts[1].split('/')[0].toInt() - 1;
            int v2 = parts[2].split('/')[0].toInt() - 1;
            int v3 = parts[3].split('/')[0].toInt() - 1;


            m_drawPath.push_back(v1);
            m_drawPath.push_back(v2);
            m_drawPath.push_back(v3);
            m_drawPath.push_back(v1);
        }
    }
    file.close();

    if (!m_vertices3D.empty()) {
        float centerX = 0, centerY = 0, centerZ = 0;
        for (const Vec3& v : m_vertices3D) {
            centerX += v.x; centerY += v.y; centerZ += v.z;
        }
        centerX /= m_vertices3D.size();
        centerY /= m_vertices3D.size();
        centerZ /= m_vertices3D.size();

        for (Vec3& v : m_vertices3D) {
            v.x -= centerX; v.y -= centerY; v.z -= centerZ;
        }
    }
    if (!m_vertices3D.empty() && !m_drawPath.empty()) {
        m_angleX = 0.0f;
        m_angleY = 0.0f;
        m_isCustomMode = true;
        rotationTimer->start(16);
    }
}


void OscilloscopeTab::updateRotation() {
    if (m_vertices3D.empty() || m_drawPath.empty()) return;


    m_angleY += 0.03f;
    m_angleX += 0.015f;

    std::vector<float> frameX;
    std::vector<float> frameY;


    float scale = objectScaleSlider->value() / 100.0f;


    for (int index : m_drawPath) {
        if (index < 0 || index >= m_vertices3D.size()) continue;

        Vec3 v = m_vertices3D[index];


        float x1 = v.x * std::cos(m_angleY) - v.z * std::sin(m_angleY);
        float z1 = v.x * std::sin(m_angleY) + v.z * std::cos(m_angleY);
        float y1 = v.y;


        float y2 = y1 * std::cos(m_angleX) - z1 * std::sin(m_angleX);
        float z2 = y1 * std::sin(m_angleX) + z1 * std::cos(m_angleX);
        float x2 = x1;


        frameX.push_back(x2 * scale);
        frameY.push_back(y2 * scale);
    }


    m_activeX = frameX;
    m_activeY = frameY;
    plotWidget->setCustomVectors(frameX, frameY);


}


void OscilloscopeTab::exportWav() {
    if (m_vertices3D.empty() || m_drawPath.empty()) {
        QMessageBox::warning(this, "Error", "Load a 3D object first!");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save 3D Audio", "", "WAV Files (*.wav)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) return;

    int sampleRate = 44100;
    int durationSecs = 5; // Record 5 seconds of rotation
    int numSamples = sampleRate * durationSecs;

    float angleY = 0.0f;
    float angleX = 0.0f;
    float scale = 0.5f;


    float drawHz = 50.0f;
    int pointsPerCycle = m_drawPath.size();
    float pointsPerSec = drawHz * pointsPerCycle;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);


    out.writeRawData("RIFF", 4);
    out << (quint32)(36 + (numSamples * 2 * sizeof(float)));
    out.writeRawData("WAVE", 4);
    out.writeRawData("fmt ", 4);
    out << (quint32)16;
    out << (quint16)3;
    out << (quint16)2;
    out << (quint32)sampleRate;
    out << (quint32)(sampleRate * 2 * sizeof(float));
    out << (quint16)(2 * sizeof(float));
    out << (quint16)32;
    out.writeRawData("data", 4);
    out << (quint32)(numSamples * 2 * sizeof(float));


    for (int i = 0; i < numSamples; ++i) {

        angleY += 0.03f / (sampleRate / 60.0f);
        angleX += 0.015f / (sampleRate / 60.0f);

        int pathIndex = static_cast<int>(i * (pointsPerSec / sampleRate)) % pointsPerCycle;
        int vertexIdx = m_drawPath[pathIndex];
        Vec3 v = m_vertices3D[vertexIdx];


        float x1 = v.x * std::cos(angleY) - v.z * std::sin(angleY);
        float z1 = v.x * std::sin(angleY) + v.z * std::cos(angleY);
        float y1 = v.y;
        float y2 = y1 * std::cos(angleX) - z1 * std::sin(angleX);
        float x2 = x1;

        out << (float)(x2 * scale);
        out << (float)(y2 * scale);
    }

    file.close();
    QMessageBox::information(this, "Success", "WAV exported successfully!");
}

void OscilloscopeTab::updateCsvScale() {
    if (m_originalX.empty() || m_originalY.empty()) return;

    float scale = csvScaleSlider->value() / 100.0f;
    m_activeX.resize(m_originalX.size());
    m_activeY.resize(m_originalY.size());


    for (size_t i = 0; i < m_originalX.size(); ++i) {
        m_activeX[i] = m_originalX[i] * scale;
        m_activeY[i] = m_originalY[i] * scale;
    }

    plotWidget->setCustomVectors(m_activeX, m_activeY);


    if (m_isCustomMode) {
        generateString();
    }
}
