#ifndef SYNTHENGINE_H
#define SYNTHENGINE_H

#include <QIODevice>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioDevice>
#include <cmath>
#include <functional>

class SynthEngine : public QIODevice {
    Q_OBJECT

public:
    explicit SynthEngine(QObject *parent = nullptr);
    ~SynthEngine();

    void start();
    void stop();
    void setAudioSource(std::function<double(double)> func);

    // --- NEW REQUIRED OVERRIDES ---
    // These tell Qt "I am a live stream, don't stop reading!"
    bool isSequential() const override;
    qint64 bytesAvailable() const override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QAudioSink *m_audioSink = nullptr;
    QAudioFormat m_format;
    std::function<double(double)> m_oscillator = [](double t){ return 0.0; };

    double m_sampleRate = 44100.0;
    qint64 m_totalSamples = 0;
};

#endif // SYNTHENGINE_H
