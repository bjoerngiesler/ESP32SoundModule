#include <Arduino.h>
#include <Wire.h>
#include <LibBB.h>
#include "Settings.h"
#include "Player.h"
#include "FileManager.h"
#include "Configuration.h"
#include "DFPHandler.h"
#include "MemFile.h"

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
static const char *KEY_IGNORE_DOTFILES = "ignore_dotfiles";
static const char *KEY_IGNORE_NONNUMERIC = "ignore_non_numeric";

MemFile removedMemFile;

void startup() {
    Configuration config("/config.ini");

    // FIXME Read and store file for "play_on_remove"

    if(config.valueForKey("global", "wifi", "off") == "on") {
        String ssid = config.valueForKey("wifi", "ssid");
        String password = config.valueForKey("wifi", "password");
        Serial.printf("Connecting to WiFi network '%s' with password '%s'\n", ssid.c_str(), password.c_str());
        wifiMulti.addAP(ssid.c_str(), password.c_str());
        wifiMulti.run();
    }

    bool ignoreDotfiles = config.valueForKey(SEC_GLOBAL, "ignore_dotfiles", true);
    bool ignoreNonNumeric = config.valueForKey(SEC_GLOBAL, "ignore_non_numeric", false);
    FileManager::FileIndexingMode indexingMode = FileManager::IndexNumAl;
    String indexingModeStr = config.valueForKey(SEC_GLOBAL, "file_indexing_mode", "numal");
    if(indexingModeStr == "alnum") {
        indexingMode = FileManager::IndexAlnum;
    } else if(indexingModeStr == "numal") {
        indexingMode = FileManager::IndexNumAl;
    }

    FileManager::inst.setIgnoreDotfiles(ignoreDotfiles);
    FileManager::inst.setIgnoreNonNumeric(ignoreNonNumeric);
    FileManager::inst.setFileIndexingMode(indexingMode);

    FileManager::inst.buildFileMap();

    // Configure signal generator
    Player::inst.setSignalGeneratorEnabled(config.valueForKey(SEC_GLOBAL, "signal_generator", false));
    Player::inst.setSignalGeneratorFrequency(config.valueForKey(SEC_SIGGEN, "frequency", 440.0f));
    Player::inst.setSignalGeneratorVolume(config.valueForKey(SEC_SIGGEN, KEY_VOLUME, 1.0f));
    String waveform = config.valueForKey(SEC_SIGGEN, "waveform", "sine");
    if(waveform == "square") Player::inst.setSignalGeneratorWaveform(Player::SQUARE);
    else if(waveform == "sawtooth" || waveform == "saw") Player::inst.setSignalGeneratorWaveform(Player::SAWTOOTH);
    else Player::inst.setSignalGeneratorWaveform(Player::SINE);

    Player::inst.setFileVolume(config.valueForKey(SEC_FILEPL, KEY_VOLUME, 1.0f));
    if(initial && config.hasValue(SEC_FILEPL, "play_on_startup")) {
        bb::printf("Playing file %s\n", config.valueForKey(SEC_FILEPL, "play_on_startup").c_str());
        Player::inst.playFile(config.valueForKey(SEC_FILEPL, "play_on_startup"));
    } else if(config.hasValue(SEC_FILEPL, "play_on_insert")) {
        Player::inst.playFile(config.valueForKey(SEC_FILEPL, "play_on_insert"));
    }
    if(config.hasValue(SEC_FILEPL, "play_on_remove")) {
        bb::printf("Storing file '%s' to be played on remove.\n", config.valueForKey(SEC_FILEPL, "play_on_remove").c_str());
        bb::printf("Free PSRAM before: %d\n", ESP.getFreePsram());

        removedMemFile = MemFile(config.valueForKey(SEC_FILEPL, "play_on_remove"));

        bb::printf("Free PSRAM after: %d\n", ESP.getFreePsram());
        bb::printf("Mem file has a size of %d, buffer at 0x%0x, filename '%s'\n", 
            removedMemFile.size(), removedMemFile.buffer(), removedMemFile.filename().c_str());
    }
    Player::inst.setEffects(config.valueForKey(SEC_FILEPL, "effects", false));
    Player::inst.setOutputVolume(config.valueForKey(SEC_GLOBAL, KEY_VOLUME, 1.0f));

    initial = false;
}

void teardown(void) {
    // FIXME Play stored file for "play_on_remove" here
    if(WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
    }

    Player::inst.stopPlayback();
    if(removedMemFile.size() != 0) {
        bb::printf("Playing removed file '%s' stored in memory\n", removedMemFile.filename().c_str());
        Player::inst.playMemFile(removedMemFile);
    }
}

void sdCardInserted(bool yn) {
    if(yn) startup();
    else teardown();
}

void setup(void) {
    Serial.begin(115200);
    psramInit();

    bb::Console::console.initialize();
    bb::Runloop::runloop.initialize();
    DFPHandler::inst.initialize();
    Player::inst.initialize();

    FileManager::inst.addSDCardInsertCallback(sdCardInserted);

    FileManager::inst.start();
    DFPHandler::inst.start();
    Player::inst.start();
    if(FileManager::inst.checkSDCardPresent()) {
        startup();
    }

    bb::Console::console.setFirstResponder(&Player::inst);
    bb::Console::console.start();
    Runloop::runloop.start();
}

void loop(void) {
}