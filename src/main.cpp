#include <Arduino.h>
#include <Wire.h>
#include "Settings.h"
#include "Player.h"
#include "FileManager.h"
#include "Configuration.h"
#include "DFPHandler.h"

#include <WiFiMulti.h>
#include <SD.h>
#include <ESP_I2S.h>

WiFiMulti wifiMulti;
bool initial = true;

void startup() {
    Configuration config("/config.ini");
    printf("Read configuration:\n");
    printf("---------------\n");
    config.print();
    printf("---------------\n");

    // FIXME Read and store file for "play_on_remove"
#if 0
    if(config.valueForKey("global", "wifi", "off") == "on") {
        std::string ssid = config.valueForKey("wifi", "ssid");
        std::string password = config.valueForKey("wifi", "password");
        Serial.printf("Connecting to WiFi network '%s' with password '%s'\n", ssid.c_str(), password.c_str());
        wifiMulti.addAP(ssid.c_str(), password.c_str());
        wifiMulti.run();
        while (WiFi.status() != WL_CONNECTED) delay(1500);
    }
#endif
    bool ignoreDotfiles = config.valueForKey("global", "ignore_dotfiles", true);
    bool ignoreNonNumeric = config.valueForKey("global", "ignore_non_numeric", false);
    FileManager::FileIndexingMode indexingMode = FileManager::IndexNumAl;
    std::string indexingModeStr = config.valueForKey("global", "file_indexing_mode", "numal");
    if(indexingModeStr == "alnum") {
        indexingMode = FileManager::IndexAlnum;
    } else if(indexingModeStr == "numal") {
        indexingMode = FileManager::IndexNumAl;
    }

    FileManager::inst.setIgnoreDotfiles(ignoreDotfiles);
    FileManager::inst.setIgnoreNonNumeric(ignoreNonNumeric);
    FileManager::inst.setFileIndexingMode(indexingMode);

    FileManager::inst.buildFileMap();
    FileManager::inst.buildFolderMap();

    Player::inst.setSignalGeneratorFrequency(config.valueForKey("signal_generator", "frequency", 440.0f));
    Player::inst.setSignalGeneratorVolume(config.valueForKey("signal_generator", "volume", 0.0f));
    std::string waveform = config.valueForKey("signal_generator", "waveform", "sine");
    if(waveform == "square") Player::inst.setSignalGeneratorWaveform(Player::SQUARE);
    else if(waveform == "sawtooth" || waveform == "saw") Player::inst.setSignalGeneratorWaveform(Player::SAWTOOTH);
    else Player::inst.setSignalGeneratorWaveform(Player::SINE);

    Player::inst.setFileVolume(config.valueForKey("file_player", "volume", 1.0f));
    if(initial && config.hasValue("global", "play_on_startup")) {
        Player::inst.playFile(config.valueForKey("global", "play_on_startup"));
    } else if(config.hasValue("global", "play_on_insert")) {
        Player::inst.playFile(config.valueForKey("global", "play_on_insert"));
    }

    Player::inst.setOutputVolume(config.valueForKey("global", "volume", 1.0f));

    initial = false;
}

void teardown(void) {
    // FIXME Play stored file for "play_on_remove" here
    if(WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
    }
}

void setup(void) {
    Serial.begin(2000000);
    while(!Serial) delay(10);
    delay(3000);

    FileManager::inst.start();
    DFPHandler::inst.start();
    Player::inst.start();

    if(FileManager::inst.isCardPresent()) {
        startup();
    }
}

void checkSDCard() {
    static bool cardPresent = FileManager::inst.isCardPresent();

    if(cardPresent && !FileManager::inst.isCardPresent()) {
        Serial.printf("SD card has been removed. Tearing down.\n");
        cardPresent = false;
        teardown();
        delay(1000);
    } else if(!cardPresent && FileManager::inst.isCardPresent()) {
        Serial.printf("SD card was inserted. Setting up.\n");
        cardPresent = true;
        startup();
    }
}

void loop(void) {
    static unsigned long lastCheckTime = millis();
    if(millis() - lastCheckTime > 1000) {
        checkSDCard();
        lastCheckTime = millis();
    }

    Player::inst.step();

    vTaskDelay(1);
}