#include "Configuration.h"
#include "FileManager.h"

Configuration::Configuration(const std::string& filename) {
    readFromFile(filename);
}

void Configuration::clear() {
    sections_ = std::map<std::string, Dictionary>();
}

bool Configuration::readFromFile(const std::string& filename) {
    FILE_CLASS file;
    
    if(file.open(filename.c_str(), O_RDONLY) == false) return false;

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
        std::string value = line.substr(eqPos+1);
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

bool Configuration::readLine(FILE_CLASS& file, std::string& line) {
    line = "";
    int val;
    while((val = file.read()) != -1) {
        if(val == '\n') break;
        line += char(val);
    }
    return line.length() > 0 || val != -1;
}