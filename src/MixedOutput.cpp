#include "MixedOutput.h"
#include "Settings.h"

MixedOutput MixedOutput::inst;

MixedOutput::MixedOutput():
    info_(SAMPLE_RATE, NUM_CHANNELS, BITS_PER_SAMPLE) {
    audioChangeHandle_ = xSemaphoreCreateBinary();
    xSemaphoreGive(audioChangeHandle_);
}

bool MixedOutput::start(bool multicore) {
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

    volumeStream_.setVolume(.5f);
    volumeStream_.setOutput(i2s_);
    volumeStream_.begin(info_);
    mixer_.begin(info_);

    copier_.begin(volumeStream_, mixer_);
    
    if(multicore) {
        printf("Starting new task on core 1\n");
        shouldStop_ = false;
        xTaskCreatePinnedToCore([](void*){MixedOutput::inst.loop();},
                                "AudioLoop", 10000, NULL, 1, &taskHandle_, 1);
    } else {
        printf("Starting on current core\n");
    }

    isRunning_ = true; 
    Serial.println("MixedOutput started");
    return true;
}

bool MixedOutput::step() {
    if(!isRunning_) {
        printf("Not running, cannot step\n");
        return false;
    }
    if(copier_.available() == 0) {
        // no data to copy, we can skip this step
        return true;
    }
    size_t bytesCopied = copier_.copy();
    printf("MixedOutput:Copied %d bytes\n", (int)bytesCopied);
    return true;
}

void MixedOutput::lockAudioChange() {
    xSemaphoreTake(audioChangeHandle_, portMAX_DELAY);
}

void MixedOutput::unlockAudioChange() {
    xSemaphoreGive(audioChangeHandle_);
}

void MixedOutput::loop() {
    while(!shouldStop_) {
        xSemaphoreTake(audioChangeHandle_, portMAX_DELAY);
        size_t bytesCopied = copier_.copyAll();
        xSemaphoreGive(audioChangeHandle_);
        vTaskDelay(1);
    }
    isRunning_ = false;
}

bool MixedOutput::stop() {
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

int MixedOutput::add(MixerInput& input, int weight) {
    lockAudioChange();
    int index = mixer_.add(input.output(), weight);
    input.setMixerIndex(index);
    unlockAudioChange();
    return index;
}

void MixedOutput::setWeight(int index, int weight) {
    lockAudioChange();
    mixer_.setWeight(index, weight);
    unlockAudioChange();
}

bool MixedOutput::remove(MixerInput& input) {
    lockAudioChange();
    int index = input.mixerIndex();
    bool result = mixer_.remove(index);
    unlockAudioChange();
    return result;
}

bool MixedOutput::remove(int index) {
    lockAudioChange();
    bool result = mixer_.remove(index);
    unlockAudioChange();
    return result;
}
