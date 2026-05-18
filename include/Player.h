#if !defined(PLAYER_H)
#define PLAYER_H

#include <string>

#include <LibBB.h>
using namespace bb;

#include <Arduino.h>
#include <AudioTools.h>
#include <AudioTools/Disk/AudioSourceSD.h>
#include <AudioTools/AudioCodecs/CodecWAV.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Communication/HTTP/URLStreamESP32.h>
#include "Settings.h"
#include "MemFile.h"

// Pipeline setup:
//   Signal generator pipeline:
//       Signal generator (sine, square, sawtooth) -> GeneratedSoundStream -> VolumeStream -> Mixer
//   Output pipeline:
//       Mixer -> VolumeStream -> VolumeMeter -> I2SStream

#define MULTITHREAD

class Player: public AudioInfoSupport, virtual public bb::Subsystem {
public:
    enum Waveform {
        SINE,
        SQUARE,
        SAWTOOTH
    };

    enum PlayMode {
        STOPPED,
        FROM_FILE,
        FROM_URL,
        FROM_MEMORY
    };

    static Player inst;

    Result initialize() override;
    Result start(ConsoleStream *stream=nullptr) override;
    Result step() override;
    Result stop(ConsoleStream *stream=nullptr) override;

    AudioInfo audioInfo() override { return info_; }
    void setAudioInfo(AudioInfo from) override;

    void clearEffects();

    void setDelay(bool onoff);
    void setDelayDepth(float depth);
    void setDelayFeedback(float feedback);
    void setDelayTime(float time);

    // Accessing the signal generator
    void setSignalGeneratorEnabled(bool enabled);
    void setSignalGeneratorWaveform(Waveform wf);
    void setSignalGeneratorFrequency(float freq);
    void setSignalGeneratorVolume(float vol);

    // Accessing the file player
    bool playFile(const String& path);
    bool playMemFile(const MemFile& file);
    bool playEncodedStream(Stream& stream, AudioDecoder& dec);

    void stopPlayback();
    bool pausePlayback(bool onoff);
    bool isPaused() { return paused_; }
    PlayMode playMode() { return playMode_; }
    bool isPlaying() { return playMode_ != STOPPED; }
    void setFileVolume(float vol) { fileVolume_.setVolume(constrain(vol, 0.0, 1.0)); }

    // Controlling the overall volume
    void setOutputVolume(float vol) { mixerVolume_.setVolume(constrain(vol, 0.0, 1.0)); }
    float outputVolume() { return mixerVolume_.volume(); }
    float measuredOutputVolume() { return volumeMeter_.volumeRatio(); }

    Result playCmd(const std::vector<String>& args, ConsoleStream *stream);
    Result pauseCmd(const std::vector<String>& args, ConsoleStream *stream);
    Result sigGenCmd(const std::vector<String>& args, ConsoleStream *stream);
    Result volumeCmd(const std::vector<String>& args, ConsoleStream *stream);

    void addOnStopCallback(std::function<bool(void)> fn);

protected:
    Player();
#if defined(MULTITHREAD)
    static void threadStatic(void* param) { Player::inst.thread(param); }
    void thread(void* param);
#endif
    void runPipeline();

    AudioInfo info_ = DEFAULT_AUDIO_INFO;

    int signalMixerIndex_ = -1;
    int fileMixerIndex_ = -1;

    // Signal generators and streams
    SineFromTable<int16_t> sineGen_;
    SquareWaveGenerator<int16_t> squareGen_;
    SawToothGenerator<int16_t> sawGen_;
    GeneratedSoundStream<int16_t> sigGenStream_;
    float sigGenVolume_ = 1.0;

    // File decoders and streams
    WAVDecoder wav_;
    MP3DecoderHelix mp3_;
    URLStreamESP32 urlStream_;

    MemoryStream *memoryStream_ = nullptr;

    NullStream silenceStream_;
    CatStream catStream_;
    EncodedAudioStream fileDecoder_;
    AudioEffectStream fileEffects_;
    PitchShift pitchshift_;
    Delay delay_;
    
    VolumeStream fileVolume_;

    InputMixer<int16_t> mixer_;
    VolumeStream mixerVolume_;
    VolumeMeter volumeMeter_;

    CsvOutput<int16_t> csv_;

#if defined(MEASURE_THROUGHPUT)
    MeasuringStream meas_;
#endif
    StreamCopy finalCopier_;

    I2SStream i2s_;    
    File audioFile_;

    PlayMode playMode_ = STOPPED;

    std::vector<std::function<bool(void)>> onStopCallbacks_;

    unsigned long lastCardCheckTime_;
    bool isSDCardPresent_;

    bool paused_;

#if defined(MULTITHREAD)
    bool threadRunning_, threadShouldStop_;
    SemaphoreHandle_t pipelineSemaphore_;
    TaskHandle_t taskHandle_;
#endif // MULTITHREAD
};

#endif // PLAYER_H