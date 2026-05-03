#include <Arduino.h>
#include "Settings.h"
#include "SoundPlayer.h"

SoundPlayer SoundPlayer::inst;

void printMetaData(MetaDataType type, const char* str, int len){
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
}


SoundPlayer::SoundPlayer() {
    pinMode(PIN_I2S_BCK, OUTPUT);
    pinMode(PIN_I2S_DATA_OUT, OUTPUT);
    pinMode(PIN_I2S_WS, OUTPUT);
}

bool SoundPlayer::start() {
    Serial.println("Starting SoundPlayer...");
    // Set up I2S
    auto cfg = i2s_.defaultConfig(TX_MODE);
    cfg.bits_per_sample = 16;
    cfg.sample_rate = 44100;
    cfg.channels = 1;
    cfg.pin_data = 2;
    cfg.pin_ws = 3;
    cfg.pin_bck = 4;
    cfg.is_master = true;
    i2s_.begin(cfg);

    volumeStream_.setOutput(i2s_);
    volumeStream_.setVolume(.5f);

    Serial.println("SoundPlayer started");
    return true;
}

void SoundPlayer::setVolume(float volume) {
    volumeStream_.setVolume(volume);
}

bool SoundPlayer::step() {
    if(playing_) {
        size_t numbytes = copier_.copy();
        if(numbytes == 0) {
            Serial.println("Finished playing");
            playing_ = false;
            audioFile_.close();
        } 
    }
    return true;
}

bool SoundPlayer::stop() {
    if(playing_) {
        playing_ = false;
    }
    return true;
}

void SoundPlayer::stopPlayback() {
    playing_ = false;
}

bool SoundPlayer::playFile(const std::string& path) {
    std::string p = std::string("/")+path;
    audioFile_ = SD.open(p.c_str(), FILE_READ);
    if(!audioFile_) {
        Serial.printf("Failed to open file '%s'\n", p.c_str());
        return false;
    }
    Serial.printf("Playing file '%s'\n", p.c_str());
    if(path.rfind(".wav") != std::string::npos) {
        wav_.begin();
        
        decoder_.setOutput(volumeStream_);
        decoder_.setDecoder(&wav_);
        decoder_.addNotifyAudioChange(i2s_);
        decoder_.addNotifyAudioChange(volumeStream_);
        decoder_.begin();

        copier_.begin(decoder_, audioFile_);
        
        playing_ = true;
    } else if(path.rfind(".mp3") != std::string::npos) {
        mp3_.begin();

        decoder_.setOutput(volumeStream_);
        decoder_.setDecoder(&mp3_);
        decoder_.addNotifyAudioChange(i2s_);
        decoder_.addNotifyAudioChange(volumeStream_);
        decoder_.begin();

        copier_.begin(decoder_, audioFile_);
        playing_ = true;
    } else if(path.rfind(".stream") != std::string::npos) {
        std::string url = audioFile_.readString().c_str();
        while(url.length() > 0 && (url.back() == '\n' || url.back() == '\r')) url.pop_back(); // Trim trailing newlines
        Serial.printf("Streaming from URL: '%s'\n", url.c_str());
        mp3_.begin();
        urlStream_.begin(url.c_str(), "audio/mp3");

        decoder_.setOutput(volumeStream_);
        decoder_.setDecoder(&mp3_);
        decoder_.addNotifyAudioChange(i2s_);
        decoder_.addNotifyAudioChange(volumeStream_);
        decoder_.begin();

        copier_.begin(decoder_, urlStream_);
        playing_ = true;
    } else {
        Serial.println("Unsupported file type");
        audioFile_.close();
        playing_ = false;
        return false;
    }

    return true;
#if 0
    std::string p = std::string("/")+path; 
    Serial.printf("Playing file '%s'\n", p.c_str());

    if(source_ != nullptr || decoder_) {
        player_.stop();
    }
    if(source_ != nullptr) {
        delete source_;
        source_ = nullptr;
    }
    if(decoder_ != nullptr) {
        delete decoder_;
        decoder_ = nullptr;
    }

    source_ = new AudioSourceSD("/system");
    player_.setAudioSource(*source_);
    if(path.rfind(".wav") != std::string::npos) {
        decoder_ = new WAVDecoder();
        player_.setDecoder(*decoder_);
        player_.setMetadataCallback(printMetaData);
        player_.begin();
    } 

    return true;
#endif
}
