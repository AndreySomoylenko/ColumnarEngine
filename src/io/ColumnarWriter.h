#pragma once

#include "data_structures/Butch.h"
#include <array>
#include <fstream>
#include <vector>

class ColumnarWriter {
  public:
    ColumnarWriter(const std::string &filename);
    void WriteChunk(const Butch &butch);
    void Close(const Scheme &scheme);

  private:
    std::vector<std::streampos> chunk_starts;
    std::ofstream os_;
    static constexpr std::size_t kIOBufferSize = 1 << 20;
    std::array<char, kIOBufferSize> io_buffer_;
};
