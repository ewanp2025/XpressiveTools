#include "SymbolicRegressionTab.h"
#include "MicroGP.h"
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QClipboard>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QDebug>
#include <QSlider>
#include <QSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <vector>
#include <memory>
#include <random>
#include <limits>
#include <algorithm>
#include <numeric>
#include <cmath>

SymbolicRegressionTab::SymbolicRegressionTab(SynthEngine* synth, QWidget* parent)
    : QWidget(parent), m_ghostSynth(synth)
{
    setupUi();
    loadSeedsFromFile();
}

void SymbolicRegressionTab::loadSeedsFromFile() {
    m_loadedSeeds.clear();
    QFile file("learned_seeds.txt");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith("//")) {
                m_loadedSeeds.append(line);
            }
        }
        file.close();
        m_lblStatus->setText(QString("Database Active: Loaded %1 seeds.").arg(m_loadedSeeds.size()));
    } else {
        m_lblStatus->setText("No learned_seeds.txt found. Starting fresh.");
    }
}

void SymbolicRegressionTab::setupUi()
{
    m_layout = new QVBoxLayout(this);

    m_scope = new UniversalScope(this);
    m_scope->setMinimumHeight(180);
    m_layout->addWidget(m_scope);

    auto loadLayout = new QHBoxLayout();
    m_btnLoad = new QPushButton("Load WAV", this);
    m_txtPath = new QLineEdit(this);
    m_txtPath->setReadOnly(true);
    loadLayout->addWidget(m_btnLoad);
    loadLayout->addWidget(m_txtPath);
    m_layout->addLayout(loadLayout);

    m_cmbDownsample = new QComboBox(this);
    m_cmbDownsample->addItems({"Original", "8000", "4000", "2000", "1000"});
    m_layout->addWidget(new QLabel("Downsample to (Hz) [Lower = faster]:", this));
    m_layout->addWidget(m_cmbDownsample);

    m_chkDechord = new QCheckBox("Dechord mode (bias towards harmonic relationships)", this);
    m_layout->addWidget(m_chkDechord);

    m_cmbSyntax = new QComboBox(this);
    m_cmbSyntax->addItems({"Nightly (ExprTk)", "Legacy"});
    m_layout->addWidget(new QLabel("Output syntax:", this));
    m_layout->addWidget(m_cmbSyntax);

    m_btnDiscover = new QPushButton("🔍 Discover Expression", this);
    m_btnDiscover->setStyleSheet("font-size: 16px; padding: 12px; background-color: #0066aa; color: white; font-weight: bold;");
    m_layout->addWidget(m_btnDiscover);

    m_lblStatus = new QLabel("Ready — load a short stab or percussion sample", this);
    m_layout->addWidget(m_lblStatus);

    auto *resultsLayout = new QHBoxLayout();


    auto *mathLayout = new QVBoxLayout();
    m_txtExpression = new QTextEdit(this);
    m_txtExpression->setReadOnly(true);
    mathLayout->addWidget(new QLabel("1. LMMS Math (Paste to Xpressive):", this));
    mathLayout->addWidget(m_txtExpression);


    auto *dnaLayout = new QVBoxLayout();
    m_txtDNAOutput = new QTextEdit(this);
    m_txtDNAOutput->setReadOnly(true);
    m_txtDNAOutput->setStyleSheet("color: #ff9900; background-color: #222;");
    dnaLayout->addWidget(new QLabel("2. Sound DNA (Paste into Learner below):", this));
    dnaLayout->addWidget(m_txtDNAOutput);

    resultsLayout->addLayout(mathLayout);
    resultsLayout->addLayout(dnaLayout);
    m_layout->addLayout(resultsLayout);

    auto *btnRowResult = new QHBoxLayout();
    m_btnCopy = new QPushButton("📋 Copy Math to Clipboard", this);
    m_btnPlay = new QPushButton("▶ Play Discovered Audio", this);
    m_btnPlay->setCheckable(true);
    btnRowResult->addWidget(m_btnCopy);
    btnRowResult->addWidget(m_btnPlay);
    m_layout->addLayout(btnRowResult);

    auto *gpGroup = new QGroupBox("Micro-GP Settings");
    auto *gpForm = new QFormLayout(gpGroup);

    m_popSizeSpin = new QSpinBox(this);
    m_popSizeSpin->setRange(50, 2000);
    m_popSizeSpin->setValue(250);
    m_popSizeSpin->setSingleStep(50);

    m_genSpin = new QSpinBox(this);
    m_genSpin->setRange(10, 300);
    m_genSpin->setValue(40);

    m_maxDepthSpin = new QSpinBox(this);
    m_maxDepthSpin->setRange(8, 512);
    m_maxDepthSpin->setValue(14);

    gpForm->addRow("Population Size:", m_popSizeSpin);
    gpForm->addRow("Generations:", m_genSpin);
    gpForm->addRow("Max Tree Depth:", m_maxDepthSpin);
    m_layout->addWidget(gpGroup);


    m_learnerGroup = new QGroupBox("🤖 The Database Box");
    auto* learnerLayout = new QVBoxLayout(m_learnerGroup);

    auto* lbl1 = new QLabel("Paste 'Sound DNA' here (one per line) to expand the database:");
    m_txtLearnerInput = new QTextEdit();
    m_txtLearnerInput->setMinimumHeight(80);
    m_txtLearnerInput->setPlaceholderText("Paste DNA codes here...");

    m_btnSaveLearned = new QPushButton("💾 Save & Append to Database (learned_seeds.txt)");
    m_btnSaveLearned->setStyleSheet("font-weight: bold; padding: 8px; background-color: #006600; color: white;");

    learnerLayout->addWidget(lbl1);
    learnerLayout->addWidget(m_txtLearnerInput);
    learnerLayout->addWidget(m_btnSaveLearned);
    m_layout->addWidget(m_learnerGroup);


    connect(m_btnLoad, &QPushButton::clicked, this, &SymbolicRegressionTab::onLoadWavClicked);
    connect(m_btnDiscover, &QPushButton::clicked, this, &SymbolicRegressionTab::onDiscoverClicked);
    connect(m_btnCopy, &QPushButton::clicked, this, &SymbolicRegressionTab::onCopyClicked);
    connect(m_btnSaveLearned, &QPushButton::clicked, this, &SymbolicRegressionTab::onSaveLearnedClicked);

    connect(m_btnPlay, &QPushButton::toggled, [this](bool checked) {
        if (!m_ghostSynth) return;
        if (checked && m_bestTree) {
            auto treePtr = std::shared_ptr<GPNode>(m_bestTree->clone().release());
            m_ghostSynth->setAudioSource([treePtr](double t) -> double {
                return treePtr->evaluate(t, 523.25);
            });
            m_ghostSynth->start();
            m_btnPlay->setText("⏹ Stop Audio");
            m_btnPlay->setStyleSheet("background-color: #aa0000; color: white;");
        } else {
            m_ghostSynth->stop();
            m_btnPlay->setText("▶ Play Discovered Audio");
            m_btnPlay->setStyleSheet("");
        }
    });
}

