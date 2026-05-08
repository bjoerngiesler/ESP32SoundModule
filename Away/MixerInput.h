#if !defined(MIXERINPUT_H)
#define MIXERINPUT_H

#include <Arduino.h>
#include <AudioTools.h>
#include "Settings.h"

class MixerInput {
public:
    virtual Stream& output() = 0;
    virtual void setVolume(float vol) = 0;

    void setActive(bool active=true) { active_ = active; }
    bool isActive() { return active_; }

protected:
    bool active_ = false;
};

#endif // MIXERINPUT_H