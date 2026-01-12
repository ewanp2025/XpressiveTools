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
#include <QLabel>
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
    void saveSidExpr();
    void clearAllSid();
    void generateConsoleWave();
    void generateSFXMacro();
    void generateFilterForge();
    void generateArpAnimator();
    void generateWavetableForge();

private:
    void setupUI();
    QString generateLegacyPCM(const std::vector<double>& q, double sr);
    QString generateModernPCM(const std::vector<double>& q, double sr);
    QString getModulatorFormula(int index);
    QString getArpFormula(int index);
    QString getSegmentWaveform(const SidSegment& s, const QString& fBase);

    QTabWidget *modeTabs;
    QDoubleSpinBox *maxDurSpin;
    QComboBox *sampleRateCombo, *buildModeCombo, *buildModeSid;
    QComboBox *buildModeConsole, *buildModeSFX, *buildModeFilter, *buildModeArp, *buildModeWavetable;
    QCheckBox *normalizeCheck;
    QTextEdit *statusBox;
    QPushButton *btnSave, *btnCopy;
    WaveformDisplay *waveVisualizer;

    std::vector<SidSegment> sidSegments;
    QVBoxLayout *sidSegmentsLayout;

    Modulator mods[5];
    ArpSettings arps[2];
    std::vector<double> originalData;
    uint32_t fileFs = 44100;

    QComboBox *consoleWaveType;
    QDoubleSpinBox *consoleSteps;
    QDoubleSpinBox *sfxStartFreq, *sfxEndFreq, *sfxDur;
    QComboBox *sfxWave;
    QComboBox *filterType;
    QDoubleSpinBox *filterTaps;
    QDoubleSpinBox *arpSpeed;
    QComboBox *arpInterval1, *arpInterval2;
    QComboBox *wtBase;
    QDoubleSpinBox *wtHarmonics;
};
#endif
