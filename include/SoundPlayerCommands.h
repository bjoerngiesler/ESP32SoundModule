#if !defined(SOUNDPLAYERCOMMANDS_H)
#define SOUNDPLAYERCOMMANDS_H

enum CommandCode {
    CMD_NEXT             = 0x01,
    CMD_PREV             = 0x02,
    CMD_PLAY             = 0x03,
    CMD_INC_VOL          = 0x04,
    CMD_DEC_VOL          = 0x05,
    CMD_VOLUME           = 0x06,
    CMD_EQ               = 0x07,
    CMD_PLAYBACK_MODE    = 0x08,
    CMD_PLAYBACK_SRC     = 0x09,
    CMD_STANDBY          = 0x0a,
    CMD_NORMAL           = 0x0b, // Come out of standby mode
    CMD_RESET            = 0x0c,
    CMD_PLAYBACK         = 0x0d,
    CMD_PAUSE            = 0x0e,
    CMD_PLAY_FOLDER      = 0x0f,
    CMD_VOL_ADJUST       = 0x10,
    CMD_REPEAT_PLAY      = 0x11,
    CMD_USE_MP3_FOLDER   = 0x12, // ??
    CMD_INSERT_ADVERT    = 0x13, // This interrupts the current track with an inserted one
    CMD_SPEC_TRACK_3000  = 0x14, // Play from specific track with up to 3000 files
    CMD_STOP_ADVERT      = 0x15, // This stops the interrupt and resumes playback on the original
    CMD_STOP             = 0x16,
    CMD_REPEAT_FOLDER    = 0x17,
    CMD_RANDOM_ALL       = 0x18,
    CMD_REPEAT_CURRENT   = 0x19,
    CMD_SET_DAC          = 0x20,

    CMD_GET_STAY_1       = 0x3c, // ?? According to original data sheet
    CMD_GET_STAY_2       = 0x3d, // ?? According to original data sheet
    CMD_GET_STAY_3       = 0x3e, // ?? According to original data sheet
    CMD_GET_SEND_INIT    = 0x3f,
    CMD_GET_RETRANSMIT   = 0x40,
    CMD_GET_REPLY        = 0x41, // ??
    CMD_GET_STATUS       = 0x42,
    CMD_GET_VOLUME       = 0x43,
    CMD_GET_EQ           = 0x44,
    CMD_GET_MODE         = 0x45,
    CMD_GET_VERSION      = 0x46,
    CMD_GET_TF_FILES     = 0x47,
    CMD_GET_U_FILES      = 0x48,
    CMD_GET_FLASH_FILES  = 0x49,
    CMD_KEEP_ON          = 0x4a,
    CMD_GET_TF_TRACK     = 0x4b,
    CMD_GET_U_TRACK      = 0x4c,
    CMD_GET_FLASH_TRACK  = 0x4d,
    CMD_GET_FOLDER_FILES = 0x4e,
    CMD_GET_FOLDERS      = 0x4f
};

enum Equalizer {
    EQ_NORMAL  = 0,
    EQ_POP     = 1,
    EQ_ROCK    = 2,
    EQ_JAZZ    = 3,
    EQ_CLASSIC = 4,
    EQ_BASE    = 5
};

enum PlayMode {
    MODE_REPEAT        = 0,
    MODE_FOLDER_REPEAT = 1,
    MODE_SINGLE_REPEAT = 2,
    MODE_RANDOM        = 3
};

enum PlaySource {
    SRC_U     = 1,
    SRC_TF    = 2,
    SRC_AUX   = 3,
    SRC_SLEEP = 4,
    SRC_FLASH = 5
};

struct Command {
    CommandCode cmd;
    bool feedback;
    uint8_t para1, para2;
};

#endif // SOUNDPLAYERCOMMANDS_H