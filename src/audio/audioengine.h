#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QAudioSink>
#include <QAudioSource>
#include <QAudioFormat>
#include <QIODevice>
#include <QMediaDevices>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <atomic>

class OpusEncoder;

class AudioEngine : public QObject {
    Q_OBJECT

public:
    enum MixSource { MixA = 0, MixB = 1, MixAB = 2, MixNegA = 3 };

    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine();

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    void enqueueAudio(const QByteArray &pcmData);
    Q_INVOKABLE void flushQueue();

    // The microphone is opened once and then remains open while connected.
    // PTT controls only the packet send gate; reopening the Android audio
    // backend for each press creates gaps that make the K4 leave TX.
    Q_INVOKABLE void openMic();
    Q_INVOKABLE void closeMic();

    // PTT owns the TX encode gate on the audio thread. The capture device is
    // intentionally not stopped on release; see openMic()/closeMic().
    Q_INVOKABLE void setPttActive(bool active);
    bool isPttActive() const { return m_pttActive.load(std::memory_order_relaxed); }
    void setEncodeMode(int mode);
    void setFrameSamples(int samples);

    // Channel volume controls (applied at playback time for instant response)
    void setMainVolume(float volume);
    void setSubVolume(float volume);
    float mainVolume() const { return m_mainVolume.load(std::memory_order_relaxed); }
    float subVolume() const { return m_subVolume.load(std::memory_order_relaxed); }

    // SUB RX mute control (when sub receiver is off, sub channel is silent)
    void setSubMuted(bool muted);

    // Audio mix routing (MX command - how main/sub maps to L/R when SUB is on)
    void setAudioMix(int left, int right); // MixSource values

    // Balance mode control (0=NOR, 1=BAL)
    void setBalanceMode(int mode);
    void setBalanceOffset(int offset); // -50 to +50

    // Microphone settings
    void setMicGain(float gain); // 0.0 to 1.0
    float micGain() const { return m_micGain.load(std::memory_order_relaxed); }

    Q_INVOKABLE void setMicDevice(const QString &deviceId);
    QString micDeviceId() const;

    // Get list of available input devices (for settings UI)
    static QList<QPair<QString, QString>> availableInputDevices(); // (id, description)

    // Output device settings
    Q_INVOKABLE void setOutputDevice(const QString &deviceId);
    QString outputDeviceId() const;

    // Get list of available output devices (for settings UI)
    static QList<QPair<QString, QString>> availableOutputDevices(); // (id, description)

signals:
    void micLevelChanged(float level);                 // RMS level 0.0-1.0 for meter display
    void txPacketReady(const QByteArray &packet);       // Encoded K4 TX packet from audio thread
    void bufferStatus(int queueBytes, int maxBytes, bool prebuffering);

private slots:
    void onMicDataReady();
    void feedAudioDevice();
    void onSystemDefaultInputChanged();
    void onSystemDefaultOutputChanged();

private:
    bool setupAudioOutput();
    bool setupAudioInput();

    // Resample into a reusable buffer, avoiding allocations in the microphone hot path.
    const QByteArray &resample48kTo12k(const QByteArray &input48k);
    void encodeAndSendFrame(const QByteArray &s16leData, int frameSamples, int encodeMode);

    // Apply MX routing + volume + balance to a raw [main, sub] interleaved packet
    void applyMixAndVolume(QByteArray &packet);

    // Audio output format: 12kHz stereo Float32 (K4 RX audio, L=Main R=Sub)
    QAudioFormat m_outputFormat;

    // Audio input format: 48kHz mono Float32 (native macOS rate, resampled to 12kHz)
    QAudioFormat m_inputFormat;

    // Audio output (speaker)
    QAudioSink *m_audioSink;
    QIODevice *m_audioSinkDevice;

