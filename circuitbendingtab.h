#ifndef CIRCUITBENDINGTAB_H
#define CIRCUITBENDINGTAB_H

#include <QWidget>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>

class SynthEngine;
class UniversalScope;

class CircuitBendingTab : public QWidget {
    Q_OBJECT

public:
    explicit CircuitBendingTab(SynthEngine* ghostSynth, QWidget *parent = nullptr);
    ~CircuitBendingTab() override;

private slots:
    void generateExpression();
    void togglePlay(bool checked);
    void updateUI();

private:
    void setupUI();
    QString buildExprTkString();
    QComboBox* m_cmbBuildMode;

    SynthEngine* m_ghostSynth = nullptr;
    UniversalScope* m_scope = nullptr;


    QComboBox* m_cmbSubstrate;


    QCheckBox* m_patchMatrix[4][4];
    QString m_anomalyNames[4] = {"Voltage Sag (Envelope)", "CMOS Gate (Bytebeat)", "ROM Glitch (Aleatoric)", "Body Contact (Galvanic)"};
    QString m_targetNames[4] = {"Clock (Phase)", "Pitch / Freq", "Amplitude (VCA)", "Main Output"};


    QSlider* m_sldSagDepth;
    QSlider* m_sldClockStarve;
    QSlider* m_sldRomGlitch;
    QSlider* m_sldBodyPressure;

    QLabel* m_lblSag;
    QLabel* m_lblClock;
    QLabel* m_lblRom;
    QLabel* m_lblBody;


    QTextEdit* m_txtOutput;
    QPushButton* m_btnGenerate;
    QPushButton* m_btnPlay;
};

#endif // CIRCUITBENDINGTAB_H