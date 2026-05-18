#include <Arduino.h>
#include <LibBB.h>
#include "DFPHandler.h"
#include "Player.h"
#include "FileManager.h"
#include "StateManager.h"

DFPHandler DFPHandler::inst;

static const uint8_t VERSION = 0xff;

std::map<DFPCmdCode, std::function<void(const DFPCmd& cmd)>> DFPHandler::callbackMap_ = {
    { CMD_NEXT, [](const DFPCmd& cmd)->void { StateManager::inst.next(); }},
    { CMD_PREV, [](const DFPCmd& cmd)->void { StateManager::inst.previous(); }},
    { CMD_INC_VOL, [](const DFPCmd& cmd)->void { Player::inst.setOutputVolume(Player::inst.outputVolume() + 0.033); }},
    { CMD_DEC_VOL, [](const DFPCmd& cmd)->void { Player::inst.setOutputVolume(Player::inst.outputVolume() - 0.033); }},
    { CMD_RESET, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdReset(cmd); }},
    { CMD_PAUSE, [](const DFPCmd& cmd)->void { Player::inst.pausePlayback(!Player::inst.isPaused()); }},
    { CMD_VOLUME, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdSetVolume(cmd); }},
    { CMD_STOP, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdStopPlayback(cmd); }},
    { CMD_PLAY_FOLDER, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdPlayFolder(cmd); }},
    { CMD_GET_U_FILES, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdGetUFiles(cmd); }},
    { CMD_GET_FOLDER_FILES, [](const DFPCmd& cmd)->void { DFPHandler::inst.cmdGetFolderFiles(cmd); }},
};

bb::Result DFPHandler::initialize(HardwareSerial& ser, unsigned int bps) {
    ser_ = ser;
    bps_ = bps;
    return Subsystem::initialize("dfp", "DFP Protocol Handler", "");
}

Result DFPHandler::start(ConsoleStream* stream) {
    ser_.begin(bps_, SERIAL_8N1, RX, TX);
    return Subsystem::start(stream);;
}

Result DFPHandler::step() {
    DFPCmd cmd;
    if(!ser_.available()) return RES_OK;

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
    ser_.begin(bps);
    bps_ = bps;
    return RES_OK;
}

bool DFPHandler::waitAvailable(unsigned int timeout, uint8_t& byte) {
    while(!ser_.available() && timeout > 0) {
        timeout--;
        delay(1);
    }
    if(!ser_.available()) {
        return false;
    }
    byte = ser_.read();
    return true;
}

bool DFPHandler::readDFPCmd(DFPCmd& cmd, unsigned int timeout) {
    uint8_t sync, ver, len;

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

    cmd.cmd = (DFPCmdCode)buf[0];
    cmd.feedback = buf[1]>0;
    cmd.para1 = buf[2];
    cmd.para2 = buf[3];
    cmd.checksum = buf[4] << 8 | buf[5];
    if(cmd.calcChecksum() != cmd.checksum) {
        bb::printf("Checksum error -- should be 0x%0x but is 0x%0x\n", cmd.calcChecksum(), cmd.checksum);
        return false;
    } 

    return true;
}

uint16_t DFPCmd::calcChecksum(bool swap) const {
    uint16_t cs = 1 + (0xffff - (VERSION  + 6 + cmd + feedback + para1 + para2));
    if(swap) return (cs&0xff) << 8 | (cs&0xff00) >> 8;
    else return cs;
}

void DFPHandler::cmdReset(const DFPCmd& cmd) {
    bb::printf("CMD: Reset!\n");
    StateManager::inst.stop();
}

void DFPHandler::cmdSetVolume(const DFPCmd& cmd) {
    uint16_t volume = cmd.para1 << 8 | cmd.para2;
    bb::printf("CMD: Set volume to %d\n", volume);
    float v = float(volume)/30.0f;
    Player::inst.setOutputVolume(constrain(v, 0.0f, 1.0f));
}

void DFPHandler::cmdStopPlayback(const DFPCmd& cmd) {
    bb::printf("CMD: Stop playback\n");
    StateManager::inst.stop();
}

void DFPHandler::cmdPlayFolder(const DFPCmd& cmd) {
    bb::printf("CMD: Play folder %d file %d\n", cmd.para1, cmd.para2);
    StateManager::inst.playFile(cmd.para2, cmd.para1);
}

void DFPHandler::cmdGetUFiles(const DFPCmd& cmd) {
    bb::printf("CMD: Get U Files %d / %d --> %d\n", cmd.para1, cmd.para2, FileManager::inst.numFiles());
    DFPCmd c = cmd;
    c.para2 = FileManager::inst.numFiles();
    if(sendCommand(c) == false) bb::printf("Send failed\n");
}

void DFPHandler::cmdGetFolderFiles(const DFPCmd& cmd) {
    bb::printf("CMD: Get Folder Files %d / %d --> %d\n", cmd.para1, cmd.para2, FileManager::inst.numFilesInFolder(cmd.para2));
    DFPCmd c = cmd;
    c.para2 = FileManager::inst.numFilesInFolder(cmd.para2);
    if(sendCommand(c) == false) bb::printf("Send failed\n");
}

bool DFPHandler::sendCommand(const DFPCmd& cmd) {
    cmd.checksum = cmd.calcChecksum(true);
    uint8_t buf[sizeof(cmd)+4];
    buf[0] = 0x7e;
    buf[1] = 0xff;
    buf[2] = 6;
    memcpy(&(buf[3]), (uint8_t*)&cmd, sizeof(cmd));
    buf[sizeof(buf)-1]=0xef;

    if(ser_.write(buf, sizeof(buf)) != sizeof(buf)) return false; // end byte

    return true;
}
