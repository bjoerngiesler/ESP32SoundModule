#include <Arduino.h>
#include <FileManager.h>
#include "Settings.h"
#include <deque>

FileManager FileManager::inst;

FileManager::FileManager() {
}

#define error(s) sd_.errorHalt(&Serial, F(s))

bool FileManager::start() {
    sdInitialized_ = false;
    bb::printf("Initializing SD card... ");
    if(!SD.begin(P_SD_CS)) {
        bb::printf("failed!\n");
        return false;
    }
    bb::printf("done.\n");

    sdInitialized_ = true;
    checkSDCardPresent();
    return true;
}

bool FileManager::stop() {
    return true;
}

bool FileManager::checkSDCardPresent() {
    if(sdInitialized_ == false) {
        if(!SD.begin(P_SD_CS)) {
            bb::printf("failed!\n");
            return false;
        }
        sdInitialized_ = true;
    }

    File root;
    if(sdPresent_) {
        root = SD.open("/");
        if(!root) {
            sdPresent_ = false;
            SD.end();
            for(auto& cb: sdCardInsertCallbacks_) cb(false);
        } else {
            root.close();
        }
    } else {
        if(SD.begin(P_SD_CS) == true) {
            root = SD.open("/");
            if(root) {
                sdPresent_ = true;
                root.close();
                for(auto& cb: sdCardInsertCallbacks_) cb(true);
            } else {
                SD.end();
            }
        }
    }

    return sdPresent_;
}

bool FileManager::listFilesInDirectory(const String& path, 
                                       std::vector<String> &files, 
                                       bool includeDirectories) {
    File file, root;
    root = SD.open(path.c_str(), FILE_READ);
    if(!root) {
        bb::printf("Failed to open '%s'\n", path.c_str());
        return false;
    }
    while(file = root.openNextFile(FILE_READ)) {
        if(file.isDirectory()) {
            if(includeDirectories) {
                files.push_back(path + file.name());
            }
        } else {
            files.push_back(path + file.name());
        }
        file.close();
    }
    root.close();
    return true;
}