bool SymbolicRegressionTab::loadWavToMemory(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    char riff[4], wave[4];
    file.read(riff, 4);
    file.seek(8);
    file.read(wave, 4);

    if (strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) return false;

    uint16_t audioFormat = 0, numChannels = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0, dataSize = 0;
    qint64 dataOffset = 0;

    while (!file.atEnd()) {
        char chunkID[4];
        if (file.read(chunkID, 4) < 4) break;
        uint32_t chunkSize;
        stream >> chunkSize;
        qint64 nextChunk = file.pos() + chunkSize;
        if (chunkSize % 2 != 0) nextChunk++;

        if (strncmp(chunkID, "fmt ", 4) == 0) {
            stream >> audioFormat;
            stream >> numChannels;
            stream >> sampleRate;
            file.seek(file.pos() + 6);
            stream >> bitsPerSample;
        } else if (strncmp(chunkID, "data", 4) == 0) {
            dataSize = chunkSize;
            dataOffset = file.pos();
            break;
        }
        file.seek(nextChunk);
    }

    if (dataSize == 0) return false;

    m_sampleRate = sampleRate;
    m_audioData.clear();
    file.seek(dataOffset);
    QByteArray raw = file.read(dataSize);

    if (audioFormat == 3 && bitsPerSample == 32) {
        const float* samples = reinterpret_cast<const float*>(raw.data());
        int count = raw.size() / 4;
        for (int i = 0; i < count; i += numChannels) {
            double val = samples[i];
            if (numChannels > 1 && (i+1 < count)) val = (val + samples[i+1]) * 0.5;
            m_audioData.push_back(val);
        }
    } else if (audioFormat == 1) {
        if (bitsPerSample == 16) {
            const int16_t* samples = reinterpret_cast<const int16_t*>(raw.data());
            int count = raw.size() / 2;
            for (int i = 0; i < count; i += numChannels) {
                double val = samples[i] / 32768.0;
                if (numChannels > 1 && (i+1 < count)) val = (val + (samples[i+1] / 32768.0)) * 0.5;
                m_audioData.push_back(val);
            }
        } else if (bitsPerSample == 8) {
            const uint8_t* samples = reinterpret_cast<const uint8_t*>(raw.data());
            int count = raw.size();
            for (int i = 0; i < count; i += numChannels) {
                double val = (samples[i] - 128) / 128.0;
                if (numChannels > 1 && (i+1 < count)) val = (val + ((samples[i+1] - 128) / 128.0)) * 0.5;
                m_audioData.push_back(val);
            }
        } else if (bitsPerSample == 24) {
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(raw.data());
            int count = raw.size() / 3;
            for (int i = 0; i < count; i += numChannels) {
                int byteIdx = i * 3;
                int32_t val32 = (bytes[byteIdx] | (bytes[byteIdx+1] << 8) | (bytes[byteIdx+2] << 16));
                if (val32 & 0x800000) val32 |= 0xFF000000;
                double val = val32 / 8388608.0;
                if (numChannels > 1 && (i+1 < count)) {
                    int b2 = (i+1) * 3;
                    int32_t val32R = (bytes[b2] | (bytes[b2+1] << 8) | (bytes[b2+2] << 16));
                    if (val32R & 0x800000) val32R |= 0xFF000000;
                    val = (val + (val32R / 8388608.0)) * 0.5;
                }
                m_audioData.push_back(val);
            }
        }
    }

    double maxVal = 0.0;
    for (double d : m_audioData) if (std::abs(d) > maxVal) maxVal = std::abs(d);
    if (maxVal > 0.0001) {
        for (double& d : m_audioData) d /= maxVal;
    }

    updateScopePreview(false);
    return true;
}

