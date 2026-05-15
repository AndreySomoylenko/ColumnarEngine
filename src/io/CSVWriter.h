#pragma once

#include <array>
#include <fstream>
#include <string>
#include <vector>

using Row = std::vector<std::string>;

class CSVWriter {
  public:
    CSVWriter(const std::string &filename, char sep = ',');

    void WriteRow(const Row &row);

    ~CSVWriter();

  private:
    std::ofstream os_;
    char sep_;
    static constexpr std::size_t kIOBufferSize = 1 << 20;
    std::array<char, kIOBufferSize> io_buffer_;
};