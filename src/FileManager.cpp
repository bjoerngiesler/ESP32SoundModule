#include <Arduino.h>
#include <FileManager.h>

FileManager FileManager::inst;

FileManager::FileManager() {
}

#define error(s) sd_.errorHalt(&Serial, F(s))

bool FileManager::start() {
    printf("Initializing SD card... ");

    if(!sd_.begin(SD_CONFIG)) {
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
    FILE_CLASS root, file;
    if(!root.open(path.c_str())) {
        printf("Failed to open /\n");
        return false;
    }
    printf("Files under '%s'\n", path.c_str());
    listFiles(root, true);
    root.close();
    return true;
}

bool FileManager::listFiles(FILE_CLASS& dir, bool recursive, std::string prefix) {
    FILE_CLASS file;
    int fileCount = 0;
    char name[255];
    while(file.openNext(&dir, O_RDONLY)) {
        memset(name, 0, 255);
        if(file.getName(name, 254) == 0) {
            printf("Error getting name of file #%d\n");
            file.close();
            return false;
        }
        printf("%s%s (0x%x) (%s)", prefix.c_str(), name, file.attrib(), file.isDir() ? "DIR" : "FILE");
        if(file.isDir() && recursive) {
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
    FILE_CLASS file;
    if(!file.open(path.c_str())) {
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
    switch(sd_.card()->type()) {
    case SD_CARD_TYPE_SD1:
        printf("SD1\n");
        break;
    case SD_CARD_TYPE_SD2:
        printf("SD2\n");
        break;
    case SD_CARD_TYPE_SDHC:
        printf("SDHC\n");
        break;
    default:
        printf("Unknown.\n");
    }

    int32_t freeClusterCount = sd_.freeClusterCount();
    if (sd_.fatType() <= 32) {
        printf("Volume is FAT%d\n",sd_.fatType());
    } else {
        printf("Volume is exFAT\n");
    }
    printf("fatCount:          %d\n", int(sd_.fatCount()));
    printf("sectorsPerCluster: %d\n", sd_.sectorsPerCluster());
    printf("fatStartSector:    %d\n", sd_.fatStartSector());
    printf("dataStartSector:   %d\n", sd_.dataStartSector());
    printf("clusterCount:      %d\n", sd_.clusterCount());
    printf("freeClusterCount:  ");
    if (freeClusterCount >= 0) {
        printf("%d\n", freeClusterCount);
    } else {
        printf("failed\n");
    }

    return true;
}