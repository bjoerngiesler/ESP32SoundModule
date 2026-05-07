#include "SignalGenerator.h"
#include "Mixer.h"

SignalGenerator SignalGenerator::inst;

SignalGenerator::SignalGenerator(): copier_(AUDIO_BUFFER_SIZE), outputBuffer_(AUDIO_BUFFER_SIZE) {
}

void SignalGenerator::start(float frequency, Waveform waveform) {
    frequency_ = frequency;
    waveform_ = waveform;

    Mixer::inst.lockAudioChange();
    switch(waveform_) {
    case SINE:
        sineGen_.begin(Mixer::inst.audioInfo());
        sineGen_.setFrequency(frequency_);
        sineGen_.setAmplitude(0.8*65534);
        signalGeneratorStream_.setInput(sineGen_);
        break;
    case SQUARE:
        squareGen_.begin(Mixer::inst.audioInfo());
        squareGen_.setFrequency(frequency_);
        squareGen_.setAmplitude(0.8*65534);
        signalGeneratorStream_.setInput(squareGen_);
        break;
    case SAWTOOTH:
    default:
        sawtoothGen_.begin(Mixer::inst.audioInfo());
        sawtoothGen_.setFrequency(frequency_);
        sawtoothGen_.setAmplitude(0.8*65534);
        signalGeneratorStream_.setInput(sawtoothGen_);
        break;
    }

    volume_.setStream(signalGeneratorStream_);
    volume_.setAudioInfo(Mixer::inst.audioInfo());
    volume_.begin();

    outputBuffer_.setStream(volume_);
    outputBuffer_.setAudioInfo(Mixer::inst.audioInfo());
    outputBuffer_.begin();

    signalGeneratorStream_.begin(Mixer::inst.audioInfo());

    Mixer::inst.unlockAudioChange();
}

void SignalGenerator::step() {
    return;
}

void SignalGenerator::stop() {
    Mixer::inst.lockAudioChange();
    signalGeneratorStream_.end();
    Mixer::inst.unlockAudioChange();
}