#ifndef VOCALTRANSFORMTAB_H
#define VOCALTRANSFORMTAB_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAudioSource>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QByteArray>
#include <QBuffer>
#include <QComboBox>
#include <vector>
#include <QMouseEvent>

class SynthEngine;

class VocalTransformTab : public QWidget {
    Q_OBJECT

public:
    explicit VocalTransformTab(SynthEngine* ghostSynth, QWidget *parent = nullptr);
    ~VocalTransformTab();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void toggleRecording();
    void processAudio();
    void generateXpressive();
    void handleAudioData();
    void playAudio();
    void trimSelection();
    void applyVocalMask();

private:
    void setupUI();

    QString generateNightlyVocalExpression(const std::vector<double>& buffer,
                                           double pitchMult,
                                           double formantMult);

    QString generateLegacyVocalExpression(const std::vector<double>& buffer,
                                          double pitchMult,
                                          double formantMult);

    QPushButton *btnRecord;
    QPushButton *btnPlay;
    QPushButton *btnTrim;
    QPushButton *btnVocalMask;
    QPushButton *btnProcess;
    QSlider *pitchSlider;
    QSlider *formantSlider;
    QSlider *roboticSlider;
    QComboBox *buildModeCombo;
    QTextEdit *xpressiveOutput;

    QAudioSource *audioInput = nullptr;
    QIODevice *audioIODevice = nullptr;
    QAudioSink *audioOutput = nullptr;
    QBuffer *playbackBuffer = nullptr;
    QAudioFormat audioFormat;

    std::vector<double> recordedBuffer;
    std::vector<double> processedBuffer;

    int selectionStartPixel = -1;
    int selectionEndPixel = -1;

    SynthEngine* m_ghostSynth;
};

#endif // VOCALTRANSFORMTAB_H
