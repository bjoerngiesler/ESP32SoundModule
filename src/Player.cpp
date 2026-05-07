#include "Player.h"
#include "Settings.h"

Player Player::inst;

#if defined(MEASURE_THROUGHPUT)
#define OUTPUT_STREAM meas_
#else
#define OUTPUT_STREAM i2s_
#endif

Player::Player():
#if defined(MEASURE_THROUGHPUT)
    meas_(20, &Serial),
#endif
    finalCopier_(AUDIO_BUFFER_SIZE)
{
    pinMode(LED_BUILTIN, OUTPUT);
    //digitalWrite(LED_BUILTIN, LOW);
}

bool Player::start() {
    pinMode(P_I2S_BCK, OUTPUT);
    pinMode(P_I2S_DATA_OUT, OUTPUT);
    pinMode(P_I2S_WS, OUTPUT);

    AudioToolsLogger.begin(Serial, AUDIO_TOOLS_LOG_LEVEL);

    auto cfg = i2s_.defaultConfig(TX_MODE);
    cfg.setAudioInfo(info_);
    cfg.pin_data = P_I2S_DATA_OUT;
    cfg.pin_ws = P_I2S_WS;
    cfg.pin_bck = P_I2S_BCK;
    cfg.is_master = true;
    i2s_.begin(cfg);

    sineGen_.begin(info_, N_B5);
    squareGen_.begin(info_, N_B5);
    sawtoothGen_.begin(info_, N_B5);

    sigGenStream_.setInput(sineGen_);
    sigGenStream_.begin(info_);

    sigGenVolume_.setStream(sigGenStream_);
    sigGenVolume_.begin(info_);

    mixer_.add(sigGenVolume_);
    mixerVolume_.setStream(mixer_);
    //mixerVolume_.setVolume(0.f);
    mixerVolume_.begin(info_);

    volumeMeter_.setStream(mixerVolume_);
    volumeMeter_.begin(info_);

#if defined(MEASURE_THROUGHPUT)
    meas_.setLogOutput(Serial);
    meas_.setStream(i2s_);
    meas_.setName("Player");
    meas_.begin(info_);
#endif

    finalCopier_.begin(OUTPUT_STREAM, volumeMeter_);

    return true;
}

bool Player::step() {
    size_t bytesCopied = finalCopier_.copy();
    LOGD("Player::step(): copied %d bytes", bytesCopied);
    float rawVol = volumeMeter_.volume();
    float vol = rawVol / (1<<(BITS_PER_SAMPLE-1));
    LOGD("Volume: raw %f, cooked %f\n", rawVol, vol);

    analogWrite(LED_BUILTIN, (1-vol)*255);
    return true;
}

bool Player::stop() {
    running_ = false;
    return true;
}

void Player::setAudioInfo(AudioInfo from) {
    printf("New audioInfo: sample_rate: %d / channels: %d / bits_per_sample: %d\n", from.sample_rate, from.channels, from.bits_per_sample);
    info_ = from;
    sineGen_.setAudioInfo(info_);
    squareGen_.setAudioInfo(info_);
    sawtoothGen_.setAudioInfo(info_);
    sigGenStream_.setAudioInfo(info_);
    sigGenVolume_.setAudioInfo(info_);
    mixer_.setAudioInfo(info_);
    mixerVolume_.setAudioInfo(info_);
    volumeMeter_.setAudioInfo(info_);
    i2s_.setAudioInfo(info_);
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
        sigGenStream_.setInput(sawtoothGen_);
        break;
    }
}

void Player::setSignalGeneratorFrequency(float freq) {
    sineGen_.setFrequency(freq);
    squareGen_.setFrequency(freq);
    sawtoothGen_.setFrequency(freq);
}

