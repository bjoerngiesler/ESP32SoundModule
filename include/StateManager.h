#if !defined(STATE_MANAGER)
#define STATE_MANAGER

#include <Arduino.h>
#include <limits.h>
#include <vector>

class StateManager {
public:
    static StateManager inst;

    bool playFile(unsigned int fileIndex, unsigned int folderIndex = NO_FILE);
    bool playFile(const String& filename, bool allowPlaylist = true);

    bool isPlaying();

    void pause();
    void stop();

    bool next() { return advance(1); }
    bool previous() { return advance(-1); }
    bool advance(int tracks);

protected:
    static const int NO_FILE = UINT_MAX;
    unsigned int curFile_ = NO_FILE;
    unsigned int curFolder_ = NO_FILE;
    
    bool readPlaylist(const String& filename);

    struct PlaylistEntry {
        String filename;
        float timeout;
    };

    std::vector<PlaylistEntry> playlist_;
    unsigned int curInPlaylist_ = NO_FILE;
    bool playFromPlaylist_ = false;
    float nextTimeout_ = 0;
    bool autonext_ = false, playlistAutonext_ = true;
    bool loop_ = false, playlistLoop_ = false;
};

#endif // STATE_MANAGER