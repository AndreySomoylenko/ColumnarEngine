#pragma once

#include <fstream>
#include <string>

class StringConverter {
public:
    void WriteString(std::ofstream &out, const std::string &to_write);
    std::string TransformStringToFilestring(const std::string &cur);
};