#include "SignalGenerator.h"
#include "MixedOutput.h"

SignalGenerator SignalGenerator::inst;

SignalGenerator::SignalGenerator() {
}

void SignalGenerator::begin(float frequency, Waveform waveform) {
    frequency_ = frequency;
    waveform_ = waveform;

    MixedOutput::inst.lockAudioChange();
    switch(waveform_) {
    case SINE:
        sineGen_.begin(MixedOutput::inst.audioInfo());
        sineGen_.setFrequency(frequency_);
        signalGeneratorStream_.setInput(sineGen_);
        break;
    case SQUARE:
        squareGen_.begin(MixedOutput::inst.audioInfo());
        squareGen_.setFrequency(frequency_);
        signalGeneratorStream_.setInput(squareGen_);
        break;
    case SAWTOOTH:
    default:
        sawtoothGen_.begin(MixedOutput::inst.audioInfo());
        sawtoothGen_.setFrequency(frequency_);
        signalGeneratorStream_.setInput(sawtoothGen_);
        break;
    }
    signalGeneratorStream_.begin(MixedOutput::inst.audioInfo());
    MixedOutput::inst.add(SignalGenerator::inst, weight_);
    MixedOutput::inst.unlockAudioChange();
}

void SignalGenerator::end() {
    MixedOutput::inst.lockAudioChange();
    signalGeneratorStream_.end();
    MixedOutput::inst.remove(mixerIndex_);
    MixedOutput::inst.unlockAudioChange();
}