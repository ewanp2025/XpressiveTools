#include "synthengine.h"
#include <QDebug>
#include <QtEndian>

SynthEngine::SynthEngine(QObject *parent) : QIODevice(parent) {
    // 1. CONFIGURATION (Stereo Float)
    m_format.setSampleRate(44100);
    m_format.setChannelCount(2);
    m_format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(m_format)) {
        qWarning() << "Format not supported, using preferred.";
        m_format = device.preferredFormat();
    }

    m_audioSink = new QAudioSink(device, m_format, this);

    // Prevent Windows Stall
    m_audioSink->setBufferSize(16384);

    connect(m_audioSink, &QAudioSink::stateChanged, this, [&](QAudio::State state){
        switch(state) {
        case QAudio::ActiveState: qDebug() << "Synth: ACTIVE"; break;
        case QAudio::SuspendedState: qDebug() << "Synth: SUSPENDED"; break;
        case QAudio::StoppedState: qDebug() << "Synth: STOPPED"; break;
        case QAudio::IdleState: qDebug() << "Synth: IDLE"; break;
        default: break;
        }
    });
}

SynthEngine::~SynthEngine() {
    stop();
    delete m_audioSink;
}

// Tell Qt we are a live stream
bool SynthEngine::isSequential() const {
    return true;
}

// --- THE FIX IS HERE ---
qint64 SynthEngine::bytesAvailable() const {
    // If we are closed (Stopped), we have NO data.
    // This stops the infinite loop of "Read Error" messages.
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
    m_oscillator = func;
}

qint64 SynthEngine::readData(char *data, qint64 maxlen) {
    int channels = m_format.channelCount();

    if (m_format.sampleFormat() == QAudioFormat::Float) {
        float *buffer = reinterpret_cast<float*>(data);
        int frames = maxlen / (sizeof(float) * channels);

        for (int i = 0; i < frames; ++i) {
            double t = (double)m_totalSamples / m_format.sampleRate();
            float sample = (float)m_oscillator(t) * 0.5f;

            for (int c = 0; c < channels; ++c) {
                *buffer++ = sample;
            }
            m_totalSamples++;
        }
        return frames * sizeof(float) * channels;
    }

    // Fallback for Integer Audio
    else if (m_format.sampleFormat() == QAudioFormat::Int16) {
        qint16 *buffer = reinterpret_cast<qint16*>(data);
        int frames = maxlen / (sizeof(qint16) * channels);

        for (int i = 0; i < frames; ++i) {
            double t = (double)m_totalSamples / m_format.sampleRate();
            float raw = (float)m_oscillator(t) * 0.5f;
            qint16 sample = static_cast<qint16>(raw * 32767.0f);

            for (int c = 0; c < channels; ++c) {
                *buffer++ = sample;
            }
            m_totalSamples++;
        }
        return frames * sizeof(qint16) * channels;
    }

    return 0;
}

qint64 SynthEngine::writeData(const char *data, qint64 len) {
    Q_UNUSED(data); Q_UNUSED(len); return 0;
}
