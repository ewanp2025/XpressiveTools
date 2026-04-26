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
#include <QMediaDevices>
#include <QAudioDevice>
#include <QByteArray>
#include <QBuffer>
#include <vector>

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

private:
    void setupUI();

    QPushButton *btnRecord;
    QPushButton *btnProcess;
    QSlider *pitchSlider;
    QSlider *formantSlider;
    QSlider *roboticSlider;
    QTextEdit *xpressiveOutput;


    QAudioSource *audioInput = nullptr;
    QIODevice *audioIODevice = nullptr;
    QAudioFormat audioFormat;

    std::vector<double> recordedBuffer;
    std::vector<double> processedBuffer;

    int selectionStartPixel = -1;
    int selectionEndPixel = -1;

    SynthEngine* m_ghostSynth;
};

#endif // VOCALTRANSFORMTAB_H
