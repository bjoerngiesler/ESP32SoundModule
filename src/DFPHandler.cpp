#include <Arduino.h>
#include <LibBB.h>
#include "DFPHandler.h"
#include "Player.h"
#include "FileManager.h"

DFPHandler DFPHandler::inst;

std::map<DFPCmdCode, std::function<void(const DFPCmd& cmd)>> DFPHandler::callbackMap_ = {
    { CMD_RESET, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdReset(cmd); }},
    { CMD_VOLUME, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdSetVolume(cmd); }},
    { CMD_STOP, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdStopPlayback(cmd); }},
    { CMD_PLAY_FOLDER, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdPlayFolder(cmd); }}
};

bb::Result DFPHandler::initialize() {
    return Subsystem::initialize("dfp", "DFP Protocol Handler", "");
}

Result DFPHandler::start(ConsoleStream* stream) {
    Serial1.begin(bps_, SERIAL_8N1, RX, TX);
    return Subsystem::start(stream);;
}

Result DFPHandler::step() {
    DFPCmd cmd;
    if(readDFPCmd(cmd, 1) == false) return RES_SUBSYS_COMM_ERROR;
    //bb::printf("DFPCmd: %02x Para1: %0x Para2: %0x Wants feedback: %d\n", cmd.cmd, cmd.para1, cmd.para2, cmd.feedback);
    if(callbackMap_.find(cmd.cmd) != callbackMap_.end()) {
        callbackMap_[cmd.cmd](cmd);
        return RES_OK;
    } 
    bb::printf("Unknown / unhandled DFPCmd 0x%02x\n", cmd.cmd);
    return RES_SUBSYS_COMM_ERROR;
}

Result DFPHandler::stop(ConsoleStream *stream) {
    return Subsystem::stop(stream);
}

Result DFPHandler::setBps(unsigned int bps) {
    if(bps_ == bps) return RES_OK;
    Serial1.begin(bps);
    bps_ = bps;
    return RES_OK;
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

bool DFPHandler::readDFPCmd(DFPCmd& DFPCmd, unsigned int timeout) {
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
        bb::printf("Unknown length\n");
        delete[] buf;
        return false;
    }

    DFPCmd.version = ver;
    DFPCmd.cmd = (DFPCmdCode)buf[0];
    DFPCmd.feedback = buf[1]>0;
    DFPCmd.para1 = buf[2];
    DFPCmd.para2 = buf[3];
    DFPCmd.checksum = buf[4] << 8 | buf[5];
    if(checkChecksum(DFPCmd) == false) {
        bb::printf("Checksum error -- should be 0x%0x but is 0x%0x\n", calcChecksum(DFPCmd), DFPCmd.checksum);
        return false;
    } 

    return true;
}

uint16_t DFPHandler::calcChecksum(const DFPCmd& cmd) {
    return 1 + (0xffff - (cmd.version  + 6 + cmd.cmd + cmd.feedback + cmd.para1 + cmd.para2));
}

bool DFPHandler::checkChecksum(const DFPCmd& cmd) {
    return calcChecksum(cmd) == cmd.checksum;
}

void DFPHandler::cmdReset(const DFPCmd& cmd) {
    bb::printf("CMD: Reset!\n");
}

void DFPHandler::cmdSetVolume(const DFPCmd& cmd) {
    uint16_t volume = cmd.para1 << 8 | cmd.para2;
    bb::printf("CMD: Set volume to %d\n", volume);
    float v = float(volume)/30.0f;
    Player::inst.setOutputVolume(constrain(v, 0.0f, 1.0f));
}

void DFPHandler::cmdStopPlayback(const DFPCmd& cmd) {
    bb::printf("CMD: Stop playback\n");
    Player::inst.stopPlayback();
}

void DFPHandler::cmdPlayFolder(const DFPCmd& cmd) {
    std::string filename = FileManager::inst.filename(cmd.para1, cmd.para2);
    bb::printf("CMD: Play folder %d file %d => '%s'\n", cmd.para1, cmd.para2, filename.c_str());
    if(filename != "") Player::inst.playFile(filename);
}
