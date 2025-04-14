#ifndef _Drpcconfig_H
#define _Drpcconfig_H

#include <string>
#include <map>

class DrpcConfig {
public:
    DrpcConfig();
    ~DrpcConfig();
    bool LoadFromFile(const std::string& filename);
    std::string GetParameter(const std::string& key) const;

private:
    std::map<std::string, std::string> m_parameters;
};

#endif