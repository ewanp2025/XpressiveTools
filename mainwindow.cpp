#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
#include <functional>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
}

void MainWindow::setupUI() {
    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    auto *title = new QLabel("Xpressive Converter", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 16px; font-weight: bold;");
    layout->addWidget(title);

    modeTabs = new QTabWidget(this);
    QWidget *modernTab = new QWidget();
    QWidget *legacyTab = new QWidget();
    modeTabs->addTab(modernTab, "Modern (Nightly/ExprTk)");
    modeTabs->addTab(legacyTab, "Legacy (Alpha/Ternary)");
    layout->addWidget(modeTabs);

    auto *btnLayout = new QHBoxLayout();
    auto *btnLoad = new QPushButton("Open WAV File", this);
    btnSave = new QPushButton("Save Expression", this);
    btnSave->setEnabled(false);
    btnLayout->addWidget(btnLoad);
    btnLayout->addWidget(btnSave);
    layout->addLayout(btnLayout);

    auto *paramGrid = new QGridLayout();
    paramGrid->addWidget(new QLabel("Max Duration (s):"), 0, 0);
    maxDurSpin = new QDoubleSpinBox();
    maxDurSpin->setRange(0.1, 60.0);
    maxDurSpin->setDecimals(3);
    paramGrid->addWidget(maxDurSpin, 0, 1);

    paramGrid->addWidget(new QLabel("Sample Rate:"), 0, 2);
    sampleRateCombo = new QComboBox();
    sampleRateCombo->addItems({"8000", "4000", "2000"});
    paramGrid->addWidget(sampleRateCombo, 1, 1);
    layout->addLayout(paramGrid);

    auto *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    layout->addWidget(line);

    auto *txtTitle = new QLabel("Expression Upgrader (Alpha TXT -> Nightly TXT)");
    txtTitle->setStyleSheet("font-weight: bold; color: #555;");
    layout->addWidget(txtTitle);

    auto *btnTxtLoad = new QPushButton("Open Alpha .txt and Save as Modern", this);
    layout->addWidget(btnTxtLoad);

    statusBox = new QTextEdit();
    statusBox->setReadOnly(true);
    statusBox->setText("Ready.");
    layout->addWidget(statusBox);

    setCentralWidget(centralWidget);

    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::loadWav);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::saveExpr);
    connect(btnTxtLoad, &QPushButton::clicked, this, &MainWindow::convertTxt);
}

void MainWindow::loadWav() {
    QString path = QFileDialog::getOpenFileName(this, "Select WAV", "", "WAV (*.wav)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        currentFileName = QFileInfo(path).fileName();
        file.seek(24);
        file.read(reinterpret_cast<char*>(&fileFs), 4);
        file.seek(44);
        QByteArray raw = file.readAll();
        const int16_t* samples = reinterpret_cast<const int16_t*>(raw.data());
        int count = raw.size() / sizeof(int16_t);
        originalData.clear();
        for(int i=0; i<count; ++i) originalData.push_back(samples[i] / 32768.0);

        double duration = (double)count / fileFs;
        maxDurSpin->setValue(duration);
        statusBox->setText(QString("Loaded: %1\nRate: %2 Hz\nLength: %3s").arg(currentFileName).arg(fileFs).arg(duration, 0, 'f', 3));
        btnSave->setEnabled(true);
    }
}

