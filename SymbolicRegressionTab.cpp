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
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QDebug>
#include <QSlider>

#include <vector>
#include <memory>
#include <random>
#include <limits>
#include <algorithm>
#include <numeric>
#include <cmath>

SymbolicRegressionTab::SymbolicRegressionTab(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

void SymbolicRegressionTab::setupUi()
{
    m_layout = new QVBoxLayout(this);

    // === OSCILLOSCOPE (live preview) ===
    m_scope = new UniversalScope(this);
    m_scope->setMinimumHeight(180);
    m_layout->addWidget(m_scope);

    // Load row
    auto loadLayout = new QHBoxLayout();
    m_btnLoad = new QPushButton("Load WAV", this);
    m_txtPath = new QLineEdit(this);
    m_txtPath->setReadOnly(true);
    loadLayout->addWidget(m_btnLoad);
    loadLayout->addWidget(m_txtPath);
    m_layout->addLayout(loadLayout);

    // Options
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

    m_btnDiscover = new QPushButton("🔍 Discover Expression (Native Micro-GP)", this);
    m_btnDiscover->setStyleSheet("font-size: 16px; padding: 12px; background-color: #0066aa; color: white; font-weight: bold;");
    m_layout->addWidget(m_btnDiscover);

    m_lblStatus = new QLabel("Ready — load a short stab or percussion sample", this);
    m_layout->addWidget(m_lblStatus);

    m_txtExpression = new QTextEdit(this);
    m_txtExpression->setReadOnly(true);
    m_layout->addWidget(new QLabel("Discovered expression (copy → Xpressive):", this));
    m_layout->addWidget(m_txtExpression);

    m_btnCopy = new QPushButton("📋 Copy to Clipboard", this);
    m_layout->addWidget(m_btnCopy);

    connect(m_btnLoad, &QPushButton::clicked, this, &SymbolicRegressionTab::onLoadWavClicked);
    connect(m_btnDiscover, &QPushButton::clicked, this, &SymbolicRegressionTab::onDiscoverClicked);
    connect(m_btnCopy, &QPushButton::clicked, this, &SymbolicRegressionTab::onCopyClicked);
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
            return tree ? tree->evaluate(t) : 0.0;
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

    QString targetStr = m_cmbDownsample->currentText();
    if (targetStr != "Original") {
        double targetRate = targetStr.toDouble();
        if (targetRate < processRate) {
            std::vector<double> dec;
            double step = processRate / targetRate;
            for (size_t i = 0; i < processData.size(); i += static_cast<size_t>(step)) {
                dec.push_back(processData[i]);
            }
            processData = std::move(dec);
            processRate = targetRate;
        }
    }

    MicroGP::Config config;
    config.legacySyntax = (m_cmbSyntax->currentText() == "Legacy");
    config.dechord = m_chkDechord->isChecked();
    config.sampleRate = processRate;
    config.populationSize = 300;
    config.generations = 60;

    m_btnDiscover->setEnabled(false);
    m_lblStatus->setText(QString("Running Native Micro-GP on %1 points...").arg(processData.size()));

    if (m_watcher) m_watcher->deleteLater();
    m_watcher = new QFutureWatcher<QString>(this);
    connect(m_watcher, &QFutureWatcher<QString>::finished, this, &SymbolicRegressionTab::onProcessFinished);

    QFuture<QString> future = QtConcurrent::run([config, processData, this]() mutable -> QString {
        MicroGP engine(config);
        auto result = engine.discoverExpression(processData);

        return result;
    });

    m_watcher->setFuture(future);
}

void SymbolicRegressionTab::onProcessFinished()
{
    m_btnDiscover->setEnabled(true);
    QString result = m_watcher->result();
    if (result.isEmpty()) result = "0";
    m_txtExpression->setPlainText(result);
    m_lblStatus->setText("Native Micro-GP Expression discovered!");


    updateScopePreview(true);
}

void SymbolicRegressionTab::onCopyClicked()
{
    if (!m_txtExpression->toPlainText().isEmpty()) {
        QApplication::clipboard()->setText(m_txtExpression->toPlainText());
        m_lblStatus->setText(" Copied to clipboard");
    }
}
