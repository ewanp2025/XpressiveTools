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
#include <QPainterPath>
#include <QTimer>

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

// --- ENVELOPE VISUALIZER CLASS ---
class EnvelopeDisplay : public QWidget {
    Q_OBJECT
public:
    explicit EnvelopeDisplay(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(120);
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }
    void updateEnvelope(double a, double d, double s, double r) {
        m_a = a; m_d = d; m_s = s; m_r = r;
        update();
    }
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        int w = width(), h = height();

        // Background Grid
        painter.setPen(QColor(45, 45, 45));
        painter.drawLine(w/4, 0, w/4, h);
        painter.drawLine(w/2, 0, w/2, h);
        painter.drawLine(3*w/4, 0, 3*w/4, h);

        QPainterPath path;
        path.moveTo(0, h);

        double x_a = m_a * (w/4.0);
        double x_d = x_a + (m_d * (w/4.0));
        double x_s = x_d + (w/4.0);
        double x_r = x_s + (m_r * (w/4.0));
        double y_s = h - (m_s * (h - 20));

        path.lineTo(x_a, 10);    // Attack
        path.lineTo(x_d, y_s);   // Decay
        path.lineTo(x_s, y_s);   // Sustain
        path.lineTo(x_r, h);     // Release

        painter.setPen(QPen(QColor(0, 255, 120), 3));
        painter.drawPath(path);
    }
private:
    double m_a=0, m_d=0.5, m_s=0.5, m_r=0.1;
};

// --- UNIVERSAL OSCILLOSCOPE (ScopeWidget) ---
// Reusable visualizer for any tab.
// Usage: scope->updateScope([](double t){ return sin(t*440); }, 1.0);
#include <functional>

class UniversalScope : public QWidget {
    Q_OBJECT
public:
    explicit UniversalScope(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(120);
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
        // Default: flat line
        m_generator = [](double){ return 0.0; };
    }

    // Call this from your tab to update the visual
    // waveFunc: A lambda function representing your math (t is time)
    // duration: Total length of the sound in seconds (e.g., 2.0s)
    // zoom: 0.0 = Single Cycle (Zoomed In), 1.0 = Full Duration (Zoomed Out)
    void updateScope(std::function<double(double)> waveFunc, double duration, double zoom) {
        m_generator = waveFunc;
        m_duration = duration;
        m_zoom = zoom;
        update(); // Trigger repaint
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(20, 20, 20)); // Dark Background

        int w = width();
        int h = height();
        int midY = h / 2;

        // Draw Grid Center Line
        painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DashLine));
        painter.drawLine(0, midY, w, midY);

        if (w < 1) return;

        // 1. Calculate Time Window
        // If zoom is 0, we view ~0.02s (approx 50Hz cycle).
        // If zoom is 1, we view m_duration.
        double windowSize = 0.02 + (m_duration - 0.02) * m_zoom;
        if(windowSize <= 0) windowSize = 0.01;

        // 2. Generate Points
        QPainterPath path;
        bool started = false;

        // Resolution: We don't need infinite points, just enough for the pixel width
        int resolution = w;

        painter.setPen(QPen(QColor(0, 255, 255), 2)); // Cyan Line

        for (int x = 0; x < resolution; ++x) {
            // Map pixel X to Time T
            double t = (double)x / (double)resolution * windowSize;

            // Execute the passed Math Function
            double sample = m_generator(t);

            // Clamp sample to -1..1 for safety
            if (sample > 1.0) sample = 1.0;
            if (sample < -1.0) sample = -1.0;

            // Map Amplitude to Y pixels
            // (Negative because Y grows downwards in UI coords)
            double y = midY - (sample * (midY - 10));

            if (!started) {
                path.moveTo(x, y);
                started = true;
            } else {
                path.lineTo(x, y);
            }
        }
        painter.drawPath(path);

        // Draw Info Text
        painter.setPen(QColor(200, 200, 200));
        QString info = QString("Window: %1s").arg(windowSize, 0, 'f', 3);
        painter.drawText(5, 15, info);
    }

