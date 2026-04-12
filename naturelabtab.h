#ifndef NATURELABTAB_H
#define NATURELABTAB_H

#include <QWidget>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QGroupBox>

class UniversalScope;

class NatureLabTab : public QWidget {
    Q_OBJECT

public:
    explicit NatureLabTab(QWidget *parent = nullptr);

signals:
    void playRequested(QString expression);
    void stopRequested();

private slots:
    void generateExpression();
    void updateLabels();
    void changeCategory(int index);
    void togglePlay(bool checked);


    void loadAvianPreset(int index);
    void loadMammalPreset(int index);
    void loadGeoPreset(int index);

private:
    void setupUI();

    QComboBox *categorySelector;
    QStackedWidget *stackedWidget;
    QComboBox *buildModeSelector;


    QWidget *avianWidget;
    QComboBox *avianPresetCombo;
    QSlider *avianBaseFreqSlider;
    QLabel  *avianBaseFreqLabel;
    QSlider *avianSweepRateSlider;
    QLabel  *avianSweepRateLabel;
    QSlider *avianFmDepthSlider;
    QLabel  *avianFmDepthLabel;
    QSlider *avianDetuneSlider;
    QLabel  *avianDetuneLabel;
    QSlider *avianDecaySlider;
    QLabel  *avianDecayLabel;
    QSlider *avianModFreqSlider;
    QLabel  *avianModFreqLabel;


    QWidget *mammalWidget;
    QComboBox *mammalPresetCombo;
    QSlider *mammalFundFreqSlider;
    QLabel  *mammalFundFreqLabel;
    QSlider *mammalHnrSlider;
    QLabel  *mammalHnrLabel;
    QSlider *mammalSubharmonicSlider;
    QLabel  *mammalSubharmonicLabel;
    QSlider *mammalFormantSlider;
    QLabel  *mammalFormantLabel;
    QSlider *mammalVibratoRateSlider;
    QLabel  *mammalVibratoRateLabel;
    QSlider *mammalDecaySlider;
    QLabel  *mammalDecayLabel;


    QWidget *geoWidget;
    QComboBox *geoPresetCombo;
    QSlider *geoTurbulenceSlider;
    QLabel  *geoTurbulenceLabel;
    QSlider *geoIntensitySlider;
    QLabel  *geoIntensityLabel;
    QSlider *geoBaseFreqSlider;
    QLabel  *geoBaseFreqLabel;
    QSlider *geoSweepSlider;
    QLabel  *geoSweepLabel;
    QSlider *geoDecaySlider;
    QLabel  *geoDecayLabel;


    QTextEdit *outputExpressionBox;
    UniversalScope *natureScope;
    QPushButton *btnPlayNature;
};

#endif // NATURELABTAB_H
