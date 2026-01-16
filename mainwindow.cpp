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
#include <QSlider>
#include <QScrollArea>
#include <QMap>
#include <vector>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>

// --- NEW STRUCTURE FOR PHONETIC LAB ---
struct SAMPhoneme {
    QString name;
    int f1, f2, f3;
    bool voiced;
    // Amplitudes (Default standard roll-off)
    int a1 = 15;
    int a2 = 10;
    int a3 = 5;
    // Relative Length in Frames (1 Frame ~ 12ms)
    // Plosives = 5, Fricatives = 12, Vowels = 18+
    int length = 12;
};
// --------------------------------------

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
    void generateConsoleWave();
    void generateSFXMacro();
    void generateFilterForge();
    void generateArpAnimator();
    void generateWavetableForge();
    void generateBesselFM();
    void loadBesselPreset(int index);
    void generateHarmonicLab();
    void generateVelocilogic();
    void generateNoiseForge();
    void generateXPFPackager();
    void generateLeadStack();
    void generateRandomPatch();
    void generateDrumArchitect();
    
    // --- NEW SLOT ---
    void generatePhoneticFormula();
    void generateStepGate();

private:
    void setupUI();
    // --- NEW INIT FUNCTION ---
    void initSamLibrary();

    QString generateLegacyPCM(const std::vector<double>& q, double sr);
    QString generateModernPCM(const std::vector<double>& q, double sr);
    QString getModulatorFormula(int index);
    QString getArpFormula(int index);
    QString getSegmentWaveform(const SidSegment& s, const QString& fBase);

    QTabWidget *modeTabs;
    QDoubleSpinBox *maxDurSpin;
    QComboBox *sampleRateCombo, *buildModeCombo, *buildModeSid;
    QComboBox *buildModeConsole, *buildModeSFX, *buildModeFilter, *buildModeArp, *buildModeWavetable;
    QComboBox *buildModeBessel, *buildModeHarmonic, *buildModeVeloci, *buildModeNoise;
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

    QComboBox *arpWave;
    QSlider *arpPwmSlider;      // The soul of the C64
    QCheckBox *arpBpmSync;      // "Lock to BPM"
    QDoubleSpinBox *arpBpmVal;  // The Song BPM
    QComboBox *arpSpeedDiv;     // 1/16, 1/32, 1/64 (The "Hubbard Speed")


    QComboBox *arpInterval1, *arpInterval2;
    QComboBox *wtBase;
    QDoubleSpinBox *wtHarmonics;

    QComboBox *besselPresetCombo;
    QComboBox *besselCarrierWave, *besselModWave;
    QDoubleSpinBox *besselCarrierMult, *besselModMult, *besselModIndex;

    QSlider *harmonicSliders[16];
    QComboBox *velociType;
    QDoubleSpinBox *noiseRes;
    QTextEdit *xpfInput;

    QComboBox *drumType;
    QComboBox *drumBuildMode;
    QSlider *drumFreqSlider;
    QSlider *drumPunchSlider;
    QSlider *drumDecaySlider;
    QSlider *drumToneSlider;
    QLabel *drumFilterWarning;

    QDoubleSpinBox *leadUnisonCount;
    QDoubleSpinBox *leadDetuneAmount;
    QComboBox *leadWaveType;
    QSlider *chaosSlider;

    // --- NEW VARIABLES FOR PHONETIC LAB ---
    QTextEdit *phoneticInput;
    QComboBox *parserModeCombo;   // Render Mode: HQ vs Retro
    QComboBox *parsingStyleCombo; // Parsing Engine: Legacy vs Nightly
    QPushButton *btnGenPhonetic;
    QLabel *phonemeRefLabel;
    QMap<QString, SAMPhoneme> samLibrary;

    // --- NEW CONVERTER TAB VARIABLES ---
    QTextEdit *convInput;       // The left box (Source)
    QTextEdit *convOutput;      // The right box (Result)
    QPushButton *btnToNightly;  // Button: Legacy -> Nightly
    QPushButton *btnToLegacy;   // Button: Nightly -> Legacy


    // --- NEW WAVETABLE TRACKER STRUCTURES ---
    struct WavetableStep {
        QString shape;      // "Pulse", "Saw", "Tri", "Noise"
        int semitones;      // Transpose (e.g., 0, +7, +12)
        int pwm;            // Pulse Width (0-100)
        double duration;    // How long to hold this step (in seconds)
    };

    QTableWidget *wtTrackerTable;
    QComboBox *wtPresetCombo;
    QCheckBox *wtLoopCheck;

    // Key Mapper Variables
    QTableWidget *keyMapTable;
    QComboBox *keyMapMode;
    QLabel *keyMapDisclaimer;
    void generateKeyMapper();

    // Velocilogic Variables
    QTableWidget *velMapTable;
    QComboBox *velMapMode;
    QLabel *velDisclaimer;


    // Helper to load presets
    void loadWavetablePreset(int index);

    // Disclaimers
    QLabel *convDisclaimer; //
    QLabel *filterDisclaimer;
    QLabel *drumDisclaimer;
    QLabel *xpfDisclaimer;
    QPushButton *btnSaveXpf;

    void saveXpfInstrument();

    // --- NEW STEP GATE VARIABLES ---
    QComboBox *gateBuildMode;
    QComboBox *gateSpeedCombo;
    QCheckBox *gateTripletCheck;
    QComboBox *gateShapeCombo;
    QTextEdit *gateCustomShape;
    QPushButton *gateSteps[16];
    QSlider *gateMixSlider;


    // --- LEGACY PIANO LAB VARIABLES ---
//    QTextEdit *pianoSourceEdit;

    // Core Instrument Controls
//    QSlider *pianoVolSlider;
//    QSlider *pianoCutoffSlider;
//    QSlider *pianoResSlider;

    // Envelope Controls
//    QSlider *pianoAttSlider;
//    QSlider *pianoDecSlider;
//    QSlider *pianoSusSlider;
//    QSlider *pianoRelSlider;

    // NEW: Math Expression Mixers
//    QSlider *pianoMixSawSlider;   // Center Saw
//    QSlider *pianoMixSubSlider;   // Sub Octave (-12)
//    QSlider *pianoMixHighSlider;  // High Octave (+12)

    // NEW: Effects (Reverb/Plate)
//    QSlider *pianoRevBandwidth;
//    QSlider *pianoRevTail;
//    QSlider *pianoRevDamping;
//    QSlider *pianoRevMix;

//    QLabel *pianoStatusLabel;

    // Function to run the logic
//    void saveLegacyPiano();

    // Helper functions for the logic
    QString convertLegacyToNightly(QString input);
    QString convertNightlyToLegacy(QString input);

    // ... end of class ...

};
#endif
