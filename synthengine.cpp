#include "synthengine.h"
#include <QDebug>
#include <QtEndian>
#include <QMutexLocker> // <--- ADD THIS

SynthEngine::SynthEngine(QObject *parent) : QIODevice(parent) {
    m_format.setSampleRate(44100);
    m_format.setChannelCount(2);
    m_format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(m_format)) {
        m_format = device.preferredFormat();
    }
    m_audioSink = new QAudioSink(device, m_format, this);
    m_audioSink->setBufferSize(16384);
}

SynthEngine::~SynthEngine() {
    stop();
    delete m_audioSink;
}

bool SynthEngine::isSequential() const { return true; }

qint64 SynthEngine::bytesAvailable() const {
    if (!isOpen()) return 0;
    return m_audioSink->bufferSize() + QIODevice::bytesAvailable();
}

void SynthEngine::start() {
    if (!isOpen()) open(QIODevice::ReadOnly);
    m_audioSink->start(this);
}

void SynthEngine::stop() {
    m_audioSink->stop();
    close();
}

void SynthEngine::setAudioSource(std::function<double(double)> func) {
    QMutexLocker locker(&m_mutex); // Locks here
    m_oscillator = func;
}


qint64 SynthEngine::readData(char *data, qint64 maxlen) {
    QMutexLocker locker(&m_mutex);

    int channels = m_format.channelCount();
    if (m_format.sampleFormat() == QAudioFormat::Float) {
        float *buffer = reinterpret_cast<float*>(data);
        int frames = maxlen / (sizeof(float) * channels);
        for (int i = 0; i < frames; ++i) {
            double t = (double)m_totalSamples / m_format.sampleRate();
            float sample = 0.0f;
            if(m_oscillator) sample = (float)m_oscillator(t) * 0.5f;

            for (int c = 0; c < channels; ++c) *buffer++ = sample;
            m_totalSamples++;
        }
        return frames * sizeof(float) * channels;
    }
    return 0;
}

qint64 SynthEngine::writeData(const char *data, qint64 len) {
    Q_UNUSED(data); Q_UNUSED(len); return 0;
}
