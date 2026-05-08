#include "Player.h"
#include "Settings.h"

Player Player::inst;

#if defined(MEASURE_THROUGHPUT)
#define OUTPUT_STREAM meas_
#else
#if defined(OUTPUT_CSV)
#define OUTPUT_STREAM csv_
#else
#define OUTPUT_STREAM i2s_
#endif
#endif

Player::Player():
#if defined(MEASURE_THROUGHPUT)
    meas_(20, &Serial),
#endif
    fileCopier_(AUDIO_BUFFER_SIZE),
    finalCopier_(AUDIO_BUFFER_SIZE),
    fileBuffer_(16*AUDIO_BUFFER_SIZE)
{
    pinMode(LED_BUILTIN, OUTPUT);
}

bool Player::start() {
    pinMode(P_I2S_BCK, OUTPUT);
    pinMode(P_I2S_DATA_OUT, OUTPUT);
    pinMode(P_I2S_WS, OUTPUT);

    AudioToolsLogger.begin(Serial, AUDIO_TOOLS_LOG_LEVEL);

    // set up signal path back to front.
    // i2s_ takes its data from finalCopier_ => no stream connection.
    auto cfg = i2s_.defaultConfig(TX_MODE);
    cfg.setAudioInfo(info_);
    cfg.pin_data = P_I2S_DATA_OUT;
    cfg.pin_ws = P_I2S_WS;
    cfg.pin_bck = P_I2S_BCK;
    cfg.is_master = true;
    i2s_.begin(cfg);
    
#if defined(MEASURE_THROUGHPUT)
    meas_.setStream(volumeMeter_);
#endif
    volumeMeter_.setStream(mixerVolume_);
    mixerVolume_.setStream(mixer_);

    mixer_.add(sigGenStream_);
    mixer_.add(fileBuffer_);

    sigGenStream_.setInput(sineGen_);

    fileDecoder_.setOutput(fileVolume_);
    fileVolume_.setOutput(fileBuffer_);

    fileDecoder_.addNotifyAudioChange(*this);

#if defined(MEASURE_THROUGPUT)
    finalCopier_.begin(i2s_, meas_);
    meas_.begin(info_);
#else
    finalCopier_.begin(i2s_, volumeMeter_);
#endif
    volumeMeter_.begin(info_);
    mixerVolume_.begin(info_);
    mixer_.begin(info_);
    sigGenStream_.begin(info_);
    fileVolume_.begin(info_);
    fileDecoder_.begin(info_);
    fileBuffer_.begin();

    sineGen_.begin(info_, N_B5);
    sineGen_.setMaxAmplitudeStep(float(1<<(BITS_PER_SAMPLE-1)));
    sineGen_.setAmplitude(0);
    sawGen_.begin(info_, N_B5);
    sawGen_.setAmplitude(0);
    squareGen_.begin(info_, N_B5);
    squareGen_.setAmplitude(0);

    return true;
}

bool Player::step() {
    size_t bytesCopied;
    if(playing_) {
        bytesCopied = fileCopier_.copy();
        LOGI("Player::step(): fileCopier copied %d bytes", bytesCopied);
        LOGI("Player::step(): decoder has %d bytes available for read and %d for write", fileDecoder_.available(), fileDecoder_.availableForWrite());
#if 0
        if(bytesCopied == 0) {
            LOGI("Player::step(): fileCopier finished playing");
            fileCopier_.end();
            playing_ = false;
            audioFile_.close();
        }
#endif
    }
    bytesCopied = finalCopier_.copy();
    LOGI("Player::step(): finalCopier copied %d bytes", bytesCopied);
    float vol = volumeMeter_.volumeRatio();
    analogWrite(LED_BUILTIN, (1-vol)*255);
    LOGD("Volume: %f", vol);

    return true;
}

bool Player::stop() {
    running_ = false;
    return true;
}

void Player::setAudioInfo(AudioInfo from) {
    LOGI("New audioInfo: sample_rate: %d / channels: %d / bits_per_sample: %d\n", from.sample_rate, from.channels, from.bits_per_sample);
    sineGen_.setAudioInfo(from);
    squareGen_.setAudioInfo(from);
    sawGen_.setAudioInfo(from);
    sigGenStream_.setAudioInfo(from);

    fileVolume_.setAudioInfo(from);
    fileBuffer_.setAudioInfo(from);

    mixer_.setAudioInfo(from);
    mixerVolume_.setAudioInfo(from);
    volumeMeter_.setAudioInfo(from);
#if defined(MEASURE_THROUGHPUT)
    meas_.setAudioInfo(from);
#endif
    i2s_.setAudioInfo(from);

    info_ = from;
}

void Player::setSignalGeneratorWaveform(Waveform wf) {
    switch(wf) {
    case SINE:
    default:
        sigGenStream_.setInput(sineGen_);
        break;
    case SQUARE:
        sigGenStream_.setInput(squareGen_);
        break;
    case SAWTOOTH:
        sigGenStream_.setInput(sawGen_);
        break;
    }
}

void Player::setSignalGeneratorFrequency(float freq) {
    sineGen_.setFrequency(freq);
    squareGen_.setFrequency(freq);
    sawGen_.setFrequency(freq);
}

void Player::setSignalGeneratorVolume(float volume) {
    sineGen_.setAmplitude(volume);
    squareGen_.setAmplitude(volume);
    sawGen_.setAmplitude(volume);
}

bool Player::playFile(const std::string& path) {
    std::string p = std::string("/")+path;
    audioFile_ = SD.open(p.c_str(), FILE_READ);
    if(!audioFile_) {
        LOGE("Failed to open file '%s'\n", p.c_str());
        return false;
    }
    LOGW("Playing file '%s'\n", p.c_str());
    if(path.rfind(".wav") != std::string::npos) {
        wav_.begin();
        fileDecoder_.setDecoder(&wav_);
        fileCopier_.begin(fileDecoder_, audioFile_);

        playing_ = true;
    } else if(path.rfind(".mp3") != std::string::npos) {
        mp3_.begin();
        fileDecoder_.setDecoder(&mp3_);
        fileCopier_.begin(fileDecoder_, audioFile_);

        playing_ = true;
    } else if(path.rfind(".stream") != std::string::npos) {
        std::string url = audioFile_.readString().c_str();
        while(url.length() > 0 && (url.back() == '\n' || url.back() == '\r')) url.pop_back(); // Trim trailing newlines
        Serial.printf("Streaming from URL: '%s'\n", url.c_str());
        mp3_.begin();
        urlStream_.begin(url.c_str(), "audio/mp3");

        fileDecoder_.setDecoder(&mp3_);
        fileCopier_.begin(fileDecoder_, urlStream_);

        playing_ = true;
    } else {
        Serial.println("Unsupported file type");
        audioFile_.close();
        playing_ = false;
        return false;
    }

    return true;
}
