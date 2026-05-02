#pragma once

#include "data_structures/Butch.h"
#include "data_structures/MetaData.h"
#include "data_structures/Scheme.h"

#include <fstream>
#include <array>

class ColumnarReader {
public:
    ColumnarReader() = default;
    ColumnarReader(const std::string &columnar);
    ColumnarReader &operator=(ColumnarReader &&other) = default;
    Butch ReadNext();
    void Reset();
    bool IsEnd();

    const Scheme &GetScheme() const;

    ~ColumnarReader();

private:
    std::ifstream is_;
    size_t cur_butch_;

    MetaData data_;
    static constexpr std::size_t kIOBufferSize = 1 << 20;
    std::array<char, kIOBufferSize> io_buffer_;
};