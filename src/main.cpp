#include <Arduino.h>
#include <Wire.h>
#include "SoundPlayer.h"
#include "FileManager.h"
#include "Configuration.h"
#include <Audio.h>
#include <WiFiMulti.h>
#include <SD.h>
#include <ESP_I2S.h>

static const uint8_t PIN_I2S_DOUT = A1;
static const uint8_t PIN_I2S_LRC = A2;
static const uint8_t PIN_I2S_BCLK = A3;

//#define NOISE

#if !defined(NOISE)
Audio* audio;
std::string startupFile;

void my_audio_info(Audio::msg_t m) {
    Serial.printf("%s: %s\n", m.s, m.msg);
}
WiFiMulti wifiMulti;
const char* ssid = "Hogsmeade";
const char* password = "HagridsLittleTricycle";
#endif
#if defined(NOISE)
I2SClass i2s;

#define BUFSIZE 255
uint8_t noise[BUFSIZE], silence[BUFSIZE];
#endif

void setup(void) {
    Serial.begin(2000000);
    while(!Serial) delay(100);
    delay(3000);
    Serial.printf("Starting up...\n");
    delay(1000);



    pinMode(PIN_I2S_BCLK, OUTPUT);
    pinMode(PIN_I2S_DOUT, OUTPUT);
    pinMode(PIN_I2S_LRC, OUTPUT);
#if  defined(NOISE)
    i2s.setPins(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
    if(i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO) == false) {
        Serial.printf("Failed to initialize I2S\n");
        while(true) delay(1000);
    } 
    Serial.printf("I2S initialized\n");
    for(int i = 0; i < BUFSIZE; i++) {
        noise[i] = random(0, 255);
        silence[i] = 0;
    }
#endif
#if !defined(NOISE)
    FileManager::inst.start();
    SoundPlayer::inst.start();
    Configuration config("/config.ini");
    SD.
    printf("Read configuration:\n");
    printf("---------------\n");
    config.print();
    printf("---------------\n");
    startupFile = config.valueForKey("global", "play_on_startup", "whatever.wav");
    printf("Startup file: '%s'\n", startupFile.c_str());
    SD.begin(PIN_CS);
#endif

#if !defined(NOISE)
    audio = new Audio(0);
    Audio::audio_info_callback = my_audio_info;
    audio->setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
    audio->setVolume(20); // default 0...21
    if(startupFile.length() > 0) {
        Serial.printf("Playing '%s'...\n", startupFile.c_str());
        audio->connecttoFS(SD, startupFile.c_str());
    }
    // wifiMulti.addAP(ssid, password);
    // wifiMulti.run(); // if there are multiple access points, use the strongest one
    // while (WiFi.status() != WL_CONNECTED) delay(1500);
    // audio->connecttohost("http://bcast.vigormultimedia.com:8888/sjcomplflac");
#endif
}

void loop(void) {
#if !defined(NOISE)
    //SoundPlayer::inst.step();
    audio->loop();
    if(!audio->isRunning()) {
        Serial.printf("Audio stopped, restarting...\n");
        audio->connecttoFS(SD, startupFile.c_str());
    }
    vTaskDelay(1);
#endif
#if defined(NOISE)
    uint8_t num = i2s.write(noise, BUFSIZE);
    Serial.printf("Wrote %d random bytes to I2S\n", num);
    delay(100);
    num = i2s.write(silence, BUFSIZE);
    Serial.printf("Wrote %d silence bytes to I2S\n", num);
    delay(100);
#endif
}