#if !defined(MEMFILE_H)
#define MEMFILE_H

#include <Arduino.h>

class MemFile {
public:
    MemFile();
    MemFile(const String& filename);
    ~MemFile();

    MemFile& operator=(const MemFile& other);

    const uint8_t *buffer() const { return buf_; }
    size_t size() const { return size_; }
    const String& filename() const { return filename_; }

protected:
    uint8_t *buf_;
    size_t size_;
    String filename_;
};

#endif // MEMFILE_H