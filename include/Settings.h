#if !defined(SETTINGS_H)
#define SETTINGS_H

#include <sys/types.h>

static const uint8_t PIN_CS           = A0;

static const uint8_t PIN_I2S_DATA_OUT = 2;
static const uint8_t PIN_I2S_WS       = 3;
static const uint8_t PIN_I2S_BCK      = 4;

static const uint8_t PIN_CFG1     = A4;
static const uint8_t PIN_CFG2     = A5;

#define DEFAULT_BUFFER_SIZE 2048

// D6: TX
// D7: RX
// D8: SCK
// D9: MISO
// D10: MOSI

// That's all folks

#endif // SETTINGS_H