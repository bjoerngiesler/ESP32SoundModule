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
static const char *KEY_FILE_INDEXING_MODE = "file_indexing_mode";
static const char *VAL_NUMAL = "numal";
static const char *VAL_ALNUM = "alnum";
static const char *VAL_ORIG = "orig";

static const char *KEY_FREQUENCY = "frequency";
static const char *KEY_WAVEFORM = "waveform";
static const char *VAL_SINE = "sine";
static const char *VAL_SQUARE = "square";
static const char *VAL_SAWTOOTH = "sawtooth";

static const char *KEY_PLAY_ON_STARTUP = "play_on_startup";
static const char *KEY_PLAY_ON_REMOVE = "play_on_remove";
static const char *KEY_PLAY_ON_INSERT = "play_on_insert";

static const char *KEY_DELAY = "delay";
static const char *KEY_DELAY_DEPTH = "delay_depth";
static const char *KEY_DELAY_FEEDBACK = "delay_feedback";
static const char *KEY_DELAY_TIME = "delay_time";


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

    FileManager::inst.setIgnoreDotfiles(config.valueForKey(SEC_GLOBAL, KEY_IGNORE_DOTFILES, true));
    FileManager::inst.setIgnoreNonNumeric(config.valueForKey(SEC_GLOBAL, KEY_IGNORE_NONNUMERIC, false));
    String indexingModeStr = config.valueForKey(SEC_GLOBAL, KEY_FILE_INDEXING_MODE, VAL_NUMAL);
    if(indexingModeStr == VAL_ALNUM) {
        FileManager::inst.setFileIndexingMode(FileManager::IndexAlnum);
    } else if(indexingModeStr == VAL_NUMAL) {
        FileManager::inst.setFileIndexingMode(FileManager::IndexNumAl);
    } else {
        FileManager::inst.setFileIndexingMode(FileManager::IndexOrig);
    }

    FileManager::inst.buildFileMap();

    // Configure signal generator
    Player::inst.setSignalGeneratorEnabled(config.valueForKey(SEC_GLOBAL, KEY_SIGGEN, false));
    Player::inst.setSignalGeneratorFrequency(config.valueForKey(SEC_SIGGEN, KEY_FREQUENCY, 440.0f));
    Player::inst.setSignalGeneratorVolume(config.valueForKey(SEC_SIGGEN, KEY_VOLUME, 1.0f));
    String waveform = config.valueForKey(SEC_SIGGEN, KEY_WAVEFORM, VAL_SINE);
    if(waveform == VAL_SQUARE) Player::inst.setSignalGeneratorWaveform(Player::SQUARE);
    else if(waveform == VAL_SAWTOOTH) Player::inst.setSignalGeneratorWaveform(Player::SAWTOOTH);
    else Player::inst.setSignalGeneratorWaveform(Player::SINE);

    Player::inst.setDelay(config.valueForKey(SEC_FILEPL, KEY_DELAY, false));
    Player::inst.setDelayDepth(config.valueForKey(SEC_FILEPL, KEY_DELAY_DEPTH, 0.4f));
    Player::inst.setDelayFeedback(config.valueForKey(SEC_FILEPL, KEY_DELAY_FEEDBACK, 0.3f));
    Player::inst.setDelayTime(config.valueForKey(SEC_FILEPL, KEY_DELAY_TIME, 0.3f));
    Player::inst.setOutputVolume(config.valueForKey(SEC_GLOBAL, KEY_VOLUME, 1.0f));

    Player::inst.setFileVolume(config.valueForKey(SEC_FILEPL, KEY_VOLUME, 1.0f));
    String on_startup = config.valueForKey(SEC_FILEPL, KEY_PLAY_ON_STARTUP);
    String on_insert = config.valueForKey(SEC_FILEPL, KEY_PLAY_ON_INSERT);
    String on_remove = config.valueForKey(SEC_FILEPL, KEY_PLAY_ON_REMOVE);
    if(on_remove != "") {
        bb::printf("Storing file '%s' to be played on remove.\n", on_remove.c_str());
        removedMemFile = MemFile(on_remove);
    }
    if(initial && on_startup != "") {
        Player::inst.playFile(on_startup);
    } else if(on_insert != "") {
        Player::inst.playFile(on_insert);
    }

    initial = false;
    Runloop::runloop.excuseOverrun();
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
    while(!Serial) delay(100);
    delay(3000);
    psramInit();

    bb::Console::console.initialize();
    bb::Runloop::runloop.initialize();
    DFPHandler::inst.initialize();
    Player::inst.initialize();

    FileManager::inst.start();
    DFPHandler::inst.start();
    Player::inst.start();

    FileManager::inst.addSDCardInsertCallback(sdCardInserted);

    bb::Console::console.setFirstResponder(&Player::inst);
    bb::Console::console.start();

    Runloop::runloop.start();
}

void loop(void) {
}