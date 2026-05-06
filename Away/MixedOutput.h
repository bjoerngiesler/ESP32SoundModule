#if !defined(MIXEDOUTPUT_H)
#define MIXEDOUTPUT_H

#include "MixerInput.h"
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

class MixedOutput {
public:
    static MixedOutput inst;

    bool start();
    bool stop();

    void lockAudioChange();
    void unlockAudioChange();

    int add(MixerInput& input, int weight = 100);
    void setWeight(int index, int weight);
    void setOutputVolume(float volume) { volumeStream_.setVolume(volume); }

protected:
    MixedOutput();
    void loop();

    I2SStream i2s_;
    VolumeStream volumeStream_; // master volume
    InputMixer<int16_t> mixer_;
    StreamCopy copier_;

    TaskHandle_t taskHandle_ = nullptr;
    SemaphoreHandle_t audioChangeHandle_ = nullptr;
    bool isRunning_ = false, shouldStop_ = false;
};

#endif // MIXEDOUTPUT_H