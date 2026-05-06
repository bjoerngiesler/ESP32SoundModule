#if !defined(FILEPLAYER_H)
#define FILEPLAYER_H

#include "Settings.h"
#include "MixerInput.h"
#include "MixedOutput.h"
#include <AudioTools.h>
#include <AudioTools/Disk/AudioSourceSD.h>
#include <AudioTools/CoreAudio/VolumeStream.h>
#include <AudioTools/AudioCodecs/CodecWAV.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Communication/HTTP/URLStreamESP32.h>
#include <AudioTools/CoreAudio/AudioEffects/SoundGenerator.h>

#include <map>
#include <functional>

// Pipeline:
// URLStreamESP32 or AudioSourceSD (file) 
//   -> EncodedAudioStream (WAV or MP3) 
// Should we decouple effects by multiplying and mixing?
// What's the correct output to isolate this and plug into a mixer pipeline?

class FilePlayer: public MixerInput, public AudioInfoSupport {
public:
    static FilePlayer inst;

    bool start();
    bool step();
    bool stop();
    bool playFile(const std::string& path);
    void stopPlayback();

    Stream& output() override { return decoder_; }

    AudioInfo audioInfo() override { return MixedOutput::inst.audioInfo(); }
    void setAudioInfo(AudioInfo from) override;

protected:
    FilePlayer();

    WAVDecoder wav_;
    MP3DecoderHelix mp3_;
    URLStreamESP32 urlStream_;
    EncodedAudioStream decoder_;
    StreamCopy copier_;

#if defined(ONLY_FILE_PLAYER)
    I2SStream i2s_;
    VolumeStream volume_;
    OutputMixer<int16_t> mixer_;
    StreamCopy mixerCopier1_, mixerCopier2_;
    BufferedStream mixerBuffer_;
#endif

    File audioFile_;

    bool playing_ = false;
};

#endif // FILEPLAYER_H