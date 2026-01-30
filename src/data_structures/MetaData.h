#pragma once

#include "Scheme.h"
#include <vector>

struct MetaData {
    std::vector<uint64_t> chunk_metas;
    Scheme scheme;
    size_t column_numbers = 0;
};