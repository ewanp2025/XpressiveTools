#ifndef WAVETABLETAB_H
#define WAVETABLETAB_H

#include <QWidget>
#include <vector>
#include <QString>

class QSlider;
class QComboBox;
class QSpinBox;
class QPushButton;
class QTextEdit;
class QLabel;
class QCheckBox;
class SynthEngine;

class WavetableTab : public QWidget {
    Q_OBJECT

public:
    explicit WavetableTab(SynthEngine* ghostSynth, QWidget *parent = nullptr);
    ~WavetableTab() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onParametersChanged();
    void onGenerate();
    void togglePlay(bool checked);

private:
    void setupUI();
    double calculateWaveform(double phase, double wtPos, int warpMode, double warpAmt);

    SynthEngine* m_ghostSynth = nullptr;


    QSlider* sldWtPos;
    QSlider* sldWarpAmt;
    QComboBox* cmbWarpMode;
    
    QComboBox* cmbModSource;
    QSlider* sldModDepth;
    QSlider* sldModRate;

    QSlider* sldUnisonDetune;
    QSpinBox* spinUnisonVoices;

    QTextEdit* txtOutput;
    QPushButton* btnGenerate;
    QPushButton* btnPlay;

    QLabel* lblWtPos;
    QLabel* lblWarpAmt;
};

#endif // WAVETABLETAB_H
