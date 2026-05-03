#if !defined(CONFIGURATION_H)
#define CONFIGURATION_H

#include <map>
#include <string>
#include "FileManager.h"

class Configuration {
public:
    Configuration(const std::string& filename = "/config.txt");

    void clear();
    bool hasSection(const std::string& section);
    bool hasValue(const std::string& section, const std::string& key);
    const std::string& valueForKey(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    void print();
protected:
    bool readFromFile(const std::string& filename);

    typedef std::map<std::string, std::string> Dictionary;
    std::map<std::string, Dictionary> sections_;

    bool readLine(File& file, std::string& line);
};

#endif // CONFIGURATION_H