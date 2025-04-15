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
    void Trim(std::string &read_buf) {
    // 去掉字符串前面的空格
    int index = read_buf.find_first_not_of(' ');
    if (index != -1) {  // 如果找到非空格字符
        read_buf = read_buf.substr(index, read_buf.size() - index);  // 截取字符串
    }

    // 去掉字符串后面的空格
    index = read_buf.find_last_not_of(' ');
    if (index != -1) {  // 如果找到非空格字符
        read_buf = read_buf.substr(0, index + 1);  // 截取字符串
    }
}

private:
    std::map<std::string, std::string> m_parameters;
};

#endif