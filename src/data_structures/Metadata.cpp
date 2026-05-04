#include "MetaData.h"

std::string MetaData::GetColumnNameByIndex(size_t index) {
    if (index >= column_numbers) {
        throw std::out_of_range("Incorrect column index");
    }

    auto names = scheme.GetSchemeNames();
    return names[index];
}

ColumnTypes MetaData::GetColumnTypeByIndex(size_t index) {
    if (index >= column_numbers) {
        throw std::out_of_range("Incorrect column index");
    }

    return scheme.GetSchemeTypes()[index];
}