private:
    std::function<double(double)> m_generator;
    double m_duration = 1.0;
    double m_zoom = 0.0; // 0 = Cycles, 1 = Full
};

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
    void generateDrumXpf();
    void generatePhoneticFormula();
    void generateStepGate();
    void generateWestCoast();

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

    // --- DRUM DESIGNER REDO ---
    QComboBox *drumTypeCombo;
    QComboBox *drumWaveCombo;
    QSlider *drumPitchSlider, *drumDecaySlider, *drumToneSlider, *drumSnapSlider;
    QSlider *drumNoiseSlider, *drumPitchDropSlider, *drumPWMSlider;
    QSlider *drumExpSlider; // Added for exponential decay
    QPushButton *btnGenerateDrum, *btnSaveDrumXpf;
    QString getXpfTemplate();

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

    // --- NUMBERS 1981 VARIABLES ---
    QComboBox *numModeCombo;       // Random vs Pattern
    QComboBox *numStepsCombo;      // 16 or 32 Steps
    QDoubleSpinBox *numDuration;   // Note length
    QTableWidget *numPatternTable; // The manual editor
    QTextEdit *numOut1;            // Output Box Left
    QTextEdit *numOut2;            // Output Box Right

    void generateNumbers1981();    // Renamed function



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



    // Helper functions for the logic
    QString convertLegacyToNightly(QString input);
    QString convertNightlyToLegacy(QString input);



    // DELAY ARCHITECT VARIABLES
    QComboBox *delayWaveCombo;
    QTextEdit *delayCustomInput;
    QDoubleSpinBox *delayTimeSpin;   // Delay time in seconds
    QDoubleSpinBox *delayFeedbackSpin; // Decay amount (0.0 - 1.0)
    QSpinBox *delayTapsSpin;         // Number of echoes
    QDoubleSpinBox *delayRateSpin;   // Internal sample rate reference (e.g., 8000)


    void generateDelayArchitect();


    // --- MACRO MORPH VARIABLES ---
    QComboBox *macroStyleCombo;
    QComboBox *macroBuildMode;   // NEW: Legacy/Nightly Selector

    QSlider *macroWonkySlider;   // Swing/Sidechain
    QSlider *macroColorSlider;   // Timbre
    QSlider *macroTimeSlider;    // Envelope
    QSlider *macroBitcrushSlider; // RENAMED: Was macroDegradeSlider
    QSlider *macroTextureSlider; // Noise/Grain
    QSlider *macroWidthSlider;   // Stereo/Detune

    QSpinBox *macroDetuneSpin;
    void generateMacroMorph();

    // --- STRING MACHINE VARIABLES (UPDATED) ---
    QComboBox *stringModelCombo;
    QComboBox *stringChordCombo;

    QSlider *stringEnsembleSlider;  // Chorus/Detune
    QSlider *stringAttackSlider;    // Volume Swell
    QSlider *stringEvolveSlider;    // NEW: Brightness Swell (Filter Env)
    QSlider *stringMotionSlider;    // NEW: Phase Drift (Visual Fix)
    QSlider *stringSpaceSlider;     // Reverb-ish Release
    QSlider *stringAgeSlider;       // Pitch Wobble

    void generateStringMachine();

    //-vintage lab
    void setupHardwareLab();
    void generateHardwareXpf();
    void loadHardwarePreset(int idx);
    void generateRandomHardware();


    EnvelopeDisplay *adsrVisualizer;
    QComboBox *hwBaseWave;
    QComboBox *hwPresetCombo;
    QSlider *hwAttack;
    QSlider *hwDecay;    // Required for the compiler error
    QSlider *hwSustain;
    QSlider *hwRelease;
    QSlider *hwCutoff;   // Required for the compiler error
    QSlider *hwResonance;
    QCheckBox *hwPeakBoost;
    QSlider *hwPwmSpeed;
    QSlider *hwPwmDepth;
    QSlider *hwVibSpeed;
    QSlider *hwVibDepth;
    QSlider *hwNoiseMix;
    QSpinBox *hwBaseNote; // Required for pitch control logic


    QComboBox *westBuildMode;
    QComboBox *westModelSelect;
    QSlider *westTimbreSlider;   // Input Gain (Wavefolding Depth)
    QSlider *westSymmetrySlider; // DC Offset (Even Harmonics)
    QSlider *westOrderSlider;    // Harmonic Balance / Crossfade
    QCheckBox *westVactrolSim;   // LPG "Bongo" Decay
    QSlider *westModFreqSlider;  // Modulation Oscillator Frequency (FM)
    QSlider *westModIndexSlider; // FM Index (Depth)
    QCheckBox *westHalfWaveFold; // Toggle for Half-Wave vs Full-Wave folding
    QSlider *westFoldStages;     // Number of cascaded folding stages (Serge-style)

    // Universal Scope
    UniversalScope *stringScope;
    QSlider *stringZoomSlider;
    UniversalScope *westScope;
    QSlider *westZoomSlider;


};
#endif
