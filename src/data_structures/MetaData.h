#pragma once

#include "Scheme.h"
#include "data_structures/Column.h"
#include <vector>

struct MetaData {
    std::vector<std::streampos> chunk_metas;
    Scheme scheme;
    size_t column_numbers = 0;
    std::streampos meta_section_start = 0;

    std::vector<std::vector<std::streampos>> columns_starts; // TODO after stable work

    std::string GetColumnNameByIndex(size_t index) const;
    ColumnTypes GetColumnTypeByIndex(size_t index) const;
};