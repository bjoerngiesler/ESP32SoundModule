#include <SD.h>

#include "MemFile.h"
#include "FileManager.h"

MemFile::MemFile(): buf_(nullptr), size_(0), filename_("") {
}


MemFile::MemFile(const String& filename): buf_(nullptr), size_(0), filename_("") {
    String fname = FileManager::inst.normalizePath(filename);
    File f = SD.open(fname);
    if(!f) {
        return;
    } 

    size_t size = f.size();
    buf_ = (uint8_t*)ps_malloc(size);
    if(buf_ == nullptr) {
        return;
    }
    size_ = size;

    for(size_t i=0; i<size; i++) {
        int byte = f.read();
        if(byte < 0) {
            free((void*)buf_);
            buf_ = nullptr;
            size_ = 0;
            return;
        }
        buf_[i] = uint8_t(byte);
    }

    filename_ = fname;
}

MemFile::~MemFile() {
    if(buf_ != nullptr) {
        free(buf_);
        buf_ = nullptr;
    }
    size_ = 0;
    filename_ = "";
}

MemFile& MemFile::operator=(const MemFile& other) {
    if(buf_ != nullptr) {
        free(buf_);
        buf_ = nullptr;
    }
    size_ = 0;
    filename_ = "";

    buf_ = (uint8_t*)ps_malloc(other.size_);
    if(buf_ == nullptr) return *this;
    memcpy(buf_, other.buf_, other.size_);
    size_ = other.size_;
    filename_ = other.filename_;

    return *this;
}