    // Audio input (microphone)
    QAudioSource *m_audioSource;
    QIODevice *m_audioSourceDevice;
    std::atomic<bool> m_micEnabled{false};
    QString m_selectedMicDeviceId;    // Empty = use system default
    QString m_selectedOutputDeviceId; // Empty = use system default
    QString m_activeMicDeviceId;
    QString m_activeOutputDeviceId;
    QMediaDevices *m_mediaDevices = nullptr;

    // Channel volume controls (0.0 to 1.0)
    std::atomic<float> m_mainVolume{1.0f};
    std::atomic<float> m_subVolume{1.0f};

    // SUB RX mute state (true = sub muted, sub channel is silent)
    std::atomic<bool> m_subMuted{true}; // Starts muted (SUB RX is off at startup)

    // Audio mix routing (MX command) - default A.B (main left, sub right)
    MixSource m_mixLeft = MixA;
    MixSource m_mixRight = MixB;
    QMutex m_mixMutex; // Protects m_mixLeft and m_mixRight (always set together)

    // Balance mode (0=NOR: independent volume, 1=BAL: L/R balance)
    std::atomic<int> m_balanceMode{0};
    std::atomic<int> m_balanceOffset{0}; // -50 to +50

    // Microphone gain control
    std::atomic<float> m_micGain{0.25f}; // Default 25% (macOS mic input is typically hot)

    // TX packetization runs in the audio thread.  Keeping sequence and Opus
    // state here makes packet timing independent of the UI event loop.
    OpusEncoder *m_opusEncoder = nullptr;
    quint8 m_txSequence = 0;
    std::atomic<bool> m_pttActive{false};
    std::atomic<int> m_encodeMode{3};
    // Frame size follows the connected radio's SL tier, exactly as in QK4 main.
    std::atomic<int> m_frameSamples{240};

    // Audio throughput: 12kHz × 2ch × sizeof(float) = 96,000 bytes/sec = 96 bytes/ms
    static constexpr int BYTES_PER_MS = 96;

    // Audio buffer sizes
    // QAudioSink buffer: 500ms — large enough for 4+ max-size packets (SL7 = 11,520 bytes = 120ms)
    // Ensures bytesFree() always exceeds one max packet, preventing partial writes and data loss
    static constexpr int OUTPUT_BUFFER_SIZE = 500 * BYTES_PER_MS; // 48,000 bytes
    // Input: 48kHz * 4 bytes/sample * 0.1 sec = 19200 bytes
    static constexpr int INPUT_BUFFER_SIZE = 19200;

    // Microphone frame buffering for Opus encoding. The read offset avoids an
    // O(N) front-removal on every 10 ms microphone poll.
    QByteArray m_micBuffer;
    int m_micReadOffset = 0;
    QByteArray m_resampleBuf12k;

    // Timer for polling microphone data (more reliable than readyRead signal)
    QTimer *m_micPollTimer;

    // Jitter buffer for RX audio playback
    QQueue<QByteArray> m_audioQueue;
    int m_queueBytes = 0; // Total decoded bytes in m_audioQueue (tracked for time-based thresholds)
    QMutex m_queueMutex;  // Protects m_audioQueue, m_queueBytes, m_prebuffering
    QTimer *m_feedTimer;
    bool m_prebuffering = true;

    // Write staging buffer: holds processed PCM that couldn't be written in one feed cycle
    // Audio-thread-only (no mutex needed) — safety net for partial QIODevice::write()
    QByteArray m_writeBuffer;
    QList<QByteArray> m_feedBatch;

    // Jitter buffer constants (adapt to any SL level automatically)
    // Prebuffer: start playback as soon as the first packet arrives.
    // The SL level already provides jitter tolerance (larger packets = more runway),
    // so additional prebuffering just adds latency without benefit.
    static constexpr int PREBUFFER_PACKETS = 1;
    static constexpr int MAX_QUEUE_BYTES = 1000 * BYTES_PER_MS; // 96,000 bytes (1s cap)
    static constexpr int FEED_INTERVAL_MS = 10;
};

#endif // AUDIOENGINE_H
