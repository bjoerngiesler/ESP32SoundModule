#include "Configuration.h"
#include "FileManager.h"

Configuration::Configuration(const String& filename) {
    readFromFile(filename);
}

void Configuration::clear() {
    sections_ = std::map<String, Dictionary>();
}

bool Configuration::readFromFile(const String& filename) {
    File file;
    
    if(!(file = SD.open(filename.c_str(), FILE_READ))) return false;

    Dictionary currentSection = sections_["global"];
    String currentSectionName = "global";

    String line;
    while(readLine(file, line)) {
        if(line[0] == '#' || line[0] == ';' || line.isEmpty()) continue; // Comment or empty line

        if(line[0] == '[' && line[line.length()-1] == ']') {
            String sectionName = line.substring(1, line.length()-1);
            if(sectionName.length() == 0) {
                bb::printf("Invalid section name: '%s'\n", line.c_str());
                continue; // Invalid section name
            }
            sections_[currentSectionName] = currentSection;
            currentSection = Dictionary();
            currentSectionName = sectionName;
            continue;
        }

        int eqPos = line.indexOf('=');
        if(eqPos == -1) {
            continue; // Invalid line
        }
        String key = line.substring(0, eqPos);
        while(!key.isEmpty() && isspace(key[key.length()-1])) key = key.substring(0, key.length()-1); // Trim trailing whitespace from key
        while(!key.isEmpty() && isspace(key[0])) key = key.substring(1, key.length()); // Trim leading whitespace from key
        String value = line.substring(eqPos+1);
        while(!value.isEmpty() && isspace(value[value.length()-1])) value = value.substring(0, value.length()-1); // Trim trailing whitespace from value
        while(!value.isEmpty() && isspace(value[0])) value = value.substring(1, value.length()); // Trim
        if(key.length() == 0 || value.length() == 0) {
            continue; // Invalid line
        }
        if(key.length() >= 2 && 
           (key[0] == '"' && key[key.length()-1] == '"') ||
           (key[0] == '\'' && key[key.length()-1] == '\'')) {
            key = key.substring(1, key.length()-1); // Remove quotes around key
        }
        if(value.length() >= 2 && 
           (value[0] == '"' && value[value.length()-1] == '"') ||
           (value[0] == '\'' && value[value.length()-1] == '\'')) {
            value = value.substring(1, value.length()-1); // Remove quotes around value
        }
        if(key.length() == 0 || value.length() == 0) {
            continue; // Invalid line
        }
        currentSection[key] = value;
    }

    sections_[currentSectionName] = currentSection;
    file.close();
    return true;
}

void Configuration::print() {
    for(const auto& section : sections_) {
        bb::printf("[%s]\n", section.first.c_str());
        for(const auto& kv : section.second) {
            bb::printf("%s=%s\n", kv.first.c_str(), kv.second.c_str());
        }
    }
}

bool Configuration::readLine(File& file, String& line) {
    line = "";
    int val;
    while((val = file.read()) != -1) {
        if(val == '\n') break;
        line += char(val);
    }
    return line.length() > 0 || val != -1;
}

bool Configuration::hasSection(const String& section) {
    return sections_.find(section) != sections_.end();
}

bool Configuration::hasValue(const String& section, const String& key) {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return false;
    return secIt->second.find(key) != secIt->second.end();
}

String Configuration::valueForKey(const String& section, const String& key, const char* defaultValue) const {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return defaultValue;
    auto keyIt = secIt->second.find(key);
    if(keyIt == secIt->second.end()) return defaultValue;
    return keyIt->second;
}

float Configuration::valueForKey(const String& section, const String& key, float defaultValue) const {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return defaultValue;
    auto keyIt = secIt->second.find(key);
    if(keyIt == secIt->second.end()) return defaultValue;
    
    float var;
    if(sscanf(keyIt->second.c_str(), "%f", &var) != 1) {
        bb::printf("Could not convert value '%s' for key '[%s]:%s to float. Returning default %f.", 
               keyIt->second.c_str(), section.c_str(), key.c_str(), defaultValue);
        return defaultValue;
    }
    return var;
}

int Configuration::valueForKey(const String& section, const String& key, int defaultValue) const {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return defaultValue;
    auto keyIt = secIt->second.find(key);
    if(keyIt == secIt->second.end()) return defaultValue;
    
    int var;
    if(sscanf(keyIt->second.c_str(), "%d", &var) != 1) {
        bb::printf("Could not convert value '%s' for key '[%s]:%s to int. Returning default %d.", 
               keyIt->second.c_str(), section.c_str(), key.c_str(), defaultValue);
        return defaultValue;
    }
    return var;    
}

bool Configuration::valueForKey(const String& section, const String& key, bool defaultValue) const {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return defaultValue;
    auto keyIt = secIt->second.find(key);
    if(keyIt == secIt->second.end()) return defaultValue;
    
    String val = keyIt->second;
    std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c){return std::tolower(c);});
    if(val == "true" || val == "on" || val == "yes") return true;
    if(val == "false" || val == "off" || val == "no") return false;
    bb::printf("Could not convert value '%s' for key '[%s]:%s to bool. Returning default %s.", 
            keyIt->second.c_str(), section.c_str(), key.c_str(), defaultValue ? "true" : "false");
    return defaultValue;
}