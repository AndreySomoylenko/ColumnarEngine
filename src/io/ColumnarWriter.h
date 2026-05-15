#pragma once

#include "data_structures/Batch.h"
#include <fstream>
#include <vector>

class ColumnarWriter {
  public:
    ColumnarWriter(const std::string &filename);
    void WriteChunk(const Batch &batch);
    void Close(const Scheme &scheme);

  private:
    std::vector<std::streampos> chunk_starts;
    std::ofstream os_;
};
