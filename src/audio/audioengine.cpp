#include "audioengine.h"
#include "opusencoder.h"
#include "../network/protocol.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QDebug>
#include <cmath>

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent), m_audioSink(nullptr), m_audioSinkDevice(nullptr), m_audioSource(nullptr),
      m_audioSourceDevice(nullptr), m_opusEncoder(new OpusEncoder(nullptr)), m_micPollTimer(nullptr) {

    m_opusEncoder->initialize(12000, 1);

    // Output format: K4 uses 12kHz stereo Float32 PCM (L=Main RX, R=Sub RX)
    m_outputFormat.setSampleRate(12000);
    m_outputFormat.setChannelCount(2);
    m_outputFormat.setSampleFormat(QAudioFormat::Float);

    // Input format: Use native 48kHz for microphone capture (most hardware supports this)
    // We'll resample to 12kHz before encoding for K4 TX
    m_inputFormat.setSampleRate(48000);
    m_inputFormat.setChannelCount(1);
    m_inputFormat.setSampleFormat(QAudioFormat::Float);

    // Timers are children of AudioEngine so moveToThread() moves them too
    m_micPollTimer = new QTimer(this);
    m_micPollTimer->setInterval(10); // Poll every 10ms for low latency
    connect(m_micPollTimer, &QTimer::timeout, this, &AudioEngine::onMicDataReady);

    m_feedTimer = new QTimer(this);
    m_feedTimer->setInterval(FEED_INTERVAL_MS);
    connect(m_feedTimer, &QTimer::timeout, this, &AudioEngine::feedAudioDevice);

    // Size the microphone hot-path buffers before the first PTT.
    m_micBuffer.reserve(2 * 1440 * static_cast<int>(sizeof(qint16)));
    m_resampleBuf12k.reserve(INPUT_BUFFER_SIZE / 4);
    m_feedBatch.reserve(4);

    m_mediaDevices = new QMediaDevices(this);
    connect(m_mediaDevices, &QMediaDevices::audioInputsChanged, this, &AudioEngine::onSystemDefaultInputChanged);
    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &AudioEngine::onSystemDefaultOutputChanged);
}

AudioEngine::~AudioEngine() {
    stop();
    delete m_opusEncoder;
    m_opusEncoder = nullptr;
}

bool AudioEngine::start() {
    bool outputOk = setupAudioOutput();

    if (outputOk) {
        m_feedTimer->start();
    }

    // Keep input setup lazy. Opening it with the first PTT prevents Android's
    // audio backend from being torn down and renegotiated on every hold.

    return outputOk;
}

void AudioEngine::stop() {
    // Stop feed timer and clear jitter buffer
    if (m_feedTimer) {
        m_feedTimer->stop();
    }
    {
        QMutexLocker lock(&m_queueMutex);
        m_audioQueue.clear();
        m_queueBytes = 0;
        m_prebuffering = true;
    }
    m_writeBuffer.clear();

    m_pttActive.store(false, std::memory_order_release);
    m_txSequence = 0;
    closeMic();

    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
        m_audioSinkDevice = nullptr;
    }

    if (m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
        m_audioSourceDevice = nullptr;
    }

    m_micBuffer.clear();
    m_micReadOffset = 0;
}

