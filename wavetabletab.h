#ifndef WAVETABLETAB_H
#define WAVETABLETAB_H

#include <QWidget>
#include <vector>
#include <QString>

class QSlider;
class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QCheckBox;
class SynthEngine;

class WavetableTab : public QWidget {
    Q_OBJECT

public:
    explicit WavetableTab(SynthEngine* ghostSynth, QWidget *parent = nullptr);
    ~WavetableTab() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onParametersChanged();
    void onGenerate();
    void togglePlay(bool checked);

private:
    void setupUI();
    double calculateWaveform(double phase, double wtPos, int warpMode, double warpAmt, int interpMode, int wtBank);

    SynthEngine* m_ghostSynth = nullptr;

    QWidget* canvasPlaceholder;


    QComboBox* cmbWtBank;
    QSlider* sldWtPos;
    QSlider* sldWarpAmt;
    QComboBox* cmbWarpMode;
    QComboBox* cmbInterpolation;


    QComboBox* cmbModSource;
    QSlider* sldModDepth;
    QSlider* sldModRate;


    QCheckBox* chkSubOsc;
    QComboBox* cmbSubShape;
    QComboBox* cmbSubOctave;
    QSlider* sldSubVol;


    QSpinBox* spinUnisonVoices;
    QSlider* sldUnisonDetune;
    QSlider* sldUnisonWidth;

    QComboBox* cmbParserMode;
    QPushButton* btnGenerate;
    QPushButton* btnPlay;

    QLabel* lblWtPos;
    QLabel* lblWarpAmt;
};

#endif // WAVETABLETAB_H
