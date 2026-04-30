#include <Arduino.h>
#include <Wire.h>
#include "SoundPlayer.h"
#include "FileManager.h"
#include "Configuration.h"
#include <Adafruit_NeoPixel.h>

#if 0
bool handleCommand(Command& cmd) {
    switch(cmd.cmd) {
    case CMD_NEXT:
    case CMD_PREV:
    case CMD_PLAY             = 0x03,
    case CMD_INC_VOL          = 0x04,
    case CMD_DEC_VOL          = 0x05,
    case CMD_VOLUME           = 0x06,
    case CMD_EQ               = 0x07,
    case CMD_PLAYBACK_MODE    = 0x08,
    case CMD_PLAYBACK_SRC     = 0x09,
    case CMD_STANDBY          = 0x0a,
    case CMD_NORMAL           = 0x0b, // Come out of standby mode
    case CMD_RESET            = 0x0c,
    case CMD_PLAYBACK         = 0x0d,
    case CMD_PAUSE            = 0x0e,
    case CMD_PLAY_FOLDER      = 0x0f,
    case CMD_VOL_ADJUST       = 0x10,
    case CMD_REPEAT_PLAY      = 0x11,
    case CMD_USE_MP3_FOLDER   = 0x12, // ??
    case CMD_INSERT_ADVERT    = 0x13, // This interrupts the current track with an inserted one
    case CMD_SPEC_TRACK_3000  = 0x14, // Play from specific track with up to 3000 files
    case CMD_STOP_ADVERT      = 0x15, // This stops the interrupt and resumes playback on the original
    case CMD_STOP             = 0x16,
    case CMD_REPEAT_FOLDER    = 0x17,
    case CMD_RANDOM_ALL       = 0x18,
    case CMD_REPEAT_CURRENT   = 0x19,
    case CMD_SET_DAC          = 0x20,

    case CMD_GET_STAY_1       = 0x3c, // ?? According to original data sheet
    case CMD_GET_STAY_2       = 0x3d, // ?? According to original data sheet
    case CMD_GET_STAY_3       = 0x3e, // ?? According to original data sheet
    case CMD_GET_SEND_INIT    = 0x3f,
    case CMD_GET_RETRANSMIT   = 0x40,
    case CMD_GET_REPLY        = 0x41, // ??
    case CMD_GET_STATUS       = 0x42,
    case CMD_GET_VOLUME       = 0x43,
    case CMD_GET_EQ           = 0x44,
    case CMD_GET_MODE         = 0x45,
    case CMD_GET_VERSION      = 0x46,
    case CMD_GET_TF_FILES     = 0x47,
    case CMD_GET_U_FILES      = 0x48,
    case CMD_GET_FLASH_FILES  = 0x49,
    case CMD_KEEP_ON          = 0x4a,
    case CMD_GET_TF_TRACK     = 0x4b,
    case CMD_GET_U_TRACK      = 0x4c,
    case CMD_GET_FLASH_TRACK  = 0x4d,
    case CMD_GET_FOLDER_FILES = 0x4e,
    case CMD_GET_FOLDERS      = 0x4f
        printf("Command 0x%0x not yet implemented!", cmd.cmd);
        return false;
    case 0x0c:
        printf("\tReset\n");
        break;
    case 0x16:    
        printf("\tStop\n");
        break;
    case 0x06:    
        printf("\tAdjust volume to %d\n", cmd.para2);
        break;
    case 0x0f:    
        printf("\tPlay folder %d file %d\n", cmd.para1, cmd.para2);
        break;
    default:
        printf("Unknown command 0x%0x\n", cmd.cmd);
    }
    return true;
}
#endif


void setup(void) {
    Serial.begin(2000000);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    while(!Serial) {
        delay(1);
    }
    delay(3000);

    Wire.begin();
    for(uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if(Wire.endTransmission() == 0) {
            printf("Found I2C device at address 0x%02x\n", addr);
        }
    }

    FileManager::inst.start();
    SoundPlayer::inst.start();
    Configuration config("/config.txt");
    printf("Read configuration:\n");
    printf("---------------\n");
    config.print();
    printf("---------------\n");
}

void loop(void) {
    SoundPlayer::inst.step();
    delay(1);
}