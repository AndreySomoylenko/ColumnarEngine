#pragma once

#include "data_structures/Butch.h"
#include "io/ColumnarReader.h"
#include <vector>

class Scanner {
  public:
    Scanner(const std::vector<std::size_t> &columns_to_read, ColumnarReader& reader);
    Butch ReadNext();
    bool IsEnd();
    void Reset();

  private:
    std::vector<size_t> columns_to_read_;
    size_t cur_index_ = 0;

    ColumnarReader& reader_;
};