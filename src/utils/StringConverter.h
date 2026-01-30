#pragma once

#include <fstream>
#include <string>
#include <vector>

class StringConverter {
public:
    std::string ReadNextString(std::ifstream &is, const std::vector<int> &separators);

    void WriteString(std::ofstream &out, const std::string &to_write);
    std::string TransformStringToFilestring(const std::string &cur);
};