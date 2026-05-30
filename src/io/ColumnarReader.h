#pragma once

#include "data_structures/Batch.h"
#include "data_structures/MetaData.h"
#include "data_structures/Scheme.h"

#include <cstddef>
#include <string>
#include <vector>

class ColumnarReader {
  public:
    ColumnarReader() = default;
    explicit ColumnarReader(const std::string &columnar);
    ColumnarReader(const ColumnarReader &) = delete;
    ColumnarReader &operator=(const ColumnarReader &) = delete;
    ColumnarReader(ColumnarReader &&other) noexcept;
    ColumnarReader &operator=(ColumnarReader &&other) noexcept;
    std::vector<size_t> GetColumnIndices(const Scheme &scheme) const;
    Batch ReadNext(const Scheme &scheme,
                   const std::vector<size_t> &columns_to_read,
                   size_t &cur_index);
    bool IsEnd(size_t cur_batch) const;

    const Scheme &GetScheme() const;

    ~ColumnarReader();

  private:
    void Close();

    MetaData data_;
    int fd_ = -1;
    const char *mapped_data_ = nullptr;
    size_t mapped_size_ = 0;
};
