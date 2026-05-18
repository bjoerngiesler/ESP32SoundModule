#include "StateManager.h"
#include "Player.h"
#include "FileManager.h"

StateManager StateManager::inst;

bool StateManager::playFile(const String& filename, bool allowPlaylist) {
    String f = filename;
    f.toLowerCase();

    // Playlists are stored in M3U form
    if(allowPlaylist && (f.endsWith("m3u") || f.endsWith("playlist"))) {
        if(readPlaylist(filename) == false) return false;
        if(playlist_.size() == 0) return false;

        if(playFile(playlist_[0].filename) == false) return false;
        playFromPlaylist_ = true;
        curInPlaylist_ = 0;
        nextTimeout_ = playlist_[0].timeout;
        return true;
    }

    playFromPlaylist_ = false;
    if(Player::inst.playFile(filename) == false) return false;

    curFile_ = NO_FILE;
    curFolder_ = NO_FILE;

    return true;
}

bool StateManager::playFile(unsigned int fileIndex, unsigned int folderIndex) {
    String filename;
    if(folderIndex == NO_FILE) {
        filename = FileManager::inst.filename(fileIndex);
    } else {
        filename = FileManager::inst.filename(folderIndex, fileIndex);
    }
        
    if(playFile(filename) == false) return false;
    curFile_ = fileIndex;
    curFolder_ = folderIndex;
    return true;
}

bool StateManager::isPlaying() {
    return Player::inst.isPlaying();
}

void StateManager::pause() {
    Player::inst.pausePlayback(!Player::inst.isPaused());
}

void StateManager::stop() {
    Player::inst.stopPlayback();
    curFile_ = NO_FILE;
    curFolder_ = NO_FILE;
}

bool StateManager::advance(int num) {
    // If in playlist, play the next item, looping back to 0 if playlistLoop_ is true.
    // If running out of playlist items, drop out at the bottom and play the next regular item.
    if(playFromPlaylist_) {
        curInPlaylist_ += num;
        if(playlistLoop_) curInPlaylist_ %= playlist_.size();
        if(curInPlaylist_ < playlist_.size()) {
            if(playFile(playlist_[curInPlaylist_].filename) == false) return false;
            nextTimeout_ = playlist_[curInPlaylist_].timeout;
            return true;
        }
    }

    // Dropped out of playlist or not in it in the first place
    playFromPlaylist_ = false;

    if(curFolder_ == NO_FILE && curFile_ == NO_FILE) return false;
    unsigned int numEntries = (curFolder_ == NO_FILE) ? FileManager::inst.numFiles() : FileManager::inst.numFilesInFolder(curFolder_);
    int cur = curFile_ + num;
    while(loop_ && cur < 0) {
        cur += numEntries;
    }
    if(loop_ && cur >= numEntries) cur %= numEntries;

    if(playFile(cur, curFolder_) == false) {        
        return false;
    }

    curFile_ = cur;
    return true;
}

bool StateManager::readPlaylist(const String& filename) {
    return false;
}