void SymbolicRegressionTab::updateScopePreview(bool useGenerated)
{
    if (m_audioData.empty()) return;

    double duration = m_audioData.size() / m_sampleRate;

    if (!useGenerated || !m_bestTree) {
        auto preview = [this, duration](double t) -> double {
            int idx = static_cast<int>(t * m_sampleRate);
            if (idx < 0 || idx >= static_cast<int>(m_audioData.size())) return 0.0;
            return m_audioData[idx];
        };
        m_scope->updateScope(preview, duration, 1.0);
    } else {
        auto preview = [tree = m_bestTree.get()](double t) -> double {
            return tree ? tree->evaluate(t, 523.25) : 0.0;
        };
        m_scope->updateScope(preview, duration, 1.0);
    }
}

void SymbolicRegressionTab::onLoadWavClicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Load short audio sample", "", "WAV files (*.wav)");
    if (!file.isEmpty()) {
        if (loadWavToMemory(file)) {
            m_txtPath->setText(file);
            m_lblStatus->setText(QString("WAV loaded! %1 samples @ %2 Hz").arg(m_audioData.size()).arg(m_sampleRate));
        } else {
            QMessageBox::warning(this, "Load Error", "Failed to parse WAV file.");
        }
    }
}

void SymbolicRegressionTab::onDiscoverClicked()
{
    if (m_audioData.empty()) {
        QMessageBox::warning(this, "No data", "Load a WAV first");
        return;
    }

    std::vector<double> processData = m_audioData;
    double processRate = m_sampleRate;

    if (m_cmbDownsample->currentIndex() > 0) {
        double targetRate = m_cmbDownsample->currentText().toDouble();
        if (targetRate > 0 && targetRate < m_sampleRate) {
            int step = std::max(1, (int)std::round(m_sampleRate / targetRate));
            std::vector<double> downsampled;
            for (size_t i = 0; i < m_audioData.size(); i += step) {
                downsampled.push_back(m_audioData[i]);
            }
            processData = downsampled;
            processRate = m_sampleRate / step;
        }
    }

    MicroGP::Config config;
    config.legacySyntax = (m_cmbSyntax->currentText() == "Legacy");
    config.dechord = m_chkDechord->isChecked();
    config.sampleRate = processRate;
    config.populationSize = m_popSizeSpin->value();
    config.generations = m_genSpin->value();
    config.maxDepth = m_maxDepthSpin->value();


    for(const QString& seed : m_loadedSeeds) {
        config.seedStrings.push_back(seed);
    }

    m_btnDiscover->setEnabled(false);
    m_lblStatus->setText(QString("Running Micro-GP (%1 pop, %2 gens)...")
                             .arg(config.populationSize).arg(config.generations));

    if (m_watcher) m_watcher->deleteLater();
    m_watcher = new QFutureWatcher<MicroGP::DiscoveryResult>(this);
    connect(m_watcher, &QFutureWatcher<MicroGP::DiscoveryResult>::finished,
            this, &SymbolicRegressionTab::onProcessFinished);

    QFuture<MicroGP::DiscoveryResult> future = QtConcurrent::run([config, processData]() -> MicroGP::DiscoveryResult {
        MicroGP engine(config);
        return engine.discoverExpression(processData);
    });

    m_watcher->setFuture(future);
}

