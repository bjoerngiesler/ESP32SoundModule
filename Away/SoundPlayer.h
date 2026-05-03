#if !defined(SOUNDPLAYER_H)
#define SOUNDPLAYER_H

#include "SoundPlayerCommands.h"
#include <Audio.h>
#include <map>
#include <functional>

class SoundPlayer {
public:
    static SoundPlayer inst;
    bool start();
    bool step();
    bool stop();
    bool playFile(const std::string& path);

protected:
    SoundPlayer();
    static std::map<CommandCode, std::function<bool(const Command& cmd)>> callbackMap_;
    bool waitAvailable(unsigned int timeout, uint8_t& byte);
    bool readCommand(Command& command, unsigned int timeout);

    Audio* audio_;
};

#endif // SOUNDPLAYER_H