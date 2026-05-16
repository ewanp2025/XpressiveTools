#ifndef XPRESSIVESEQTAB_H
#define XPRESSIVESEQTAB_H

#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDial>
#include <QCheckBox>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

class XpressiveSeqTab : public QWidget
{
    Q_OBJECT

public:
    explicit XpressiveSeqTab(QWidget *parent = nullptr);
    ~XpressiveSeqTab();

private slots:
    void onUiChanged();
    void onTextChanged();

private:
    void setupUi();
    void generateString();


    QComboBox* modeSelector;
    QSpinBox* numStepsSpin;
    QCheckBox* toggleHardGate;
    QTextEdit* codeOutputO1;
    QTextEdit* codeOutputO2;

    QCheckBox* stepToggles[32];
    QCheckBox* stepAccents[32];
    QComboBox* stepPitches[32];

    QCheckBox* stepSquelch[32];
    QSpinBox* stepSquAmt[32];

    QDial* dialDriveBase;
    QDial* dialDriveSweep;
    QDial* dialDecayBase;
    QDial* dialDecaySweep;
    QDial* dialMacroClock;
    QDial* dialDetune;
    QDial* dialSquDecay;

    QLabel* lblDriveBase;
    QLabel* lblDriveSweep;
    QLabel* lblDecayBase;
    QLabel* lblDecaySweep;
    QLabel* lblMacroClock;
    QLabel* lblDetune;
    QLabel* lblSquDecay;


    QCheckBox* toggleNSCLathe;
    QComboBox* comboFilterType;


    QCheckBox* toggleWobble;
    QDial* dialTimbralSmear;
    QLabel* lblTimbralSmear;


    QCheckBox* toggleEcho;
    QDial* dialEchoAmt;
    QLabel* lblEchoAmt;


    QCheckBox* toggleEvolution;
    QSpinBox* spinEvolutionBar;

    bool isUpdatingUI;
};

#endif // XPRESSIVESEQTAB_H
