#pragma once

#include "data_structures/Butch.h"
#include "data_structures/MetaData.h"
#include "data_structures/Scheme.h"

#include <fstream>

class ColumnarReader {
public:
    ColumnarReader() = default;
    ColumnarReader(const std::string &columnar);
    ColumnarReader &operator=(ColumnarReader &&other) = default;
    Butch ReadNext();
    void Reset();
    bool IsEnd();

    Scheme GetScheme();

    ~ColumnarReader();

private:
    std::ifstream is_;
    size_t cur_butch_;

    std::string Unpack(const std::string &from_file, const size_t index);

    MetaData data_;
};