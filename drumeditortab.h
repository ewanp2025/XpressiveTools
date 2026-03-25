#ifndef DRUMEDITORTAB_H
#define DRUMEDITORTAB_H

#include <QWidget>
#include <QString>
#include <vector>
#include <functional>
#include <QStackedWidget>


class QPushButton;
class QTextEdit;
class QRadioButton;
class QLabel;
class QComboBox;
class QSlider;
class UniversalScope; 
class SynthEngine;

class DrumEditorTab : public QWidget {
    Q_OBJECT

public:

    explicit DrumEditorTab(SynthEngine* synthEngine, QWidget *parent = nullptr);

signals:
    void expressionGenerated(const QString& expr);
    void statusMessage(const QString& msg);
private slots:

    void onModeSwitched();


    void updateDrum();
    void generateDrumXpf();

    void onLoadSample();
    void analyzeAudioData(const std::vector<float>& audioData, int sampleRate);
    void onTrimChanged();

private:
    SynthEngine* m_ghostSynth;
    QString getXpfTemplate();

    std::vector<float> m_rawAudioData;
    int m_rawSampleRate = 44100;

    QSlider* m_trimStartSlider;
    QSlider* m_trimEndSlider;
    QLabel* m_lblTrimDetails;


    QRadioButton* m_radioManual;
    QRadioButton* m_radioAnalyze;
    QWidget* m_manualContainer;
    QWidget* m_analyzeContainer;


    UniversalScope* drumScope;
    QLabel* drumDisclaimer;
    QComboBox* drumTypeCombo;
    QComboBox* drumWaveCombo;
    QSlider *drumPitchSlider, *drumDecaySlider, *drumPitchDropSlider;
    QSlider *drumToneSlider, *drumSnapSlider, *drumNoiseSlider;
    QSlider *drumPWMSlider, *drumExpSlider;
    QPushButton *btnPlayDrum, *btnGenerateDrum, *btnSaveDrumXpf;
    QStackedWidget* m_modeStack;


    QPushButton* m_btnLoadSample;
    QTextEdit* m_outputExpression;
    QLabel* m_lblAnalysisResults;
    QLabel* m_lblFilterRecommendation;
    QTextEdit* m_manualOutputBox;
    UniversalScope* m_analyzeScope;


    std::vector<float> loadWavFile(const QString& filePath, int& outSampleRate);

};

#endif // DRUMEDITORTAB_H
