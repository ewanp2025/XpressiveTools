#include "synthengine.h"
#include <QDebug>
#include <QtEndian>
#include <QMutexLocker>
#include <QDebug>

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

    open(QIODevice::ReadOnly);
    m_audioSink->start(this);
}

SynthEngine::~SynthEngine() {
    m_audioSink->stop();
    close();
    delete m_audioSink;
}

bool SynthEngine::isSequential() const { return true; }

qint64 SynthEngine::bytesAvailable() const {
    if (!isOpen()) return 0;
    return m_audioSink->bufferSize() + QIODevice::bytesAvailable();
}

void SynthEngine::start() {
    QMutexLocker locker(&m_mutex);
    m_totalSamples = 0;
    m_isPlaying = true;
}
void SynthEngine::stop() {
    QMutexLocker locker(&m_mutex);
    m_isPlaying = false;
}

void SynthEngine::setAudioSource(std::function<double(double)> func) {
    QMutexLocker locker(&m_mutex);
    m_oscillator = func;
}

void SynthEngine::setExpression(QString code) {
    m_currentCode = code;
}

qint64 SynthEngine::readData(char *data, qint64 maxlen) {
    QMutexLocker locker(&m_mutex);


    memset(data, 0, maxlen);

    int channels = m_format.channelCount();
    if (channels == 0) return maxlen;

    if (m_format.sampleFormat() == QAudioFormat::Float) {
        float *buffer = reinterpret_cast<float*>(data);
        int frames = maxlen / (sizeof(float) * channels);

        for (int i = 0; i < frames; ++i) {
            float sample = 0.0f;

            if (m_isPlaying && m_oscillator) {
                double t = (double)m_totalSamples / m_format.sampleRate();


                if (m_totalSamples < 5) {
                    qDebug() << "[AUDIO] Fetching sample for time:" << t;
                }

                sample = (float)m_oscillator(t) * 0.5f;

                if (std::isnan(sample) || std::isinf(sample)) sample = 0.0f;
                m_totalSamples++;
            }

            for (int c = 0; c < channels; ++c) {
                *buffer++ = sample;
            }
        }
    }


    return maxlen;
}

qint64 SynthEngine::writeData(const char *data, qint64 len) {
    Q_UNUSED(data);
    return len;
}
