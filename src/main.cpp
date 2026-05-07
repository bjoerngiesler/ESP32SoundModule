#include <Arduino.h>
#include <Wire.h>
#include "Settings.h"
#include "FilePlayer.h"
#include "SignalGenerator.h"
#include "Mixer.h"
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
#if 0
    printf("Read configuration:\n");
    printf("---------------\n");
    config.print();
    printf("---------------\n");
#endif

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
#if 0
#if !defined(ONLY_FILE_PLAYER)
    Mixer::inst.start(false);
#endif

    if(config.valueForKey("global", "signal_generator", false) == true) {
        Serial.println("Starting signal generator");

        float frequency = config.valueForKey("signal_generator", "frequency", 440.0f);
        std::string waveform = config.valueForKey("signal_generator", "waveform", "sine");
        if(waveform == "square") SignalGenerator::inst.start(frequency, SignalGenerator::SQUARE);
        else if(waveform == "sawtooth") SignalGenerator::inst.start(frequency, SignalGenerator::SAWTOOTH);
        else SignalGenerator::inst.start(frequency, SignalGenerator::SINE);

        SignalGenerator::inst.setVolume(config.valueForKey("signal_generator", "volume", .2f));
        Mixer::inst.setInput(SignalGenerator::inst, MIXER_INPUT_SIGNALGENERATOR);
    }

    if(config.valueForKey("global", "file_player", true) == true) {
        Serial.println("Starting file player");
        FilePlayer::inst.start();

        if(initial && config.hasValue("global", "play_on_startup")) {
            FilePlayer::inst.playFile(config.valueForKey("global", "play_on_startup"));
        } else if(config.hasValue("global", "play_on_insert")) {
            FilePlayer::inst.playFile(config.valueForKey("global", "play_on_insert"));
        }

        FilePlayer::inst.setVolume(config.valueForKey("file_player", "volume", .5f));
        Mixer::inst.setInput(FilePlayer::inst, MIXER_INPUT_FILEPLAYER);
    }

    // volume goes from 0 to 30 in config file for compatibility
    float vol = config.valueForKey("global", "volume", 30)/30.0f;
    Mixer::inst.setOutputVolume(constrain(vol, 0.0f, 1.0f));
#endif

    initial = false;
}

void teardown(void) {
    // FIXME Read and store file for "play_on_remove"
//    Mixer::inst.stop();

    if(WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
    }
}

void setup(void) {
    Serial.begin(2000000);
    while(!Serial) delay(10);
    delay(3000);
    DFPHandler::inst.start();

#if 0
    if(FileManager::inst.isCardPresent()) {
        startup();
    }
#endif

    Player::inst.start();
}

void loop(void) {
#if 0
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
#endif
#if 0
    // DFPHandler::inst.step();
    FilePlayer::inst.step();
#if defined(ONLY_FILE_PLAYER)
#else
    //printf("********** Step() in Signal Generator\n");
    SignalGenerator::inst.step();
    //printf("********** Step() in Mixer\n");
    Mixer::inst.step();
    //printf("********** Done for this loop\n");
#endif
#endif

    Player::inst.step();

    vTaskDelay(1);
}