#include <Arduino.h>
#include "Settings.h"
#include "FilePlayer.h"
#include "Mixer.h"

FilePlayer FilePlayer::inst;

#if defined(ONLY_FILE_PLAYER)
SineWaveGenerator<int16_t> sineWave1(SAMPLE_RATE);                // subclass of SoundGenerator with max amplitude of 32000
SineWaveGenerator<int16_t> sineWave2(SAMPLE_RATE);                // subclass of SoundGenerator with max amplitude of 32000
GeneratedSoundStream<int16_t> sound1(sineWave1);             // Stream generated from sine wave
GeneratedSoundStream<int16_t> sound2(sineWave2);             // Stream generated from sine wave
#endif

// Pipeline:
// URLStreamESP32 or AudioSourceSD (file) 
//   -> EncodedAudioStream (WAV or MP3) 
// Should we decouple effects by multiplying and mixing?
// What's the correct output to isolate this and plug into a mixer pipeline?

void printMetaData(MetaDataType type, const char* str, int len){
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
}

FilePlayer::FilePlayer():
    outputBuffer_(AUDIO_BUFFER_SIZE)
#if defined(ONLY_FILE_PLAYER)
,   mixer_(i2s_, 1), 
    mixerCopier1_(AUDIO_BUFFER_SIZE), 
    mixerCopier2_(AUDIO_BUFFER_SIZE), 
    copier_(AUDIO_BUFFER_SIZE), 
#endif
{
}

bool FilePlayer::start() {
#if defined(ONLY_FILE_PLAYER)
    pinMode(P_I2S_BCK, OUTPUT);
    pinMode(P_I2S_DATA_OUT, OUTPUT);
    pinMode(P_I2S_WS, OUTPUT);

    auto cfg = i2s_.defaultConfig(TX_MODE);
    cfg.setAudioInfo(DEFAULT_AUDIO_INFO);
    cfg.pin_data = P_I2S_DATA_OUT;
    cfg.pin_ws = P_I2S_WS;
    cfg.pin_bck = P_I2S_BCK;
    cfg.is_master = true;
    i2s_.begin(cfg);

    sineWave1.begin(DEFAULT_AUDIO_INFO, N_B5);
    sineWave2.begin(DEFAULT_AUDIO_INFO, N_E5);

    mixerCopier1_.begin(mixer_, sound1);
    mixerCopier2_.begin(mixer_, buffer_);

    buffer_.setOutput(mixer_);

    mixer_.setOutput(i2s_);
    mixer_.setOutputCount(2);
    mixer_.setWeight(0, 0.2f);
    mixer_.setWeight(1, 1.0f);
    volume_.setVolume(1.0f);
    volume_.begin();

#endif

    // decoder_.addNotifyAudioChange(*this);
    // decoder_.addNotifyAudioChange(volume_);
    // decoder_.begin();

#if 0
    volume_.setStream(decoder_);
    volume_.setAudioInfo(Mixer::inst.audioInfo());
    volume_.begin();
#endif

    // outputBuffer_.setStream(decoder_);
    // outputBuffer_.begin();

    return true;
}

bool FilePlayer::step() {
#if 0
    static float phase = 0;
    phase += 0.01f;
    mixer_.setWeight(0, fabs(sin(phase)));
    mixer_.setWeight(1, fabs(cos(phase+PI)));
#endif

    size_t numbytes;
#if defined(ONLY_FILE_PLAYER)
    numbytes = mixerCopier1_.copy();
    printf("Mixer copied %d bytes\n", (int)numbytes);
    numbytes = mixerCopier2_.copy();
    printf("Mixer copied %d bytes\n", (int)numbytes);
#endif

    if(!playing_) {
        printf("Not playing\n");
        return true;
    }

    numbytes = copier_.copyBytes(AUDIO_BUFFER_SIZE);
    if(numbytes == 0 && playing_ == true) {
        Serial.println("Finished playing");
        playing_ = false;
        audioFile_.close();
    } else {
        Serial.printf("FilePlayer: Copied %d bytes to 0x%x; now available %d\n", (int)numbytes, copier_.getTo());
        Serial.printf("Available on decoder (0x%x) now: %d\n", &decoder_, decoder_.available());
    }

    return true;
}

