#if !defined(FILEMANAGER_H)
#define FILEMANAGER_H

#include <map>
#include <FS.h>
#include <SD_MMC.h>
#include <SPI.h>
#include <SD.h>
#include <LibBB.h>

using namespace fs;
using namespace bb;

class FileManager {
public:
    static FileManager inst;

    typedef std::map<unsigned int, String> FileMap;
    typedef std::map<unsigned int, FileMap> FolderMap;

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
    bool step();
    bool stop(); 

    void buildFileMap();
    void printFileMap(ConsoleStream* stream = nullptr);

    const String& filename(unsigned int fileIndex);
    String filename(unsigned int folderIndex, unsigned int fileIndex);
    String foldername(unsigned int folderIndex);
    unsigned int numFolders();
    unsigned int numFiles();
    unsigned int numFilesInFolder(unsigned int folderIndex);

    bool checkSDCardPresent();
    bool printCardInfo();
    bool printDirectory(const String& path="/", bool recursive=false);
    bool printContentsOfFile(const String& path);

    bool listFilesInDirectory(const String& path, 
                              std::vector<String> &files, 
                              bool includeDirectories=false);
    bool listFilesInDirectory(const String& path, 
                              FileMap &files, 
                              FileType typeFilter);

    void setFileIndexingMode(FileIndexingMode mode) { indexingMode_ = mode; }
    void setIgnoreDotfiles(bool ignore) { ignoreDotfiles_ = ignore; }
    void setIgnoreNonNumeric(bool ignore) { ignoreNonNumeric_ = ignore; }

    bool fileExists(const String& path);

    String normalizePath(const String& p);

    void addSDCardInsertCallback(std::function<void(bool)> cb);

    Result lsCmd(const std::vector<String>& args, ConsoleStream *stream);
    Result catCmd(const std::vector<String>& args, ConsoleStream *stream);
    Result cdCmd(const std::vector<String>& args, ConsoleStream *stream);
    Result pwdCmd(const std::vector<String>& args, ConsoleStream *stream);
    Result mvCmd(const std::vector<String>& args, ConsoleStream *stream);
    Result filemapCmd(const std::vector<String>& args, ConsoleStream *stream);
protected:
    FileManager();
    bool printDirectory(File dir, bool recursive=false, String prefix="");
    FolderMap fileMap_;
    bool sdInitialized_ = false;
    bool sdPresent_ = false;
    FileIndexingMode indexingMode_ = IndexNumAl;
    bool ignoreDotfiles_ = true;
    bool ignoreNonNumeric_ = false;
    String defaultFilename_ = "";
    String curWd_ = "/";

    std::vector<std::function<void(bool)>> sdCardInsertCallbacks_;
};

#endif // FILEMANAGER_H