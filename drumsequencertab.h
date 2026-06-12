#ifndef DRUMSEQUENCERTAB_H
#define DRUMSEQUENCERTAB_H

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QString>
#include <vector>

class DrumSequencerTab : public QWidget
{
    Q_OBJECT

public:
    explicit DrumSequencerTab(QWidget *parent = nullptr);
    ~DrumSequencerTab();

private slots:
    void onUiChanged();
    void onPresetChanged(int index);
    void copyToClipboard();

private:
    void setupUi();
    void generateMath();
    QString getSoundMath(int index);

    QComboBox* modeSelector;
    QComboBox* presetSelector;
    QCheckBox* enableDistortion;
    QTextEdit* codeOutput;
    QPushButton* btnCopy;

    static const int NUM_TRACKS = 5;
    static const int NUM_STEPS = 16;

    QComboBox* trackSounds[NUM_TRACKS];
    QCheckBox* stepGrid[NUM_TRACKS][NUM_STEPS];
    
    bool isUpdatingPreset;
};

#endif // DRUMSEQUENCERTAB_H