bool AudioEngine::setupAudioOutput() {
    // Find the output device - use selected device or fall back to default
    QAudioDevice outputDevice;

    if (!m_selectedOutputDeviceId.isEmpty()) {
        // Try to find the selected device
        for (const QAudioDevice &device : QMediaDevices::audioOutputs()) {
            if (device.id() == m_selectedOutputDeviceId) {
                outputDevice = device;
                break;
            }
        }
    }

    // Fall back to default if selected device not found
    if (outputDevice.isNull()) {
        outputDevice = QMediaDevices::defaultAudioOutput();
    }

    if (outputDevice.isNull()) {
        qWarning() << "AudioEngine: No audio output device available";
        return false;
    }

    if (!outputDevice.isFormatSupported(m_outputFormat)) {
        qWarning() << "AudioEngine: 12kHz output format not supported by device";
        return false;
    }

    m_activeOutputDeviceId = outputDevice.id();
    m_audioSink = new QAudioSink(outputDevice, m_outputFormat, this);
    m_audioSink->setBufferSize(OUTPUT_BUFFER_SIZE);

    m_audioSinkDevice = m_audioSink->start();
    if (!m_audioSinkDevice) {
        qWarning() << "AudioEngine: Failed to start audio output";
        delete m_audioSink;
        m_audioSink = nullptr;
        return false;
    }

    // Receiver volume is applied by the per-channel mixer. Keep the platform
    // sink at unity, matching current QK4 mainline.
    m_audioSink->setVolume(1.0f);
    flushQueue();

    return true;
}

bool AudioEngine::setupAudioInput() {
    // Find the input device - use selected device or fall back to default
    QAudioDevice inputDevice;

    if (!m_selectedMicDeviceId.isEmpty()) {
        // Try to find the selected device
        for (const QAudioDevice &device : QMediaDevices::audioInputs()) {
            if (device.id() == m_selectedMicDeviceId) {
                inputDevice = device;
                break;
            }
        }
    }

    // Fall back to default if selected device not found
    if (inputDevice.isNull()) {
        inputDevice = QMediaDevices::defaultAudioInput();
    }

    if (inputDevice.isNull()) {
        qWarning() << "AudioEngine: No audio input device available";
        return false;
    }

    if (!inputDevice.isFormatSupported(m_inputFormat)) {
        qWarning() << "AudioEngine: 48kHz input format not supported by device";
        return false;
    }

    m_activeMicDeviceId = inputDevice.id();
    m_audioSource = new QAudioSource(inputDevice, m_inputFormat, this);
    m_audioSource->setBufferSize(INPUT_BUFFER_SIZE);

    // Don't start mic by default - user must enable
    return true;
}

void AudioEngine::enqueueAudio(const QByteArray &pcmData) {
    if (pcmData.isEmpty())
        return;

    QMutexLocker lock(&m_queueMutex);

    const int pktBytes = pcmData.size();

    // Mainline's self-correcting jitter buffer: after a transient stall,
    // discard oldest audio until latency returns to the normal target depth.
    const int highWaterBytes = pktBytes * 5;
    const int targetBytes = pktBytes * 2;
    if (m_queueBytes + pktBytes > highWaterBytes) {
        while (m_queueBytes > targetBytes && !m_audioQueue.isEmpty())
            m_queueBytes -= m_audioQueue.dequeue().size();
    }

    while (m_queueBytes + pktBytes > MAX_QUEUE_BYTES && !m_audioQueue.isEmpty()) {
        m_queueBytes -= m_audioQueue.dequeue().size();
    }

    m_audioQueue.enqueue(pcmData);
    m_queueBytes += pktBytes;
}

void AudioEngine::flushQueue() {
    QMutexLocker lock(&m_queueMutex);
    m_audioQueue.clear();
    m_queueBytes = 0;
    m_prebuffering = true;
    m_writeBuffer.clear();
}

