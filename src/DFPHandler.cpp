#include <Arduino.h>
#include "DFPHandler.h"
#include "SoundPlayer.h"
#include "FileManager.h"

DFPHandler DFPHandler::inst;

std::map<CommandCode, std::function<void(const Command& cmd)>> DFPHandler::callbackMap_ = {
    { CMD_RESET, [](const Command& cmd)->void { DFPHandler::inst.cmdReset(cmd); }},
    { CMD_VOLUME, [](const Command& cmd)->void { DFPHandler::inst.cmdSetVolume(cmd); }},
    { CMD_STOP, [](const Command& cmd)->void { DFPHandler::inst.cmdStopPlayback(cmd); }},
    { CMD_PLAY_FOLDER, [](const Command& cmd)->void { DFPHandler::inst.cmdPlayFolder(cmd); }}
};


bool DFPHandler::start() {
    Serial1.begin(9600, SERIAL_8N1, RX, TX);
    return true;
}

bool DFPHandler::step() {
    Command cmd;
    if(readCommand(cmd, 1) == false)  return false;
    printf("Command: %02x Para1: %0x Para2: %0x Wants feedback: %d\n", cmd.cmd, cmd.para1, cmd.para2, cmd.feedback);
    if(callbackMap_.find(cmd.cmd) != callbackMap_.end()) {
        callbackMap_[cmd.cmd](cmd);
        return true;
    } 
    printf("Unknown / unhandled command 0x%02x\n", cmd.cmd);
    return false;
}

bool DFPHandler::stop() {
    return true;
}

bool DFPHandler::waitAvailable(unsigned int timeout, uint8_t& byte) {
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

bool DFPHandler::readCommand(Command& command, unsigned int timeout) {
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

    command.version = ver;
    command.cmd = (CommandCode)buf[0];
    command.feedback = buf[1]>0;
    command.para1 = buf[2];
    command.para2 = buf[3];
    command.checksum = buf[4] << 8 | buf[5];
    if(checkChecksum(command) == false) {
        printf("Checksum error -- should be 0x%0x but is 0x%0x\n", calcChecksum(command), command.checksum);
        return false;
    } 

    return true;
}

uint16_t DFPHandler::calcChecksum(const Command& cmd) {
    return 1 + (0xffff - (cmd.version  + 6 + cmd.cmd + cmd.feedback + cmd.para1 + cmd.para2));
}

bool DFPHandler::checkChecksum(const Command& cmd) {
    return calcChecksum(cmd) == cmd.checksum;
}

void DFPHandler::cmdReset(const Command& cmd) {
    printf("CMD: Reset!\n");
}

void DFPHandler::cmdSetVolume(const Command& cmd) {
    uint16_t volume = cmd.para1 << 8 | cmd.para2;
    printf("CMD: Set volume to %d\n", volume);
    float v = float(volume)/30.0f;
    SoundPlayer::inst.setVolume(v);
}

void DFPHandler::cmdStopPlayback(const Command& cmd) {
    printf("CMD: Stop playback\n");
    SoundPlayer::inst.stopPlayback();
}

void DFPHandler::cmdPlayFolder(const Command& cmd) {
    std::string filename = FileManager::inst.filename(cmd.para1, cmd.para2);
    printf("CMD: Play folder %d file %d => '%s'\n", cmd.para1, cmd.para2, filename.c_str());
    if(filename != "") SoundPlayer::inst.playFile(filename);
}
