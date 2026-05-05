#include "Configuration.h"
#include "FileManager.h"

Configuration::Configuration(const std::string& filename) {
    readFromFile(filename);
}

void Configuration::clear() {
    sections_ = std::map<std::string, Dictionary>();
}

bool Configuration::readFromFile(const std::string& filename) {
    File file;
    
    if(!(file = SD.open(filename.c_str(), FILE_READ))) return false;

    Dictionary currentSection = sections_["global"];
    std::string currentSectionName = "global";

    std::string line;
    while(readLine(file, line)) {
        if(line[0] == '#' || line[0] == ';' || line.empty()) continue; // Comment or empty line

        if(line[0] == '[' && line[line.length()-1] == ']') {
            std::string sectionName = line.substr(1, line.length()-2);
            if(sectionName.length() == 0) {
                printf("Invalid section name: '%s'\n", line.c_str());
                continue; // Invalid section name
            }
            sections_[currentSectionName] = currentSection;
            currentSection = Dictionary();
            currentSectionName = sectionName;
            continue;
        }

        size_t eqPos = line.find('=');
        if(eqPos == std::string::npos) {
            continue; // Invalid line
        }
        std::string key = line.substr(0, eqPos);
        while(!key.empty() && isspace(key.back())) key.pop_back(); // Trim trailing whitespace from key
        while(!key.empty() && isspace(key.front())) key.erase(key.begin()); // Trim leading whitespace from key
        std::string value = line.substr(eqPos+1);
        while(!value.empty() && isspace(value.back())) value.pop_back(); // Trim trailing whitespace from value
        while(!value.empty() && isspace(value.front())) value.erase(value.begin()); // Trim
        if(key.length() == 0 || value.length() == 0) {
            continue; // Invalid line
        }
        if(key.size() >= 2 && 
           (key[0] == '"' && key[key.size()-1] == '"') ||
           (key[0] == '\'' && key[key.size()-1] == '\'')) {
            key = key.substr(1, key.length()-2); // Remove quotes around key
        }
        if(value.size() >= 2 && 
           (value[0] == '"' && value[value.size()-1] == '"') ||
           (value[0] == '\'' && value[value.size()-1] == '\'')) {
            value = value.substr(1, value.length()-2); // Remove quotes around value
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
        printf("[%s]\n", section.first.c_str());
        for(const auto& kv : section.second) {
            printf("%s=%s\n", kv.first.c_str(), kv.second.c_str());
        }
    }
}

bool Configuration::readLine(File& file, std::string& line) {
    line = "";
    int val;
    while((val = file.read()) != -1) {
        if(val == '\n') break;
        line += char(val);
    }
    return line.length() > 0 || val != -1;
}

bool Configuration::hasSection(const std::string& section) {
    return sections_.find(section) != sections_.end();
}

bool Configuration::hasValue(const std::string& section, const std::string& key) {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return false;
    return secIt->second.find(key) != secIt->second.end();
}

std::string Configuration::valueForKey(const std::string& section, const std::string& key, const char* defaultValue) const {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return defaultValue;
    auto keyIt = secIt->second.find(key);
    if(keyIt == secIt->second.end()) return defaultValue;
    return keyIt->second;
}

float Configuration::valueForKey(const std::string& section, const std::string& key, float defaultValue) const {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return defaultValue;
    auto keyIt = secIt->second.find(key);
    if(keyIt == secIt->second.end()) return defaultValue;
    
    float var;
    if(sscanf(keyIt->second.c_str(), "%f", &var) != 1) {
        printf("Could not convert value '%s' for key '[%s]:%s to float. Returning default %f.", 
               keyIt->second.c_str(), section.c_str(), key.c_str(), defaultValue);
        return defaultValue;
    }
    return var;
}

int Configuration::valueForKey(const std::string& section, const std::string& key, int defaultValue) const {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return defaultValue;
    auto keyIt = secIt->second.find(key);
    if(keyIt == secIt->second.end()) return defaultValue;
    
    int var;
    if(sscanf(keyIt->second.c_str(), "%d", &var) != 1) {
        printf("Could not convert value '%s' for key '[%s]:%s to int. Returning default %d.", 
               keyIt->second.c_str(), section.c_str(), key.c_str(), defaultValue);
        return defaultValue;
    }
    return var;    
}

bool Configuration::valueForKey(const std::string& section, const std::string& key, bool defaultValue) const {
    auto secIt = sections_.find(section);
    if(secIt == sections_.end()) return defaultValue;
    auto keyIt = secIt->second.find(key);
    if(keyIt == secIt->second.end()) return defaultValue;
    
    std::string val = keyIt->second;
    std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c){return std::tolower(c);});
    if(val == "true" || val == "on" || val == "yes") return true;
    if(val == "false" || val == "off" || val == "no") return false;
    printf("Could not convert value '%s' for key '[%s]:%s to bool. Returning default %s.", 
            keyIt->second.c_str(), section.c_str(), key.c_str(), defaultValue ? "true" : "false");
    return defaultValue;
}