void AudioEngine::feedAudioDevice() {
    if (!m_audioSinkDevice)
        return;

    // Drain any leftover write buffer from a previous partial write
    if (!m_writeBuffer.isEmpty()) {
        int bytesFree = m_audioSink->bytesFree();
        if (bytesFree > 0) {
            qint64 toWrite = qMin(static_cast<qint64>(m_writeBuffer.size()), static_cast<qint64>(bytesFree));
            qint64 written = m_audioSinkDevice->write(m_writeBuffer.constData(), toWrite);
            if (written > 0)
                m_writeBuffer.remove(0, static_cast<int>(written));
        }
        if (!m_writeBuffer.isEmpty())
            return; // Still have leftover — don't pull more from queue yet
    }

    // Query sink capacity (audio-thread-only, no mutex needed)
    int bytesFree = m_audioSink->bytesFree();

    // Reuse the batch instead of allocating at the 100 Hz feed rate.
    m_feedBatch.clear();
    int preDrainQueueBytes = 0;
    bool snapshotPrebuffering = true;
    {
        QMutexLocker lock(&m_queueMutex);

        if (m_audioQueue.isEmpty())
            return;

        // Wait for at least one packet before starting playback
        if (m_prebuffering) {
            if (m_audioQueue.size() < PREBUFFER_PACKETS)
                return;
            m_prebuffering = false;
        }

        preDrainQueueBytes = m_queueBytes;
        snapshotPrebuffering = m_prebuffering;

        // Drain packets that fit in the sink's free space
        while (!m_audioQueue.isEmpty()) {
            int headSize = m_audioQueue.head().size();
            if (bytesFree < headSize)
                break;

            QByteArray pkt = m_audioQueue.dequeue();
            m_queueBytes -= pkt.size();
            bytesFree -= headSize;
            m_feedBatch.append(std::move(pkt));
        }
    }

    emit bufferStatus(preDrainQueueBytes, MAX_QUEUE_BYTES, snapshotPrebuffering);

    // Apply mix/volume and write to audio sink without holding the lock
    for (QByteArray &packet : m_feedBatch) {
        applyMixAndVolume(packet);
        qint64 written = m_audioSinkDevice->write(packet.constData(), packet.size());
        if (written < packet.size()) {
            // Partial write — save remainder for next feed cycle
            m_writeBuffer.append(packet.constData() + written, packet.size() - static_cast<int>(written));
        }
    }
}

// Compute one output channel's mix from main/sub sources
static inline float mixChannel(float mainSample, float subSample, AudioEngine::MixSource src, float mainVol,
                               float subVol) {
    switch (src) {
    case AudioEngine::MixA:
        return mainSample * mainVol;
    case AudioEngine::MixB:
        return subSample * subVol;
    case AudioEngine::MixAB:
        return mainSample * mainVol + subSample * subVol;
    case AudioEngine::MixNegA:
        return -mainSample * mainVol;
    }
    return 0.0f;
}

void AudioEngine::applyMixAndVolume(QByteArray &packet) {
    float *samples = reinterpret_cast<float *>(packet.data());
    int totalFloats = packet.size() / sizeof(float);
    int sampleCount = totalFloats / 2;

    // Load atomic/guarded values once per packet (not per sample)
    const float mainVol = m_mainVolume.load(std::memory_order_relaxed);
    const float subVol = m_subVolume.load(std::memory_order_relaxed);
    const bool subMuted = m_subMuted.load(std::memory_order_relaxed);
    const int balMode = m_balanceMode.load(std::memory_order_relaxed);
    const int balOffset = m_balanceOffset.load(std::memory_order_relaxed);

    MixSource mixL, mixR;
    {
        QMutexLocker lock(&m_mixMutex);
        mixL = m_mixLeft;
        mixR = m_mixRight;
    }

    // Pre-compute BL balance gains (BAL mode only, applied after MX routing)
    float balLeftGain = 1.0f, balRightGain = 1.0f;
    if (balMode == 1) {
        balLeftGain = qBound(0.0f, (50.0f - balOffset) / 50.0f, 1.0f);
        balRightGain = qBound(0.0f, (50.0f + balOffset) / 50.0f, 1.0f);
    }

    for (int i = 0; i < sampleCount; i++) {
        float mainSample = samples[i * 2];    // Left channel (Main RX / VFO A)
        float subSample = samples[i * 2 + 1]; // Right channel (Sub RX / VFO B)

        // Step 1: SUB RX off — both channels get main audio only, sub slider has no effect
        // BL balance still applies (L/R gain is independent of SUB RX state)
        if (subMuted) {
            float s = mainSample * mainVol;
            samples[i * 2] = qBound(-1.0f, s * balLeftGain, 1.0f);
            samples[i * 2 + 1] = qBound(-1.0f, s * balRightGain, 1.0f);
            continue;
        }

        // Step 2: SUB RX on — apply MX routing
        float left, right;
        if (balMode == 0) {
            // NOR mode: main slider controls main, sub slider controls sub
            left = mixChannel(mainSample, subSample, mixL, mainVol, subVol);
            right = mixChannel(mainSample, subSample, mixR, mainVol, subVol);
        } else {
            // BAL mode: mainVolume controls both receivers (sub slider repurposed as balance)
            left = mixChannel(mainSample, subSample, mixL, mainVol, mainVol);
            right = mixChannel(mainSample, subSample, mixR, mainVol, mainVol);

            // Step 3: Apply BL balance (L/R gain adjustment after MX routing)
            left *= balLeftGain;
            right *= balRightGain;
        }

        // Step 4: Clamp
        samples[i * 2] = qBound(-1.0f, left, 1.0f);
        samples[i * 2 + 1] = qBound(-1.0f, right, 1.0f);
    }
}

