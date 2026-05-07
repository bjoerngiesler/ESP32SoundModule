#if !defined(SETTINGS_H)
#define SETTINGS_H

#include <sys/types.h>
#include <AudioTools.h>

static const uint8_t P_SD_CS         = A0;

static const uint8_t P_I2S_DATA_OUT = 2;
static const uint8_t P_I2S_WS       = 3;
static const uint8_t P_I2S_BCK      = 4;

static const uint8_t P_CFG1     = A4;
static const uint8_t P_CFG2     = A5;

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define BITS_PER_SAMPLE 16
#define AUDIO_BUFFER_SIZE 2048

#define MIXER_INPUT_FILEPLAYER      0
#define MIXER_INPUT_SIGNALGENERATOR 1
#define NUM_MIXER_INPUTS            2

static const AudioInfo DEFAULT_AUDIO_INFO = { SAMPLE_RATE, NUM_CHANNELS, BITS_PER_SAMPLE };

//#define ONLY_FILE_PLAYER

// D6: TX
// D7: RX
// D8: SCK
// D9: MISO
// D10: MOSI

// That's all folks

#endif // SETTINGS_H