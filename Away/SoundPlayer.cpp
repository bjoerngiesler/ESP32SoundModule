#include <Arduino.h>
#include "SoundPlayer.h"
#include "Settings.h"

SoundPlayer SoundPlayer::inst;

static void audioInfo(Audio::msg_t m) {
    Serial.printf("%s: %s\n", m.s, m.msg);
}

std::map<CommandCode, std::function<bool(const Command& cmd)>> SoundPlayer::callbackMap_ = {
};

SoundPlayer::SoundPlayer() {
    pinMode(PIN_I2S_BCLK, OUTPUT);
    pinMode(PIN_I2S_DOUT, OUTPUT);
    pinMode(PIN_I2S_LRC, OUTPUT);

    audio_ = new Audio(0);
    Audio::audio_info_callback = audioInfo;
    audio_->setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
    audio_->setVolume(20); // default 0...21
}

bool SoundPlayer::start() {
    Serial1.begin(9600, SERIAL_8N1, RX, TX);
    return true;
}

bool SoundPlayer::step() {
    audio_->loop();
    Command cmd;
    if(readCommand(cmd, 10) == false)  return false;
    printf("Command: %02x Para1: %0x Para2: %0x Wants feedback: %d\n", cmd.cmd, cmd.para1, cmd.para2, cmd.feedback);
    if(callbackMap_.find(cmd.cmd) != callbackMap_.end()) {
        return callbackMap_[cmd.cmd](cmd);
    } 
    printf("Unknown / unhandled command 0x%02x\n", cmd.cmd);
    return false;
}

bool SoundPlayer::stop() {
    return true;
}

bool SoundPlayer::waitAvailable(unsigned int timeout, uint8_t& byte) {
    while(!Serial1.available() && timeout > 0) {
        timeout--;
        delay(1);
    }
    if(!Serial1.available()) {
        return false;
    }
    byte = Serial1.read();
    return true;
}

bool SoundPlayer::readCommand(Command& command, unsigned int timeout) {
    uint8_t sync, ver, len, cmd, feedback, para1, para2, checksum;

    // sync
    while(true) {
        if(!waitAvailable(timeout, sync)) {
            //printf("Timeout while waiting for sync byte\n");
            return false;
        }
        if(sync == 0x7e) break;
        while(true) {
            if(!waitAvailable(timeout, sync)) {
                //printf("Timeout while waiting for sync byte\n");
                return false;
            }
            if(sync == 0xef) break;
        }
    }

    if(!waitAvailable(timeout, ver)) {
        //printf("Timeout while waiting for version byte\n");
        return false;
    }
    if(!waitAvailable(timeout, len)) {
        //printf("Timeout while waiting for length byte\n");
        return false;
    }

    uint8_t* buf = new uint8_t[len];
    for(int i=0; i<len; i++) {
        if(!waitAvailable(timeout, buf[i])) {
            delete[] buf;
            return false;
        }
    }

    if(len != 6) {
        printf("Unknown length\n");
        delete[] buf;
        return false;
    }

    command.cmd = (CommandCode)buf[0];
    command.feedback = buf[1]>0;
    command.para1 = buf[2];
    command.para2 = buf[3];

    return true;
}

bool SoundPlayer::playFile(const std::string& path) {
    Serial.printf("Playing file '%s'\n", path.c_str());
    return audio_->connecttoFS(SD, path.c_str());;
}
