#include <Arduino.h>
#include <FileManager.h>

FileManager FileManager::inst;

FileManager::FileManager() {
}

#define error(s) sd_.errorHalt(&Serial, F(s))

bool FileManager::start() {
    printf("Initializing SD card... ");

    if(!SD.begin(PIN_CS)) {
        printf("failed!\n");
        return false;
    }
    printf("done.\n");

    return true;
}

bool FileManager::stop() {
    return true;
}

bool FileManager::listFiles(const std::string& path) {
    File file, root;
    root = SD.open(path.c_str(), FILE_READ);
    if(!root) {
        printf("Failed to open /\n");
        return false;
    }
    printf("Files under '%s'\n", path.c_str());
    listFiles(root, true);
    root.close();
    return true;
}

bool FileManager::listFiles(File dir, bool recursive, std::string prefix) {
    File file;
    int fileCount = 0;
    while(file = dir.openNextFile(FILE_READ)) {
        printf("%s%s (0x%x) (%s)", prefix.c_str(), file.name(), file.size(), file.isDirectory() ? "DIR" : "FILE");
        if(file.isDirectory() && recursive) {
            printf(":\n");
            listFiles(file, recursive, prefix+"  ");
        } else {
            printf("\n");
        }
        file.close();
        fileCount++;
    }
    printf("%s(%d files)\n", prefix.c_str(), fileCount);
    return true;
}

bool FileManager::printContentsOfFile(const std::string& path) {
    File file = SD.open(path.c_str(), FILE_READ);
    if(!file) {
        printf("Couldn't open '%s'\n", path.c_str());
        return false;
    }
    
    printf("Contents of '%s'\n", path.c_str());
    unsigned int num = 0;
    int val;
    while((val = file.read()) != -1) {
        printf("%c", uint8_t(val));
        num++;
    }
    printf("\n%d bytes\n", num);
    file.close();
    return true;
}


bool FileManager::printCardInfo() {
    printf("Card type: ");
    switch(SD.cardType()) {
    case CARD_SD:
        printf("SD1\n");
        break;
    case CARD_SDHC:
        printf("SDHC\n");
        break;
    default:
        printf("Unknown.\n");
    }
    
#if 0
    int32_t freeClusterCount = SD.freeClusterCount();
    if (SD.fatType() <= 32) {
        printf("Volume is FAT%d\n",SD.fatType());
    } else {
        printf("Volume is exFAT\n");
    }
    printf("fatCount:          %d\n", int(SD.fatCount()));
    printf("sectorsPerCluster: %d\n", SD.sectorsPerCluster());
    printf("fatStartSector:    %d\n", SD.fatStartSector());
    printf("dataStartSector:   %d\n", SD.dataStartSector());
    printf("clusterCount:      %d\n", SD.clusterCount());
    printf("freeClusterCount:  ");
    if (freeClusterCount >= 0) {
        printf("%d\n", freeClusterCount);
    } else {
        printf("failed\n");
    }
#endif

    return true;
}