void AudioEngine::openMic() {
    // Idempotent: after the first PTT, subsequent presses must be instant.
    if (m_micEnabled.load(std::memory_order_relaxed) && m_audioSourceDevice)
        return;

    if (!m_audioSource && !setupAudioInput()) {
        qWarning() << "AudioEngine: microphone input is unavailable";
        return;
    }

    m_audioSourceDevice = m_audioSource->start();
    if (!m_audioSourceDevice) {
        qWarning() << "AudioEngine: Failed to start microphone device";
        return;
    }

    m_micEnabled.store(true, std::memory_order_relaxed);
    m_micPollTimer->start();
}

void AudioEngine::closeMic() {
    if (!m_micEnabled.exchange(false, std::memory_order_acq_rel))
        return;

    m_micPollTimer->stop();
    if (m_audioSource)
        m_audioSource->stop();
    m_audioSourceDevice = nullptr;
    m_micBuffer.clear();
    m_micReadOffset = 0;
}

void AudioEngine::setPttActive(bool active) {
    m_pttActive.store(active, std::memory_order_release);
    if (active) {
        m_txSequence = 0;
        openMic();
        m_micBuffer.clear();
        m_micReadOffset = 0;
    }
    // On release capture remains running; complete frames are consumed but
    // not sent. This is the current QK4 mainline lifecycle.
}

void AudioEngine::setEncodeMode(int mode) {
    m_encodeMode.store(qBound(0, mode, 3), std::memory_order_relaxed);
}

void AudioEngine::setFrameSamples(int samples) {
    m_frameSamples.store(samples, std::memory_order_relaxed);
}

const QByteArray &AudioEngine::resample48kTo12k(const QByteArray &input48k) {
    // Simple 4:1 decimation with averaging filter (48kHz / 4 = 12kHz).
    const float *inputSamples = reinterpret_cast<const float *>(input48k.constData());
    int inputCount = input48k.size() / sizeof(float);
    int outputCount = inputCount / 4;
    m_resampleBuf12k.resize(outputCount * static_cast<int>(sizeof(float)));
    float *output = reinterpret_cast<float *>(m_resampleBuf12k.data());

    for (int i = 0; i < outputCount; i++) {
        // Average 4 samples for simple low-pass filtering
        int srcIdx = i * 4;
        float sum = 0.0f;
        int count = 0;
        for (int j = 0; j < 4 && (srcIdx + j) < inputCount; j++) {
            sum += inputSamples[srcIdx + j];
            count++;
        }
        output[i] = (count > 0) ? (sum / count) : 0.0f;
    }
    return m_resampleBuf12k;
}

