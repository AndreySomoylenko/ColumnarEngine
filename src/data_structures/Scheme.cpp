#include "Scheme.h"
#include "data_structures/Column.h"
#include <stdexcept>
#include <vector>

void Scheme::Add(const Raw &str) {
    if (str.size() < 2) {
        throw std::invalid_argument("Incorrect scheme");
    }

    if (str[1] == "int64") {
        column_types.emplace_back(ColumnTypes::Int64);
    } else if (str[1] == "string") {
        column_types.emplace_back(ColumnTypes::String);
    } else {
        column_types.emplace_back(ColumnTypes::Unknown);
    }

    column_names.emplace_back(str[0]);
}

const std::vector<std::string> &Scheme::GetSchemeNames() const { return column_names; }

const std::vector<ColumnTypes> &Scheme::GetSchemeTypes() const { return column_types; }

std::vector<Raw> Scheme::GiveRaws() const {
    std::vector<Raw> result;
    result.reserve(column_names.size());

    for (size_t i = 0; i < column_names.size(); ++i) {
        std::string type_str;

        if (column_types[i] == ColumnTypes::Int64) {
            type_str = "int64";
        } else if (column_types[i] == ColumnTypes::String) {
            type_str = "string";
        } else if (column_types[i] == ColumnTypes::Timestamp) {
            type_str = "timestamp";
        }
        else if (column_types[i] == ColumnTypes::Unknown) {
            type_str = "unknown";
        }

        result.push_back(Raw{column_names[i], type_str});
    }

    return result;
}