void MainWindow::convertTxt() {
    QString path = QFileDialog::getOpenFileName(this, "Select Alpha TXT", "", "Text Files (*.txt)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    QString content = QString::fromUtf8(file.readAll());

    // 1. Extract audio samples (e.g. -0.950)
    QRegularExpression re("-?\\d\\.\\d{3}");
    QRegularExpressionMatchIterator i = re.globalMatch(content);
    std::vector<double> extractedSamples;
    while (i.hasNext()) {
        extractedSamples.push_back(i.next().captured().toDouble());
    }

    if (extractedSamples.size() < 2) {
        statusBox->setText("Error: File too small or invalid format.");
        return;
    }

    //  Sample Rate Detection
    // Find all time markers (t < 0.123456)
    QRegularExpression timeRe("t<(\\d+\\.\\d+)");
    QRegularExpressionMatchIterator ti = timeRe.globalMatch(content);

    double detectedSr = 8000; // Default fallback
    if (ti.hasNext()) {
        double t1 = ti.next().captured(1).toDouble();
        if (ti.hasNext()) {
            double t2 = ti.next().captured(1).toDouble();
            double diff = std::abs(t2 - t1);
            if (diff > 0) {
                detectedSr = std::round(1.0 / diff);
            }
        }
    }

    // 3. Constrain to allowed rates to prevent "16 Hz" errors
    if (detectedSr > 6000) detectedSr = 8000;
    else if (detectedSr > 3000) detectedSr = 4000;
    else detectedSr = 2000;

    QString modernExpr = generateModernExpression(extractedSamples, detectedSr);
    QString savePath = QFileDialog::getSaveFileName(this, "Save Modern Expression", path.replace(".txt", "_Modern.txt"), "Text Files (*.txt)");

    if (!savePath.isEmpty()) {
        QFile outFile(savePath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(modernExpr.toUtf8());
            statusBox->append(QString("\nSuccess: Converted at %1 Hz").arg(QString::number(detectedSr)));
        }
    }
}

QString MainWindow::generateModernExpression(const std::vector<double>& q, double sr) {
    int N = q.size();
    if (N == 0) return "0";
    QString header = QString("var s := floor(t * %1);\n").arg(sr);

    std::function<QString(int, int)> buildTree = [&](int start, int end) -> QString {
        if (start == end) return QString::number(q[start], 'f', 3);
        int mid = start + (end - start) / 2;
        return QString("((s <= %1) ? (%2) : (%3))").arg(QString::number(mid), buildTree(start, mid), buildTree(mid + 1, end));
    };

    return header + buildTree(0, N - 1);
}

QString MainWindow::generateLegacyExpression(const std::vector<double>& q, double sr) {
    int blockSize = 128;
    int N = q.size();
    QStringList blocks;
    for (int b = 0; b < std::ceil((double)N / blockSize); ++b) {
        int start = b * blockSize;
        int end = std::min((b + 1) * blockSize, N);
        QString seg = "";
        for (int i = start; i < end; ++i) {
            seg += QString("(t<%1?%2:").arg(QString::number(i/sr, 'f', 6), QString::number(q[i], 'f', 3));
        }
        seg += "0" + QString(")").repeated(end - start);
        blocks.append(seg);
    }
    QString expr = blocks.last();
    for (int k = blocks.size() - 2; k >= 0; --k) {
        double T = (double)((k + 1) * blockSize) / sr;
        expr = QString("(t<%1?%2:%3)").arg(QString::number(T, 'f', 6), blocks[k], expr);
    }
    return expr;
}

void MainWindow::saveExpr() {
    if (originalData.empty()) return;
    double targetFs = sampleRateCombo->currentText().toDouble();
    int maxSamples = std::min((int)originalData.size(), (int)(maxDurSpin->value() * targetFs));

    std::vector<double> processed;
    double step = (double)fileFs / targetFs;
    for(int i=0; i < maxSamples; ++i) {
        double d = originalData[int(i*step)];
        d = std::round((d + 1.0) * 0.5 * 15.0);
        d = ((d / 15.0) * 2.0 - 1.0) * 0.95;
        processed.push_back(d);
    }

    QString finalExpr = (modeTabs->currentIndex() == 0) ? generateModernExpression(processed, targetFs) : generateLegacyExpression(processed, targetFs);

    QString savePath = QFileDialog::getSaveFileName(this, "Save Expression", currentFileName.replace(".wav", ".txt"), "Text Files (*.txt)");
    if (!savePath.isEmpty()) {
        QFile outFile(savePath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(finalExpr.toUtf8());
            statusBox->append("\nExport Complete.");
        }
    }
}
