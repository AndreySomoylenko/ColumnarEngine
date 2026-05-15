#pragma once

#include <array>
#include <fstream>
#include <string>
#include <vector>

using Raw = std::vector<std::string>;

class CSVWriter {
  public:
    CSVWriter(const std::string &filename, char sep = ',');

    void WriteRaw(const Raw &raw);

    ~CSVWriter();

  private:
    std::ofstream os_;
    char sep_;
    static constexpr std::size_t kIOBufferSize = 1 << 20;
    std::array<char, kIOBufferSize> io_buffer_;
};