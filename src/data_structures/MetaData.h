#pragma once

#include "Scheme.h"
#include <vector>

struct MetaData {
    std::vector<std::streampos> chunk_metas;
    Scheme scheme;
    size_t column_numbers = 0;
    std::streampos meta_section_start = 0;

    std::vector<std::vector<std::streampos>>
        columns_starts; // TODO after stable work

    size_t GetColumnIndexByName(const std::string &name) const;
};