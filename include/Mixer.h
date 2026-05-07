#if !defined(MIXER_H)
#define MIXER_H

#include "MixerInput.h"
#include "Settings.h"
#include <Arduino.h>
#include <AudioTools.h>
#include <AudioTools/Disk/AudioSourceSD.h>
#include <AudioTools/CoreAudio/VolumeStream.h>
#include <AudioTools/CoreAudio/AudioStreams.h>
#include <AudioTools/AudioCodecs/CodecWAV.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Communication/HTTP/URLStreamESP32.h>
#include <AudioTools/CoreAudio/AudioEffects/SoundGenerator.h>

// Setup:
// <n Streams> 
//   -> InputMixer   (each stream gets a weight)
//   -> VolumeStream (master volume control)
//   -> I2SStream    (output)

class Mixer: public AudioInfoSupport {
public:
    static Mixer inst;

    bool start(bool multicore);
    bool stop();
    bool step();

    void lockAudioChange();
    void unlockAudioChange();

    AudioInfo audioInfo() override { return Mixer::inst.audioInfo(); }
    void setAudioInfo(AudioInfo from) override;

    size_t numInputs() { return mixer_.size(); }

    bool setInput(MixerInput& input, size_t ch, float weight = 1.0);
    bool removeInput(size_t ch);
    bool setWeight(size_t ch, float weight);

    void setOutputVolume(float volume) { volumeStream_.setVolume(volume); }

protected:
    Mixer();
    void loop();
    
    I2SStream i2s_;
    VolumeStream volumeStream_; // master volume
    OutputMixer<int16_t> mixer_;
    StreamCopy inputCopiers_[NUM_MIXER_INPUTS];
    BufferedStream dummyStream_;

    TaskHandle_t taskHandle_ = nullptr;
    SemaphoreHandle_t audioChangeHandle_ = nullptr;
    bool isRunning_ = false, shouldStop_ = false;

    bool multicore_ = false;

    AudioInfo info_;
};

#endif // MIXER_H