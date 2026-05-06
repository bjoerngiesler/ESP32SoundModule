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
    void begin(float frequency, Waveform waveform);
    void end();

    Stream& output() override { return signalGeneratorStream_; }

private:
    SignalGenerator();

    FastSineGenerator<int16_t> sineGen_;
    SquareWaveGenerator<int16_t> squareGen_;
    SawToothGenerator<int16_t> sawtoothGen_;
    GeneratedSoundStream<int16_t> signalGeneratorStream_;

    float frequency_ = 440.0f;
    Waveform waveform_ = SINE;
};

#endif // SIGNALGENERATOR_H