#pragma once

#include "data_structures/Batch.h"
#include "data_structures/MetaData.h"
#include "data_structures/Scheme.h"

#include <fstream>

class ColumnarReader {
  public:
    ColumnarReader() = default;
    ColumnarReader(const std::string &columnar);
    ColumnarReader &operator=(ColumnarReader &&other) = default;
    Batch ReadNext(const Scheme &scheme, size_t &cur_index);
    bool IsEnd(size_t cur_batch) const;

    const Scheme &GetScheme() const;

    ~ColumnarReader();

  private:
    std::ifstream is_;

    MetaData data_;
};
