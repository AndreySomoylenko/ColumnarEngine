#pragma once

#include "data_structures/Batch.h"
#include <fstream>
#include <vector>

class ColumnarWriter {
  public:
    explicit ColumnarWriter(const std::string &filename);
    void WriteChunk(const Batch &batch);
    void Close(const Scheme &scheme) &&;

  private:
    std::ofstream os_;
    std::vector<std::vector<std::streampos>> column_starts_;
    size_t batch_count;
};
