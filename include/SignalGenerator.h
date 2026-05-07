#if !defined(SIGNALGENERATOR_H)
#define SIGNALGENERATOR_H

#include <Arduino.h>
#include "MixerInput.h"

// Stream setup:
// One out of {sine, square, sawtooth} generator
//   -> GeneratedSoundStream (wraps the generator to provide AudioInfo and byte stream)


class SignalGenerator: public MixerInput {
public:
    enum Waveform {
        SINE,
        SQUARE,
        SAWTOOTH
    };

    static SignalGenerator inst;
    void start(float frequency, Waveform waveform);
    void step();
    void stop();

    virtual Stream& output() { return outputBuffer_; }
    void setVolume(float vol) { volume_.setVolume(vol); }

private:
    SignalGenerator();

    SineWaveGenerator<int16_t> sineGen_;
    SquareWaveGenerator<int16_t> squareGen_;
    SawToothGenerator<int16_t> sawtoothGen_;
    GeneratedSoundStream<int16_t> signalGeneratorStream_;

    VolumeStream volume_;
    BufferedStream outputBuffer_;

    StreamCopy copier_;

    float frequency_ = 440.0f;
    Waveform waveform_ = SINE;
};

#endif // SIGNALGENERATOR_H