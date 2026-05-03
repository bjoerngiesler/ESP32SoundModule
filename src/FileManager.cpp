#include <Arduino.h>
#include <FileManager.h>
#include "Settings.h"

FileManager FileManager::inst;

FileManager::FileManager() {
}

#define error(s) sd_.errorHalt(&Serial, F(s))

bool FileManager::start() {
    sdInitialized_ = false;
    printf("Initializing SD card... ");
    if(!SD.begin(PIN_CS)) {
        printf("failed!\n");
        return false;
    }
    printf("done.\n");

    sdInitialized_ = true;
    return true;
}

bool FileManager::stop() {
    return true;
}

bool FileManager::isCardPresent() {
    std::vector<std::string> files;

    if(sdInitialized_ == false) {
        if(!SD.begin(PIN_CS)) {
            printf("failed!\n");
            return false;
        }
        sdInitialized_ = true;
    }

    if(listFilesInDirectory("/", files) == true) {
        //Serial.printf("SD card is present. Root directory contains %d entries.\n", int(files.size()));
        return true;
    }
    return false;
}

bool FileManager::listFilesInDirectory(const std::string& path, 
                                       std::vector<std::string> &files, 
                                       bool includeDirectories) {
    File file, root;
    root = SD.open(path.c_str(), FILE_READ);
    if(!root) {
        printf("Failed to open '%s'\n", path.c_str());
        return false;
    }
    while(file = root.openNextFile(FILE_READ)) {
        if(file.isDirectory()) {
            if(includeDirectories) {
                files.push_back(std::string(file.name()));
            }
        } else {
            files.push_back(std::string(file.name()));
        }
        file.close();
    }
    root.close();
    return true;
}

bool FileManager::listFilesInDirectory(const std::string& path, 
                                       FileMap &files, 
                                       FileType typeFilter) {
    File file, root;
    root = SD.open(path.c_str(), FILE_READ);
    if(!root) {
        printf("Failed to open '%s'\n", path.c_str());
        return false;
    }
    std::vector<std::string> names;
    while(file = root.openNextFile(FILE_READ)) {
        std::string name = file.name();
        if(ignoreDotfiles_ && name[0] == '.') {
            file.close();
            continue;
        }
        if(ignoreNonNumeric_ && std::isdigit(name[0]) == 0) {
            file.close();
            continue;
        }
        if(name.rfind(".ini") != std::string::npos) {
            file.close();
            continue;
        }
        if((typeFilter & FileTypeFile) && !file.isDirectory()) {
            names.push_back(name);
        }
        if((typeFilter & FileTypeDirectory) && file.isDirectory()) {
            names.push_back(name);
        }
        file.close();
    }
    root.close();

    files.clear();
    unsigned int index = 0, maxIndex = 0;
    if(indexingMode_ == IndexOrig) {
        for(const std::string& name : names) {
            files[index+1] = name;
            index++;
        }
    } else if(indexingMode_ == IndexAlnum) {
        std::sort(names.begin(), names.end());
        for(const std::string& name : names) {
            files[index+1] = name;
            index++;
        }
    } else if(indexingMode_ == IndexNumAl) {
        std::vector<std::string> alphaNames;
        for(const std::string& name : names) {
            if(std::isdigit(name[0]) == 0) { // not a digit
                alphaNames.push_back(name);
                continue;
            }
            index = 0;
            for(char c: name) {
                if(std::isdigit(c)) {
                    index = index*10 + (c-'0');
                } else {
                    break;
                }
            }
            files[index] = name;
            if(index > maxIndex) {
                maxIndex = index;
            }
        }
        unsigned int alphaIndex = maxIndex+1;
        std::sort(alphaNames.begin(), alphaNames.end());
        for(const std::string& name : alphaNames) {
            files[alphaIndex++] = name;
        }
    }

    return true;
}

bool FileManager::printDirectory(const std::string& path, bool recursive) {
    File file, root;
    root = SD.open(path.c_str(), FILE_READ);
    if(!root) {
        printf("Failed to open /\n");
        return false;
    }
    printf("Files under '%s'\n", path.c_str());
    bool retval = printDirectory(root, recursive);
    root.close();
    return retval;
}

bool FileManager::printDirectory(File dir, bool recursive, std::string prefix) {
    File file;
    int fileCount = 0;
    while(file = dir.openNextFile(FILE_READ)) {
        printf("%s%s (0x%x) (%s)", prefix.c_str(), file.name(), file.size(), file.isDirectory() ? "DIR" : "FILE");
        if(file.isDirectory() && recursive) {
            printf(":\n");
            printDirectory(file, recursive, prefix+"  ");
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

void FileManager::buildFileMap() {
    listFilesInDirectory("/", fileMap_, FileTypeFile);
    for(auto& [index, name] : fileMap_) {
        printf("Indexed file: %d: %s\n", index, name.c_str());
    }
}

void FileManager::buildFolderMap() {
    listFilesInDirectory("/", folderMap_, FileTypeDirectory);
    for(auto& [index, name] : folderMap_) {
        printf("Indexed folder: %d: %s\n", index, name.c_str());
    }
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
    
    return true;
}

const std::string& FileManager::filename(unsigned int fileIndex) {
    for(auto& [index, name]: folderMap_) {
        if(index == fileIndex) return name;
    }
    return defaultFilename_;
}

std::string FileManager::filename(unsigned int folderIndex, unsigned int fileIndex) {
    std::string f = foldername(folderIndex);
    if(f == "") {
        printf("No folder name found for folder %d\n", folderIndex);
        return defaultFilename_;
    }
    f = std::string("/") + f;
    printf("Folder name '%s' at index %d\n", f.c_str(), folderIndex);

    FileMap fm;
    listFilesInDirectory(f, fm, FileTypeFile);

    for(auto& [index, name]: fm) {
        if(index == fileIndex) {
            std::string path = f + "/" + name;
            printf("File name '%s' at index %d. Returning %s\n", name.c_str(), folderIndex, path.c_str());
            return path;
        }
    }

    printf("No file name found for file %d\n", fileIndex);
    return defaultFilename_;
}

std::string FileManager::foldername(unsigned int folderIndex) {
    const std::string empty="";
    for(auto& [index, name]: folderMap_) {
        if(index == folderIndex) return name;
    }
    return empty;
}

unsigned int FileManager::numFolders() {
    unsigned int maxIndex = 0;
    for(auto& [index, name] : folderMap_) {
        if(index > maxIndex) maxIndex = index;
    }
    return maxIndex;
}

unsigned int FileManager::numFiles() {
    unsigned int maxIndex = 0;
    for(auto& [index, name] : fileMap_) {
        if(index > maxIndex) maxIndex = index;
    }
    return maxIndex;
}

unsigned int FileManager::numFilesInFolder(unsigned int folderIndex) {
    std::string f = foldername(folderIndex);
    if(f == "") return 0;

    FileMap fm;
    listFilesInDirectory(f, fm, FileTypeFile);
    unsigned int maxIndex = 0;
    for(auto& [index, name]: fm) {
        if(index > maxIndex) maxIndex = index;
    }
    return maxIndex;
}
