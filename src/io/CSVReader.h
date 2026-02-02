#pragma once

#include "utils/StringConverter.h"
#include <fstream>
#include <string>
#include <vector>

using Raw = std::vector<std::string>;

class CSVReader {
public:
    CSVReader(const std::string &filename, char sep = ',');

    void ReadNext(Raw &raw);
    bool IsEnd();

    ~CSVReader();

private:
    StringConverter converter_;
    std::ifstream is_;
    char sep_;

    std::string cur_string_;

    std::vector<char> buffer_;

    std::string tmp_;
};