void SymbolicRegressionTab::onProcessFinished()
{
    m_btnDiscover->setEnabled(true);
    MicroGP::DiscoveryResult res = m_watcher->result();

    QString expr = res.expression.isEmpty() ? "0" : res.expression;
    m_txtExpression->setPlainText(expr);
    m_txtDNAOutput->setPlainText(res.serializedDNA);

    m_lblStatus->setText("Expression discovered!");

    if (res.tree) {
        m_bestTree = std::move(res.tree);
    }

    updateScopePreview(true);
}

void SymbolicRegressionTab::onCopyClicked()
{
    if (!m_txtExpression->toPlainText().isEmpty()) {
        QApplication::clipboard()->setText(m_txtExpression->toPlainText());
        m_lblStatus->setText(" Copied to clipboard");
    }
}

void SymbolicRegressionTab::onSaveLearnedClicked()
{
    QStringList lines = m_txtLearnerInput->toPlainText().split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) return;

    QFile file("learned_seeds.txt");

    int addedCount = 0;
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        for(const QString& line : lines) {
            QString cleanLine = line.trimmed();
            if(!cleanLine.isEmpty()) {
                // Verify it's valid DNA before saving
                QString testStr = cleanLine;
                QTextStream testStream(&testStr);
                auto testTree = GPNode::deserialize(testStream);
                if (testTree) {
                    out << cleanLine << "\n";
                    m_loadedSeeds.append(cleanLine);
                    addedCount++;
                }
            }
        }
        file.close();

        QMessageBox::information(this, "Database Updated",
                                 QString("Successfully validated and appended %1 seeds to learned_seeds.txt!").arg(addedCount));

        m_txtLearnerInput->clear();
        m_lblStatus->setText(QString("Database now holds %1 total seeds.").arg(m_loadedSeeds.size()));
    } else {
        QMessageBox::warning(this, "File Error", "Could not open learned_seeds.txt for writing.");
    }
}
