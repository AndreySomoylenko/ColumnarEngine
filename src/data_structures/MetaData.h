#pragma once

#include "data_structures/Scheme.h"
#include <vector>

struct MetaData {
    Scheme scheme;
    size_t column_numbers = 0;
    size_t batch_numbers = 0;
    std::streampos meta_section_start = 0;

    std::vector<std::vector<std::streampos>>
        columns_starts;

    size_t GetColumnIndexByName(const std::string &name) const;
};
