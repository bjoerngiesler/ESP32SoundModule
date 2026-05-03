#if !defined(FILEMANAGER_H)
#define FILEMANAGER_H

#include <map>
#include <FS.h>
#include <SD_MMC.h>
#include <SPI.h>
#include <SD.h>

using namespace fs;

class FileManager {
public:
    static FileManager inst;

    typedef std::map<unsigned int, std::string> FileMap;
    enum FileIndexingMode {
        IndexOrig,
        IndexAlnum,
        IndexNumAl
    };

    enum FileType {
        FileTypeFile = 0x1,
        FileTypeDirectory = 0x2
    };

    bool start();
    bool step() { return true; }
    bool stop(); 

    void buildFileMap();
    void buildFolderMap();

    const std::string& filename(unsigned int fileIndex);
    std::string filename(unsigned int folderIndex, unsigned int fileIndex);
    std::string foldername(unsigned int folderIndex);
    unsigned int numFolders();
    unsigned int numFiles();
    unsigned int numFilesInFolder(unsigned int folderIndex);

    bool isCardPresent();
    bool printCardInfo();
    bool printDirectory(const std::string& path="/", bool recursive=false);
    bool printContentsOfFile(const std::string& path);

    bool listFilesInDirectory(const std::string& path, 
                              std::vector<std::string> &files, 
                              bool includeDirectories=false);
    bool listFilesInDirectory(const std::string& path, 
                              FileMap &files, 
                              FileType typeFilter);

    void setFileIndexingMode(FileIndexingMode mode) { indexingMode_ = mode; }
    void setIgnoreDotfiles(bool ignore) { ignoreDotfiles_ = ignore; }
    void setIgnoreNonNumeric(bool ignore) { ignoreNonNumeric_ = ignore; }

protected:
    FileManager();
    bool printDirectory(File dir, bool recursive=false, std::string prefix="");
    FileMap fileMap_, folderMap_;
    bool sdInitialized_ = false;
    FileIndexingMode indexingMode_ = IndexNumAl;
    bool ignoreDotfiles_ = true;
    bool ignoreNonNumeric_ = false;
    std::string defaultFilename_ = "";
};

#endif // FILEMANAGER_H