#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QClipboard>
#include <QApplication>
#include <QPainter>
#include <vector>

struct SidSegment {
    QComboBox* waveType;
    QDoubleSpinBox* duration;
    QDoubleSpinBox* decay;
    QDoubleSpinBox* freqOffset;
    QPushButton* deleteBtn;
    QWidget* container;
};

struct Modulator {
    QComboBox* shape;
    QDoubleSpinBox* rate;
    QDoubleSpinBox* depth;
    QCheckBox* sync;
    QComboBox* multiplier;
};

struct ArpSettings {
    QComboBox* wave;
    QComboBox* chord;
    QDoubleSpinBox* speed;
    QCheckBox* sync;
    QComboBox* multiplier;
};

class WaveformDisplay : public QWidget {
    Q_OBJECT
public:
    explicit WaveformDisplay(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(150);
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }
    void updateData(const std::vector<SidSegment>& segments);
protected:
    void paintEvent(QPaintEvent *) override;
private:
    std::vector<SidSegment> m_segments;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
private slots:
    void loadWav();
    void saveExpr();
    void copyToClipboard();
    void addSidSegment();
    void removeSidSegment();
    void clearAllSid();
    void saveSidExpr();
private:
    void setupUI();
    QString generateLegacyPCM(const std::vector<double>& q, double sr);
    QString generateModernPCM(const std::vector<double>& q, double sr);
    QString getGlobalAdsrFormula();
    QString applyBitcrush(const QString& expr);
    QString getModulatorFormula(int index);
    QString getArpFormula(int index);

    QTabWidget *modeTabs;
    QDoubleSpinBox *maxDurSpin;
    QComboBox *sampleRateCombo, *buildModeCombo;
    QCheckBox *normalizeCheck;
    QTextEdit *statusBox;
    QPushButton *btnSave, *btnCopy;
    WaveformDisplay *waveVisualizer;

    std::vector<SidSegment> sidSegments;
    QVBoxLayout *sidSegmentsLayout;
    QDoubleSpinBox *aSpin, *dSpin, *sSpin;
    QCheckBox *useGlobalAdsr;

    Modulator mods[5];
    ArpSettings arps[2];
    std::vector<double> originalData;
    uint32_t fileFs = 44100;
};
#endif
