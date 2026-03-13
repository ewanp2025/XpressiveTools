#ifndef PCMEDITORTAB_H
#define PCMEDITORTAB_H

#include <QWidget>
#include <vector>
#include <QPainter>
#include <QMouseEvent>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTextEdit>
#include <QSlider>
#include <QTableWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QHeaderView>
#include <QRegularExpression>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>


class SynthEngine;
class UniversalScope;


class PCMAudioBuffer {
public:
    std::vector<float> data;
    float sampleRate = 44100.0f;

    void normalize();
    void trim(int startIdx, int endIdx);
    void applyPingPong(int startIdx, int endIdx);
    void reverseRegion(int startIdx, int endIdx);
    float detectPitch(int startIdx, int endIdx);
};

// --- The UI Layer ---
class PCMEditorTab : public QWidget {
    Q_OBJECT

public:

    explicit PCMEditorTab(SynthEngine* ghostSynth, QWidget *parent = nullptr);
    void loadPCMExpression(const std::vector<float>& newData);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onTrimClicked();
    void onPingPongClicked();
    void onReverseClicked();
    void onNormalizeClicked();
    void onDetectPitchClicked();
    void onTimeStretchClicked();
    void togglePlayNightly();
    void onChopClicked();
    void onFadeInClicked();
    void onFadeOutClicked();
    void onAkaiGrimeClicked();

private:
    PCMAudioBuffer audioBuffer;
    SynthEngine* m_ghostSynth;


    int selectionStartPixel = -1;
    int selectionEndPixel = -1;
    int pixelToIndex(int pixelX);

    void setupAmigaUI();


    std::vector<double> m_nightlyBuffer;
    double m_nightlySampleRate = 8000.0;

    QTextEdit *nightlyPcmInput;
    QTextEdit *nightlyPcmOutput;

    UniversalScope *nightlySourceScope;
    UniversalScope *nightlyMutatedScope;

    QSlider *ntTrimStart;
    QSlider *ntTrimLength;
    QSlider *ntSpeedStretch;
    QSlider *ntBitcrush;
    QSlider *ntWavefold;
    QSlider *ntFormantShift;

    QSpinBox *ntSliceCount;
    QTableWidget *ntSliceTable;

    QPushButton *btnPlayNightly;
    QPushButton *btnCopyNightly;


    void parseNightlyInput();
    void updateNightlyPreview();
    void generateNightlyExpression();

    QComboBox *buildModeCombo;


    QString generateLegacyPCM(const std::vector<double>& q, double sr);

};

#endif // PCMEDITORTAB_H