void AudioEngine::onMicDataReady() {
    if (!m_audioSourceDevice || !m_micEnabled.load(std::memory_order_relaxed))
        return;

    QByteArray data48k = m_audioSourceDevice->readAll();
    if (data48k.isEmpty()) {
        // No data available yet - this is normal, just wait for next poll
        return;
    }

    // Resample from 48kHz to 12kHz into the reusable hot-path buffer.
    const QByteArray &data12k = resample48kTo12k(data48k);

    // Convert Float32 to S16LE, apply gain, and buffer for frame-based emission
    const float *floatData = reinterpret_cast<const float *>(data12k.constData());
    int floatSamples = data12k.size() / sizeof(float);

    const float gain = m_micGain.load(std::memory_order_relaxed);

    // Convert and append to the frame buffer with gain applied.
    for (int i = 0; i < floatSamples; i++) {
        float sample = qBound(-1.0f, floatData[i] * gain, 1.0f);
        qint16 s16Sample = static_cast<qint16>(sample * 32767.0f);
        m_micBuffer.append(reinterpret_cast<const char *>(&s16Sample), sizeof(qint16));
    }

    const int frameSamples = m_frameSamples.load(std::memory_order_relaxed);
    const int frameBytes = frameSamples * static_cast<int>(sizeof(qint16));
    const bool pttActive = m_pttActive.load(std::memory_order_acquire);
    const int encodeMode = m_encodeMode.load(std::memory_order_relaxed);

    while (m_micBuffer.size() - m_micReadOffset >= frameBytes) {
        if (pttActive) {
            const QByteArray frame = QByteArray::fromRawData(m_micBuffer.constData() + m_micReadOffset, frameBytes);
            encodeAndSendFrame(frame, frameSamples, encodeMode);
        }
        m_micReadOffset += frameBytes;
    }
    if (m_micReadOffset > 0 && m_micReadOffset * 2 >= m_micBuffer.size()) {
        m_micBuffer.remove(0, m_micReadOffset);
        m_micReadOffset = 0;
    }
}

void AudioEngine::encodeAndSendFrame(const QByteArray &s16leData, int frameSamples, int encodeMode) {
    QByteArray audioData;
    const qint16 *samples = reinterpret_cast<const qint16 *>(s16leData.constData());
    const int sampleCount = s16leData.size() / static_cast<int>(sizeof(qint16));

    switch (encodeMode) {
    case 0: { // EM0: stereo Float32
        audioData.resize(sampleCount * 2 * static_cast<int>(sizeof(float)));
        float *output = reinterpret_cast<float *>(audioData.data());
        for (int i = 0; i < sampleCount; ++i) {
            const float normalized = static_cast<float>(samples[i]) / 32768.0f;
            output[i * 2] = normalized;
            output[i * 2 + 1] = normalized;
        }
        break;
    }
    case 1: { // EM1: stereo S16LE
        audioData.resize(sampleCount * 2 * static_cast<int>(sizeof(qint16)));
        qint16 *output = reinterpret_cast<qint16 *>(audioData.data());
        for (int i = 0; i < sampleCount; ++i) {
            output[i * 2] = samples[i];
            output[i * 2 + 1] = samples[i];
        }
        break;
    }
    case 2:
    case 3:
    default:
        audioData = m_opusEncoder ? m_opusEncoder->encode(s16leData, frameSamples) : QByteArray();
        break;
    }

    if (!audioData.isEmpty())
        emit txPacketReady(Protocol::buildAudioPacket(audioData, m_txSequence++, encodeMode, frameSamples));
}

void AudioEngine::setMainVolume(float volume) {
    m_mainVolume.store(qBound(0.0f, volume, 1.0f), std::memory_order_relaxed);
}

void AudioEngine::setSubVolume(float volume) {
    m_subVolume.store(qBound(0.0f, volume, 1.0f), std::memory_order_relaxed);
}

void AudioEngine::setSubMuted(bool muted) {
    m_subMuted.store(muted, std::memory_order_relaxed);
}

