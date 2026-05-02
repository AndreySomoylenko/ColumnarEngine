#pragma once

#include "Scheme.h"
#include <vector>

struct MetaData {
    std::vector<std::streampos> chunk_metas;
    Scheme scheme;
    size_t column_numbers = 0;
    std::streampos meta_section_start = 0;
};