#include <Arduino.h>
#include <Wire.h>
#include <LibBB.h>
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

static const char *SEC_GLOBAL = "global";
static const char *SEC_SIGGEN = "signal_generator";
static const char *SEC_FILEPL = "file_player";

static const char *KEY_VOLUME = "volume";
static const char *KEY_SIGGEN = "signal_generator";
static const char *KEY_FILEPL = "file_player";


void startup() {
    Configuration config("/config.ini");
    bb::printf("Read configuration:\n");
    bb::printf("---------------\n");
    config.print();
    bb::printf("---------------\n");

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
    bool ignoreDotfiles = config.valueForKey(SEC_GLOBAL, "ignore_dotfiles", true);
    bool ignoreNonNumeric = config.valueForKey(SEC_GLOBAL, "ignore_non_numeric", false);
    FileManager::FileIndexingMode indexingMode = FileManager::IndexNumAl;
    std::string indexingModeStr = config.valueForKey(SEC_GLOBAL, "file_indexing_mode", "numal");
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

    // Configure signal generator
    Player::inst.setSignalGeneratorEnabled(config.valueForKey(SEC_GLOBAL, "signal_generator", false));
    Player::inst.setSignalGeneratorFrequency(config.valueForKey(SEC_SIGGEN, "frequency", 440.0f));
    Player::inst.setSignalGeneratorVolume(config.valueForKey(SEC_SIGGEN, KEY_VOLUME, 1.0f));
    std::string waveform = config.valueForKey(SEC_SIGGEN, "waveform", "sine");
    if(waveform == "square") Player::inst.setSignalGeneratorWaveform(Player::SQUARE);
    else if(waveform == "sawtooth" || waveform == "saw") Player::inst.setSignalGeneratorWaveform(Player::SAWTOOTH);
    else Player::inst.setSignalGeneratorWaveform(Player::SINE);

    Player::inst.setFileVolume(config.valueForKey(SEC_FILEPL, KEY_VOLUME, 1.0f));
    if(initial && config.hasValue(SEC_GLOBAL, "play_on_startup")) {
        bb::printf("Playing file %s\n", config.valueForKey(SEC_GLOBAL, "play_on_startup").c_str());
        Player::inst.playFile(config.valueForKey(SEC_GLOBAL, "play_on_startup"));
    } else if(config.hasValue(SEC_GLOBAL, "play_on_insert")) {
        Player::inst.playFile(config.valueForKey(SEC_GLOBAL, "play_on_insert"));
    }

    Player::inst.setOutputVolume(config.valueForKey(SEC_GLOBAL, KEY_VOLUME, 1.0f));

    initial = false;
}

void teardown(void) {
    // FIXME Play stored file for "play_on_remove" here
    if(WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
    }
}

void setup(void) {
    Serial.begin(115200);
    while(!Serial);
    delay(3000);

    bb::Console::console.initialize();
    bb::Runloop::runloop.initialize();
    DFPHandler::inst.initialize();
    Player::inst.initialize();

    FileManager::inst.start();
    DFPHandler::inst.start();
    DFPHandler::inst.start();
    Player::inst.start();
    if(FileManager::inst.isCardPresent()) {
        startup();
    }

    bb::Console::console.setFirstResponder(&Player::inst);
    bb::Console::console.start();
    bb::printf("starting runloop\n");
    Runloop::runloop.start();
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

    DFPHandler::inst.step();
    Player::inst.step();

    vTaskDelay(1);
}