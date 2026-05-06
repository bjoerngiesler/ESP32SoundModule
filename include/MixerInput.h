#if !defined(MIXERINPUT_H)
#define MIXERINPUT_H

#include <Arduino.h>
#include <AudioTools.h>

class MixerInput {
public:
    virtual Stream& output() = 0;
    void setMixerIndex(int i) { mixerIndex_ = i; }
    int mixerIndex() { return mixerIndex_; }
    void setWeight(uint8_t w) { weight_ = w; }
    uint8_t weight() { return weight_; }
protected:
    int mixerIndex_ = 0;
    uint8_t weight_ = 100;
};

#endif // MIXERINPUT_H