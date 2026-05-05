#if !defined(SOUNDPLAYER_H)
#define SOUNDPLAYER_H

#include "Settings.h"
#include <AudioTools.h>
#include <AudioTools/Disk/AudioSourceSD.h>
#include <AudioTools/CoreAudio/VolumeStream.h>
#include <AudioTools/AudioCodecs/CodecWAV.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Communication/HTTP/URLStreamESP32.h>
#include <AudioTools/CoreAudio/AudioEffects/SoundGenerator.h>

#include <map>
#include <functional>

class SoundPlayer {
public:
    enum Waveform {
        SINE,
        SQUARE,
        SAWTOOTH
    };

    static SoundPlayer inst;
    bool start();
    bool step();
    bool stop();
    bool playFile(const std::string& path);
    void setVolume(float volume);
    void stopPlayback();

    void enableSignalGenerator(bool enable);
    void setSignalGeneratorFrequency(float frequency);
    void setSignalGeneratorWaveform(Waveform w);

protected:
    SoundPlayer();

    I2SStream i2s_;
    WAVDecoder wav_;
    MP3DecoderHelix mp3_;
    URLStreamESP32 urlStream_;
    VolumeStream volumeStream_;

    FastSineGenerator<int16_t> sineGen_;
    SquareWaveGenerator<int16_t> squareGen_;
    SawToothGenerator<int16_t> sawtoothGen_;
    GeneratedSoundStream<int16_t> signalGeneratorStream_;

    bool signalGeneratorEnabled_ = false;
    float signalGeneratorFrequency_ = 440.0f;
    Waveform signalGeneratorWaveform_ = SINE;

    EncodedAudioStream decoder_;

    StreamCopy copier_;
    File audioFile_;

    bool playing_ = false;
};

#endif // SOUNDPLAYER_H