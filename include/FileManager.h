#if !defined(FILEMANAGER_H)
#define FILEMANAGER_H

#include <SdFat.h>

const uint8_t PIN_CS = A0;
#define SPI_CLOCK SD_SCK_MHZ(50)
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(PIN_CS, DEDICATED_SPI, SPI_CLOCK)
#else
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif

#if SD_FAT_TYPE == 0
#define SDFAT_CLASS SdFat
#define FILE_CLASS File
#elif SD_FAT_TYPE == 1
#define SDFAT_CLASS SdFat32
#define FILE_CLASS File32
#elif SD_FAT_TYPE == 2
#define SDFAT_CLASS SdExFat
#define FILE_CLASS ExFile
#elif SD_FAT_TYPE == 3
#define SDFAT_CLASS SdFs
#define FILE_CLASS FsFile
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

class FileManager {
public:
    static FileManager inst;

    bool start();
    bool step() { return true; }
    bool stop(); 

    bool printCardInfo();
    bool listFiles(const std::string& path="/");
    bool printContentsOfFile(const std::string& path);

protected:
    FileManager();
    bool listFiles(FILE_CLASS& dir, bool recursive, std::string prefix="");
    SDFAT_CLASS sd_;
};

#endif // FILEMANAGER_H