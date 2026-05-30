#pragma once

#include "data_structures/Batch.h"
#include "data_structures/MetaData.h"
#include "data_structures/Scheme.h"

#include <fstream>
#include <vector>

class ColumnarReader {
  public:
    ColumnarReader() = default;
    explicit ColumnarReader(const std::string &columnar);
    ColumnarReader &operator=(ColumnarReader &&other) = default;
    std::vector<size_t> GetColumnIndices(const Scheme &scheme) const;
    Batch ReadNext(const Scheme &scheme,
                   const std::vector<size_t> &columns_to_read,
                   size_t &cur_index);
    bool IsEnd(size_t cur_batch) const;

    const Scheme &GetScheme() const;

    ~ColumnarReader();

  private:
    std::ifstream is_;

    MetaData data_;
};
