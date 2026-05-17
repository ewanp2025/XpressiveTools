#ifndef SPECTRALHOLOGRAMTAB_H
#define SPECTRALHOLOGRAMTAB_H

#include <QWidget>
#include <vector>
#include <QString>
#include <memory>

class QPushButton;
class QSlider;
class QTextEdit;
class QComboBox;
class QLabel;
class QSpinBox;
class QCheckBox;
class SynthEngine;

class SpectralHologramTab : public QWidget {
    Q_OBJECT

public:
    explicit SpectralHologramTab(SynthEngine* ghostSynth, QWidget *parent = nullptr);
    ~SpectralHologramTab() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onLoadWav();
    void onAnalyze();
    void onGenerate();
    void onApplyMaterial(int index);
    void togglePlay(bool checked);

private:
    void setupUI();
    bool loadWavToMemory(const QString& path);
    QString buildTree(int start, int end, const std::vector<double>& arr);

    SynthEngine* m_ghostSynth = nullptr;
    

    std::vector<double> m_audioData;
    double m_sampleRate = 44100.0;
    double m_duration = 0.0;


    std::vector<double> m_masterEnv;
    std::vector<double> m_harmonicWeights;
    
    int m_envResolution = 64;


    QPushButton* btnLoad;
    QPushButton* btnAnalyze;
    QPushButton* btnGenerate;
    QPushButton* btnPlay;
    
    QSpinBox* spinPartials;
    QSlider* sldStretch;
    QSlider* sldInharmonicity;
    QSlider* sldFormantShift;
    QSlider* sldSpectralBlur;
    
    QComboBox* cmbMaterial;
    QComboBox* cmbBuildMode;
    QTextEdit* txtOutput;
    QLabel* lblStatus;
};

#endif // SPECTRALHOLOGRAMTAB_H