bool FileManager::listFilesInDirectory(const String& path, 
                                       FileMap &files, 
                                       FileType typeFilter) {
    File file, root;
    root = SD.open(path.c_str(), FILE_READ);
    if(!root) {
        bb::printf("Failed to open '%s'\n", path.c_str());
        return false;
    }
    std::vector<String> names;

    while(file = root.openNextFile(FILE_READ)) {
        String name = file.name();
        if(ignoreDotfiles_ && name[0] == '.') {
            file.close();
            continue;
        }
        if(ignoreNonNumeric_ && std::isdigit(name[0]) == 0) {
            bb::printf("Ignore Nonnumeric - not counting '%s'\n", name.c_str());
            file.close();
            continue;
        }
        if(name.endsWith(".ini")) {
            file.close();
            continue;
        }
        if((typeFilter & FileTypeFile) && !file.isDirectory()) {
            names.push_back(path.endsWith("/") ? path + name : path + "/" + name);
        }
        if((typeFilter & FileTypeDirectory) && file.isDirectory()) {
            names.push_back(path.endsWith("/") ? path + name : path + "/" + name);
        }
        file.close();
    }
    root.close();

    files.clear();
    unsigned int index = 0, maxIndex = 0;
    if(indexingMode_ == IndexOrig) {
        for(const String& name : names) {
            files[index+1] = name;
            index++;
        }
    } else if(indexingMode_ == IndexAlnum) {
        std::sort(names.begin(), names.end());
        for(const String& name : names) {
            files[index+1] = name;
            index++;
        }
    } else if(indexingMode_ == IndexNumAl) {
        std::vector<String> alphaNames;
        for(const String& name : names) {
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
        for(const String& name : alphaNames) {
            files[alphaIndex++] = name;
        }
    }

    return true;
}

bool FileManager::printDirectory(const String& path, bool recursive) {
    File file, root;
    root = SD.open(path.c_str(), FILE_READ);
    if(!root) {
        bb::printf("Failed to open /\n");
        return false;
    }
    bb::printf("Files under '%s'\n", path.c_str());
    bool retval = printDirectory(root, recursive);
    root.close();
    return retval;
}

bool FileManager::printDirectory(File dir, bool recursive, String prefix) {
    File file;
    int fileCount = 0;
    while(file = dir.openNextFile(FILE_READ)) {
        bb::printf("%s%s (0x%x) (%s)", prefix.c_str(), file.name(), file.size(), file.isDirectory() ? "DIR" : "FILE");
        if(file.isDirectory() && recursive) {
            bb::printf(":\n");
            printDirectory(file, recursive, prefix+"  ");
        } else {
            bb::printf("\n");
        }
        file.close();
        fileCount++;
    }
    bb::printf("%s(%d files)\n", prefix.c_str(), fileCount);
    return true;
}

bool FileManager::printContentsOfFile(const String& path) {
    File file = SD.open(path.c_str(), FILE_READ);
    if(!file) {
        bb::printf("Couldn't open '%s'\n", path.c_str());
        return false;
    }
    
    bb::printf("Contents of '%s'\n", path.c_str());
    unsigned int num = 0;
    int val;
    while((val = file.read()) != -1) {
        bb::printf("%c", uint8_t(val));
        num++;
    }
    bb::printf("\n%d bytes\n", num);
    file.close();
    return true;
}

void FileManager::buildFileMap() {
    fileMap_.clear();

    listFilesInDirectory("/", fileMap_[0], FileTypeFile);

    FileMap dirMap;
    listFilesInDirectory("/", dirMap, FileTypeDirectory);

    for(auto& [index, name] : dirMap) {
        listFilesInDirectory(name, fileMap_[index], FileTypeFile);
    }
}

void FileManager::printFileMap(ConsoleStream *stream) {
    for(auto& [index, fmap]: fileMap_) {
        if(index == 0) {
            if(stream) stream->printf("Folder 0 (/): %d files\n", fmap.size());
            else bb::printf("Folder 0 (/): %d files\n", fmap.size());
        } else {
            if(stream) stream->printf("Folder %d: %d files\n", index, fmap.size());
            else bb::printf("Folder %d: %d files\n", index, fmap.size());
        }
        for(auto& [i, name]: fileMap_[index]) {
            if(stream) stream->printf("\t%d -> '%s'\n", i, name.c_str());
            else bb::printf("\t%d -> '%s'\n", i, name.c_str());
        }
    }
}

bool FileManager::printCardInfo() {
    bb::printf("Card type: ");
    switch(SD.cardType()) {
    case CARD_SD:
        bb::printf("SD1\n");
        break;
    case CARD_SDHC:
        bb::printf("SDHC\n");
        break;
    default:
        bb::printf("Unknown.\n");
    }
    
    return true;
}

const String& FileManager::filename(unsigned int fileIndex) {
    if(fileMap_[0].find(fileIndex) == fileMap_[0].end()) {
        return defaultFilename_;
    }
    return fileMap_[0][fileIndex];
}

String FileManager::filename(unsigned int folderIndex, unsigned int fileIndex) {
    if(fileMap_.find(folderIndex) == fileMap_.end()) {
        bb::printf("No folder %d\n", folderIndex);
        return defaultFilename_;
    }
    if(fileMap_[folderIndex].find(fileIndex) == fileMap_[folderIndex].end()) {
        bb::printf("No file name found for file %d/%d\n", folderIndex, fileIndex);
        return defaultFilename_;
    }
    return fileMap_[folderIndex][fileIndex];
}

unsigned int FileManager::numFolders() {
    unsigned int maxIndex = 0;
    for(auto& [index, name] : fileMap_) {
        if(index > maxIndex) maxIndex = index;
    }
    return maxIndex;
}

unsigned int FileManager::numFiles() {
    unsigned int maxIndex = 0;
    for(auto& [index, name] : fileMap_[0]) {
        if(index > maxIndex) maxIndex = index;
    }
    return maxIndex;
}

unsigned int FileManager::numFilesInFolder(unsigned int folderIndex) {
    if(fileMap_.find(folderIndex) == fileMap_.end()) {
        bb::printf("No folder %d\n", folderIndex);
        return 0;
    }
    return fileMap_[folderIndex].size();
}

bool FileManager::fileExists(const String& path) {
    return SD.exists(path.c_str());
}

String FileManager::normalizePath(const String& p) {
    String path = p;

    if(!path.startsWith("/")) path = curWd_ + "/" + path;
    std::deque<String> components;
    const char *c_ptr = path.c_str();
    char *comp = strtok((char*)c_ptr, "/");
    while(comp != nullptr) {
        if(strcmp(comp, "..") == 0 && components.size() > 0) {
            components.pop_back();
        } else {
            components.push_back(String(comp));
        }
        comp = strtok(nullptr, "/");
    }

    if(components.size() == 0) {
        return "/";
    }

    path = "/";
    for(size_t i=0; i<components.size()-1; i++) {
        path = path + components[i] + "/";
    }
    path = path + components[components.size()-1];

    return path;
}

void FileManager::addSDCardInsertCallback(std::function<void(bool)> cb) {
    sdCardInsertCallbacks_.push_back(cb);
}


Result FileManager::lsCmd(const std::vector<String>& args, ConsoleStream *stream) {
    Runloop::runloop.excuseOverrun();
    if(args.size() == 0) {
        if(printDirectory(curWd_) == true) return RES_OK;
        return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
    }
    String path = normalizePath(args[0]);
    if(printDirectory(path) == true) return RES_OK;
    return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
}

Result FileManager::catCmd(const std::vector<String>& args, ConsoleStream *stream) {
    Runloop::runloop.excuseOverrun();
    String path = normalizePath(args[0]);
    if(fileExists(path) == false) return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
    File f = SD.open(path);
    uint8_t buf[256];
    int numRead;
    while(f.available()) {
        numRead = f.read(buf, sizeof(buf));
        for(unsigned int i=0; i<numRead; i++)
            stream->printf("%c", buf[i]);
    }
    stream->printf("\n");
    return RES_OK;
}

Result FileManager::cdCmd(const std::vector<String>& args, ConsoleStream *stream) {
    if(args.size() == 0) {
        curWd_ = "/";
        return RES_OK;
    }

    String path = normalizePath(args[0]);
    if(path == "/") {
        curWd_ = "/";
        return RES_OK;
    }

    if(SD.exists(path)) {
        File f = SD.open(path);
        if(f.isDirectory()) {
            f.close();
            curWd_ = path;
            return RES_OK;
        } else {
            f.close();
            return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
        }
    }

    return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
}

Result FileManager::pwdCmd(const std::vector<String>& args, ConsoleStream *stream) {
    if(curWd_.indexOf(" ") != -1) {
        stream->printf("\"%s\"\n", curWd_.c_str());
    } else {
        stream->printf("%s\n", curWd_.c_str());
    }
    return RES_OK;
}

Result FileManager::mvCmd(const std::vector<String>& args, ConsoleStream *stream) {
    String path1 = normalizePath(args[0]);
    String path2 = normalizePath(args[1]);
    if(SD.rename(path1, path2) == true) return RES_OK;
    return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
}

Result FileManager::filemapCmd(const std::vector<String>& args, ConsoleStream *stream) {
    printFileMap(stream);
    return RES_OK;
}
