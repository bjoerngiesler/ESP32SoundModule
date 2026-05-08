#if !defined(PLAYER_H)
#define PLAYER_H

#include <string>
#include <Arduino.h>
#include <AudioTools.h>
#include <AudioTools/Disk/AudioSourceSD.h>
#include <AudioTools/AudioCodecs/CodecWAV.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Communication/HTTP/URLStreamESP32.h>
#include "Settings.h"

// Pipeline setup:
//   Signal generator pipeline:
//       Signal generator (sine, square, sawtooth) -> GeneratedSoundStream -> VolumeStream -> Mixer
//   Output pipeline:
//       Mixer -> VolumeStream -> VolumeMeter -> I2SStream


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
    void setSignalGeneratorVolume(float vol);

    // Accessing the file player
    bool playFile(const std::string& path);
    void stopPlayback() { playing_ = false; }
    void setFileVolume(float vol) { fileVolume_.setVolume(vol); }

    // Controlling the overall volume
    void setOutputVolume(float vol) { mixerVolume_.setVolume(vol); }
    float measuredOutputVolume() { return volumeMeter_.volumeRatio(); }

protected:
    Player();
    AudioInfo info_ = DEFAULT_AUDIO_INFO;

    // Signal generators and streams
    SineFromTable<int16_t> sineGen_;
    SquareWaveGenerator<int16_t> squareGen_;
    SawToothGenerator<int16_t> sawGen_;
    GeneratedSoundStream<int16_t> sigGenStream_;

    // File decoders and streams
    WAVDecoder wav_;
    MP3DecoderHelix mp3_;
    URLStreamESP32 urlStream_;

    EncodedAudioStream fileDecoder_;
    BufferedStream fileBuffer_;
    VolumeStream fileVolume_;
    StreamCopy fileCopier_;

    InputMixer<int16_t> mixer_;
    VolumeStream mixerVolume_;
    VolumeMeter volumeMeter_;

    CsvOutput<int16_t> csv_;

#if defined(MEASURE_THROUGHPUT)
    MeasuringStream meas_;
#endif
    StreamCopy finalCopier_;

    I2SStream i2s_;    

    bool running_ = false, playing_ = false;
    File audioFile_;
};

#endif // PLAYER_H