bool FilePlayer::stop() {
    if(playing_) stopPlayback();

    return true;
}

void FilePlayer::stopPlayback() {
#if defined(ONLY_FILE_PLAYER)
    i2s_.end();
#else
    //MixedOutput::inst.remove(*this);
#endif
    playing_ = false;
}

void FilePlayer::setAudioInfo(AudioInfo from) {
    printf("New audioInfo: sample_rate: %d / channels: %d / bits_per_sample: %d\n", from.sample_rate, from.channels, from.bits_per_sample);
    printf("FilePlayer audioInfo: sample_rate: %d / channels: %d / bits_per_sample: %d\n", info_.sample_rate, info_.channels, info_.bits_per_sample);
    volume_.setAudioInfo(from);
    outputBuffer_.setAudioInfo(from);
    Mixer::inst.setAudioInfo(from);
    info_ = from;
}

bool FilePlayer::playFile(const std::string& path) {
    if(playing_) stopPlayback();
        
    std::string p = std::string("/")+path;
    audioFile_ = SD.open(p.c_str(), FILE_READ);
    if(!audioFile_) {
        Serial.printf("Failed to open file '%s'\n", p.c_str());
        return false;
    }
    Serial.printf("Playing file '%s'\n", p.c_str());
    if(path.rfind(".wav") != std::string::npos) {
        wav_.begin();
        
        decoder_.setDecoder(&wav_);
        decoder_.addNotifyAudioChange(FilePlayer::inst);
        decoder_.begin();
        copier_.begin(decoder_, audioFile_);
        playing_ = true;
#if defined(ONLY_FILE_PLAYER)
        volume_.setOutput(mixerBuffer_);
        mixer_.begin(BUFFER_SIZE);
        decoder_.addNotifyAudioChange(sound1);
        decoder_.addNotifyAudioChange(sound2);
        decoder_.addNotifyAudioChange(i2s_);
#else
        //MixedOutput::inst.add(*this, weight());
#endif
    } else if(path.rfind(".mp3") != std::string::npos) {
        mp3_.begin();

        decoder_.setDecoder(&mp3_);
#if defined(ONLY_FILE_PLAYER)
        decoder_.setOutput(volume_);
        decoder_.addNotifyAudioChange(volume_);
        decoder_.addNotifyAudioChange(i2s_);
#else
        //MixedOutput::inst.add(*this, weight());
#endif
        decoder_.begin();
        copier_.begin(decoder_, audioFile_);
        //MixedOutput::inst.add(*this, weight());
        playing_ = true;
    } else if(path.rfind(".stream") != std::string::npos) {
        std::string url = audioFile_.readString().c_str();
        while(url.length() > 0 && (url.back() == '\n' || url.back() == '\r')) url.pop_back(); // Trim trailing newlines
        Serial.printf("Streaming from URL: '%s'\n", url.c_str());
        mp3_.begin();
        urlStream_.begin(url.c_str(), "audio/mp3");

        decoder_.setDecoder(&mp3_);
        decoder_.addNotifyAudioChange(*this);
#if defined(ONLY_FILE_PLAYER)
        decoder_.setOutput(volume_);
        decoder_.addNotifyAudioChange(volume_);
#else
        //MixedOutput::inst.add(*this, weight());
#endif
        decoder_.begin(Mixer::inst.audioInfo());
        copier_.begin(decoder_, urlStream_);
        playing_ = true;
    } else {
        Serial.println("Unsupported file type");
        audioFile_.close();
        playing_ = false;
        return false;
    }

    return true;
}
