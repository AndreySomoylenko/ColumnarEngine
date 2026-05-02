#pragma once

#include <array>
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
    std::ifstream is_;
    char sep_;

    std::string cur_string_;

    std::string tmp_;


    static constexpr std::size_t kIOBufferSize = 1 << 20;
    std::array<char, kIOBufferSize> io_buffer_;
};