void AudioEngine::setAudioMix(int left, int right) {
    QMutexLocker lock(&m_mixMutex);
    m_mixLeft = static_cast<MixSource>(qBound(0, left, 3));
    m_mixRight = static_cast<MixSource>(qBound(0, right, 3));
}

void AudioEngine::setBalanceMode(int mode) {
    m_balanceMode.store(qBound(0, mode, 1), std::memory_order_relaxed);
}

void AudioEngine::setBalanceOffset(int offset) {
    m_balanceOffset.store(qBound(-50, offset, 50), std::memory_order_relaxed);
}

void AudioEngine::setMicGain(float gain) {
    const float cubic = gain * gain * gain;
    m_micGain.store(qBound(0.0f, cubic, 1.0f), std::memory_order_relaxed);
}

void AudioEngine::setMicDevice(const QString &deviceId) {
    if (m_selectedMicDeviceId != deviceId) {
        m_selectedMicDeviceId = deviceId;

        bool wasEnabled = m_micEnabled.load(std::memory_order_relaxed);
        if (wasEnabled)
            closeMic();

        // Recreate the audio source with the new device
        if (m_audioSource) {
            delete m_audioSource;
            m_audioSource = nullptr;
        }
        // setupAudioInput remains lazy, matching mainline's device lifecycle.
        if (wasEnabled)
            openMic();
    }
}

QString AudioEngine::micDeviceId() const {
    return m_selectedMicDeviceId;
}

QList<QPair<QString, QString>> AudioEngine::availableInputDevices() {
    QList<QPair<QString, QString>> devices;

    // Add "System Default" as the first option
    devices.append(qMakePair(QString(""), QString("System Default")));

    // Add all available input devices
    for (const QAudioDevice &device : QMediaDevices::audioInputs()) {
        devices.append(qMakePair(QString(device.id()), device.description()));
    }

    return devices;
}

void AudioEngine::setOutputDevice(const QString &deviceId) {
    if (m_selectedOutputDeviceId != deviceId) {
        m_selectedOutputDeviceId = deviceId;

        // Restart audio output with the new device if currently running
        if (m_audioSink) {
            m_audioSink->stop();
            delete m_audioSink;
            m_audioSink = nullptr;
            m_audioSinkDevice = nullptr;

            setupAudioOutput();
        }
    }
}

void AudioEngine::onSystemDefaultInputChanged() {
    if (!m_selectedMicDeviceId.isEmpty() || !m_audioSource)
        return;

    const QString newDefault = QMediaDevices::defaultAudioInput().id();
    if (newDefault.isEmpty() || newDefault == m_activeMicDeviceId)
        return;

    const bool wasOpen = m_micEnabled.load(std::memory_order_relaxed);
    if (wasOpen)
        closeMic();
    delete m_audioSource;
    m_audioSource = nullptr;
    if (wasOpen)
        openMic();
}

void AudioEngine::onSystemDefaultOutputChanged() {
    if (!m_selectedOutputDeviceId.isEmpty() || !m_audioSink)
        return;

    const QString newDefault = QMediaDevices::defaultAudioOutput().id();
    if (newDefault.isEmpty() || newDefault == m_activeOutputDeviceId)
        return;

    m_audioSink->stop();
    delete m_audioSink;
    m_audioSink = nullptr;
    m_audioSinkDevice = nullptr;
    setupAudioOutput();
}

QString AudioEngine::outputDeviceId() const {
    return m_selectedOutputDeviceId;
}

QList<QPair<QString, QString>> AudioEngine::availableOutputDevices() {
    QList<QPair<QString, QString>> devices;

    // Add "System Default" as the first option
    devices.append(qMakePair(QString(""), QString("System Default")));

    // Add all available output devices
    for (const QAudioDevice &device : QMediaDevices::audioOutputs()) {
        devices.append(qMakePair(QString(device.id()), device.description()));
    }

    return devices;
}
