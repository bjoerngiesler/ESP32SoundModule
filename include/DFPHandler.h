#if !defined(DFPHANDLER_H)
#define DFPHANDLER_H

#include <sys/types.h>
#include <map>
#include <functional>
#include <DFPCommands.h>

class DFPHandler {
public:
    static DFPHandler inst;

    bool start();
    bool step();
    bool stop();

    void cmdReset(const Command& cmd);
    void cmdSetVolume(const Command& cmd);
    void cmdStopPlayback(const Command& cmd);
    void cmdPlayFolder(const Command& cmd);
protected:
    static std::map<CommandCode, std::function<void(const Command& cmd)>> callbackMap_;
    bool waitAvailable(unsigned int timeout, uint8_t& byte);
    bool readCommand(Command& command, unsigned int timeout);
    uint16_t calcChecksum(const Command& cmd);
    bool checkChecksum(const Command& cmd);
};

#endif // DFPHANDLER_H