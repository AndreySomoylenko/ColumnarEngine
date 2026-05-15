#pragma once

#include "data_structures/Butch.h"
#include "data_structures/MetaData.h"
#include "data_structures/Scheme.h"

#include <array>
#include <fstream>

class ColumnarReader {
  public:
    ColumnarReader() = default;
    ColumnarReader(const std::string &columnar);
    ColumnarReader &operator=(ColumnarReader &&other) = default;
    Butch ReadNext(const Scheme &scheme, size_t &cur_index);
    bool IsEnd(size_t cur_butch) const;

    const Scheme &GetScheme() const;

    ~ColumnarReader();

  private:
    std::ifstream is_;

    MetaData data_;
    static constexpr std::size_t kIOBufferSize = 1 << 20;
    std::array<char, kIOBufferSize> io_buffer_;
};