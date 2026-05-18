#if !defined(CONFIGURATION_H)
#define CONFIGURATION_H

#include <map>
#include <string>
#include "FileManager.h"

class Configuration {
public:
    Configuration(const String& filename = "/config.txt");

    void clear();
    bool hasSection(const String& section);
    bool hasValue(const String& section, const String& key);
    String valueForKey(const String& section, const String& key, const char* defaultValue = "") const;
    float valueForKey(const String& section, const String& key, float defaultValue) const;
    int valueForKey(const String& section, const String& key, int defaultValue) const;
    bool valueForKey(const String& section, const String& key, bool defaultValue) const;
    void print();
protected:
    bool readFromFile(const String& filename);

    typedef std::map<String, String> Dictionary;
    std::map<String, Dictionary> sections_;

    bool readLine(File& file, String& line);
};

#endif // CONFIGURATION_H