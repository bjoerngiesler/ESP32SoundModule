#include "Mixer.h"
#include "Settings.h"

Mixer Mixer::inst;

Mixer::Mixer():
    info_(SAMPLE_RATE, NUM_CHANNELS, BITS_PER_SAMPLE),
    dummyStream_(AUDIO_BUFFER_SIZE) {
    audioChangeHandle_ = xSemaphoreCreateBinary();
    xSemaphoreGive(audioChangeHandle_);
}

bool Mixer::start(bool multicore) {
    if(isRunning_) {
        printf("Already running, not starting again\n");
        return false;
    }

    Serial.println("Starting MixedOutput...");

    pinMode(P_I2S_BCK, OUTPUT);
    pinMode(P_I2S_DATA_OUT, OUTPUT);
    pinMode(P_I2S_WS, OUTPUT);

    // Set up I2S
    auto cfg = i2s_.defaultConfig(TX_MODE);
    cfg.setAudioInfo(info_);
    cfg.pin_data = P_I2S_DATA_OUT;
    cfg.pin_ws = P_I2S_WS;
    cfg.pin_bck = P_I2S_BCK;
    cfg.is_master = true;
    i2s_.begin(cfg);

    mixer_.setOutput(volumeStream_);
    mixer_.setOutputCount(NUM_MIXER_INPUTS);
    mixer_.begin(AUDIO_BUFFER_SIZE);

    for(size_t i=0; i<NUM_MIXER_INPUTS; i++) {
        inputCopiers_[i].begin(mixer_, dummyStream_);
    }

    volumeStream_.setVolume(.5f);
    volumeStream_.setOutput(i2s_);
    volumeStream_.begin(info_);

    if(multicore) {
        printf("Starting new task on core 1\n");
        shouldStop_ = false;
        xTaskCreatePinnedToCore([](void*){Mixer::inst.loop();},
                                "AudioLoop", 10000, NULL, 1, &taskHandle_, 1);
    } else {
        printf("Starting on current core\n");
    }
    multicore_ = multicore;

    isRunning_ = true; 
    Serial.println("MixedOutput started");
    return true;
}

bool Mixer::step() {
    if(multicore_) {
        return false;
    }

    if(!isRunning_) {
        printf("Not running, cannot step\n");
        return false;
    }

    for(size_t i=0; i<NUM_MIXER_INPUTS; i++) {
        printf("Copier %d copies from 0x%x to 0x%x (mixer), dummy is 0x%x\n", i, inputCopiers_[i].getFrom(), inputCopiers_[i].getTo(), &dummyStream_);
        if(inputCopiers_[i].available() > 0) {
            printf("Mixer: Trying to copy %d available bytes on %d\n", inputCopiers_[i].available(), i);
            size_t bytesCopied = inputCopiers_[i].copyBytes(AUDIO_BUFFER_SIZE);
            printf("Mixer: Copied %d bytes on copier %d\n", (int)bytesCopied, i);
        } else {
            printf("Nothing available on channel %d\n", i);
        }
    }

    return true;
}

void Mixer::lockAudioChange() {
    xSemaphoreTake(audioChangeHandle_, portMAX_DELAY);
}

void Mixer::unlockAudioChange() {
    xSemaphoreGive(audioChangeHandle_);
}

void Mixer::loop() {
    while(!shouldStop_) {
        xSemaphoreTake(audioChangeHandle_, portMAX_DELAY);
        for(size_t i=0; i<NUM_MIXER_INPUTS; i++) {
            if(inputCopiers_[i].available() > 0) {
                inputCopiers_[i].copy();
            }
        }
        xSemaphoreGive(audioChangeHandle_);
        vTaskDelay(1);
    }
    isRunning_ = false;
}

bool Mixer::stop() {
    if(!isRunning_) {
        printf("Not running, cannot stop\n");
        return false;
    }
    Serial.println("Stopping MixedOutput...");
    shouldStop_ = true;
    while(isRunning_) delay(1);
    i2s_.end();
    Serial.println("MixedOutput stopped");
    return true;
}

bool Mixer::setInput(MixerInput& input, size_t ch, float weight) {
    if(ch >= NUM_MIXER_INPUTS) {
        printf("Invalid channel %d, max is %d\n", ch, NUM_MIXER_INPUTS - 1);
        return false;
    }
    if(ch != MIXER_INPUT_FILEPLAYER) return false;
    //input.output().setOutput(mixer_);
#if 0
    lockAudioChange();
    printf("Setting input copier up from 0x%x (input) to 0x%x (mixer)\n", &input.output(), &mixer_);
    inputCopiers_[ch].end();
    inputCopiers_[ch].begin(i2s_, input.output());
#endif
    mixer_.setWeight(ch, weight);
    unlockAudioChange();
    return true;
}

void Mixer::setAudioInfo(AudioInfo from) {
    printf("New audioInfo: sample_rate: %d / channels: %d / bits_per_sample: %d\n", from.sample_rate, from.channels, from.bits_per_sample);
    printf("Mixer audioInfo: sample_rate: %d / channels: %d / bits_per_sample: %d\n", info_.sample_rate, info_.channels, info_.bits_per_sample);
    volumeStream_.setAudioInfo(from);
    i2s_.setAudioInfo(from);
    dummyStream_.setAudioInfo(from);

    info_ = from;
}


bool Mixer::setWeight(size_t ch, float weight) {
    if(ch >= NUM_MIXER_INPUTS) {
        printf("Invalid channel %d, max is %d\n", ch, NUM_MIXER_INPUTS - 1);
        return false;
    }
    lockAudioChange();
    mixer_.setWeight(ch, weight);
    unlockAudioChange();
    return true;
}

bool Mixer::removeInput(size_t ch) {
    if(ch >= NUM_MIXER_INPUTS) {
        printf("Invalid channel %d, max is %d\n", ch, NUM_MIXER_INPUTS - 1);
        return false;
    }
    lockAudioChange();
    inputCopiers_[ch].end();
    inputCopiers_[ch].begin(mixer_, dummyStream_);
    unlockAudioChange();
    return true;
}
