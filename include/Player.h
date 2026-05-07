#if !defined(PLAYER_H)
#define PLAYER_H

#include <Arduino.h>
#include <AudioTools.h>
#include "Settings.h"

// Pipeline setup:
//   Signal generator pipeline:
//       Signal generator (sine, square, sawtooth) -> GeneratedSoundStream -> VolumeStream -> Mixer
//   Output pipeline:
//       Mixer -> VolumeStream -> VolumeMeter -> I2SStream

#define MEASURE_THROUGHPUT
#define AUDIO_TOOLS_LOG_LEVEL AudioToolsLogLevel::Warning
#define NUM_MIXER_CHANNELS 1

class Player: public AudioInfoSupport {
public:
    enum Waveform {
        SINE,
        SQUARE,
        SAWTOOTH
    };

    static Player inst;

    bool start();
    bool step();
    bool stop();

    AudioInfo audioInfo() override { return info_; }
    void setAudioInfo(AudioInfo from) override;

    // Accessing the signal generator
    void setSignalGeneratorWaveform(Waveform wf);
    void setSignalGeneratorFrequency(float freq);
    void setSignalGeneratorVolume(float vol) { sigGenVolume_.setVolume(vol); }

protected:
    Player();
    AudioInfo info_ = DEFAULT_AUDIO_INFO;

    // Signal generators and streams
    SineFromTable<int16_t> sineGen_;
    SquareWaveGenerator<int16_t> squareGen_;
    SawToothGenerator<int16_t> sawtoothGen_;
    GeneratedSoundStream<int16_t> sigGenStream_;
    VolumeStream sigGenVolume_;

    InputMixer<int16_t> mixer_;
    VolumeStream mixerVolume_;
    VolumeMeter volumeMeter_;
    StreamCopy mixerCopiers_[NUM_MIXER_CHANNELS];

#if defined(MEASURE_THROUGHPUT)
    MeasuringStream meas_;
#endif
    StreamCopy finalCopier_;

    I2SStream i2s_;    

    bool running_ = false;
};

#endif // PLAYER_H