#if !defined(FILEMANAGER_H)
#define FILEMANAGER_H

#include <FS.h>
#include <SD_MMC.h>
#include <SPI.h>
#include <SD.h>

using namespace fs;

const uint8_t PIN_CS = A0;
const uint8_t PIN_MISO = MISO;
const uint8_t PIN_MOSI = MOSI;
const uint8_t PIN_SCK = SCK;

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
    bool listFiles(File dir, bool recursive, std::string prefix="");
};

#endif // FILEMANAGER_H