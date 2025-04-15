#include "DrpcConfig.h"
#include <memory>
#include <iostream>
// Implementation of DrpcConfig class methods

DrpcConfig::DrpcConfig() {
    // Constructor implementation
}

DrpcConfig::~DrpcConfig() {
    // Destructor implementation
}

bool DrpcConfig::LoadFromFile(const std::string& filePath) {
    std::unique_ptr<FILE, decltype(&fclose)> pf(
        fopen(filePath.c_str(), "r"),  // 打开配置文件
        &fclose  // 文件关闭函数
    );
    if (pf == nullptr) {
        exit(EXIT_FAILURE); 
        return false;
    }
    char buf[1024];  // 用于存储从文件中读取的每一行内容
    // 使用pf.get()方法获取原始指针，逐行读取文件内容
    while (fgets(buf, 1024, pf.get()) != nullptr) {
        std::string read_buf(buf);  // 将读取的内容转换为字符串
        Trim(read_buf);  // 去掉字符串前后的空格

        // 忽略注释行（以#开头）和空行
        if (read_buf[0] == '#' || read_buf.empty()) continue;

        // 查找键值对的分隔符'='
        int index = read_buf.find('=');
        if (index == -1) continue;  // 如果没有找到'='，跳过该行

        // 提取键（key）
        std::string key = read_buf.substr(0, index);
        Trim(key);  // 去掉key前后的空格

        // 查找行尾的换行符
        int endindex = read_buf.find('\n', index);
        // 提取值（value），并去掉换行符
        std::string value = read_buf.substr(index + 1, endindex - index - 1);
        Trim(value);  // 去掉value前后的空格

        // 将键值对存入配置map中
        m_parameters.insert({key, value});
    }
    // Load configuration from the specified file
    // Implementation goes here
    return true; // Return true if successful
}

// std::string DrpcConfig::GetParameter(const std::string& key) const {
//     // Retrieve the configuration parameter for the given key
//     // Implementation goes here
//     return ""; // Return the parameter value
// }