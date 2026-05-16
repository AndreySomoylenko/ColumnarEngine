#include "data_structures/Scheme.h"
#include "data_structures/Column.h"
#include <stdexcept>
#include <vector>

void Scheme::Add(const Row &str) {
    if (str.size() < 2) {
        throw std::invalid_argument("Incorrect scheme");
    }

    if (str[1] == "int16") {
        column_types.emplace_back(ColumnTypes::Int16);
    } else if (str[1] == "int32") {
        column_types.emplace_back(ColumnTypes::Int32);
    } else if (str[1] == "int64") {
        column_types.emplace_back(ColumnTypes::Int64);
    } else if (str[1] == "string") {
        column_types.emplace_back(ColumnTypes::String);
    } else if (str[1] == "timestamp") {
        column_types.emplace_back(ColumnTypes::Timestamp);
    } else if (str[1] == "date") {
        column_types.emplace_back(ColumnTypes::Date);
    } else if (str[1] == "double") {
        column_types.emplace_back(ColumnTypes::Double);
    } else if (str[1] == "int128") {
        column_types.emplace_back(ColumnTypes::Int128);
    } else {
        column_types.emplace_back(ColumnTypes::Unknown);
    }

    column_names.emplace_back(str[0]);
    name_to_index[str[0]] = column_names.size() - 1;
    name_to_type[str[0]] = column_types.back();
}

void Scheme::RemoveColumn(size_t index) {
    if (index >= column_names.size()) {
        throw std::out_of_range("Incorrect column index");
    }

    column_names.erase(column_names.begin() + index);
    column_types.erase(column_types.begin() + index);

    name_to_index.clear();
    name_to_type.clear();
    for (size_t i = 0; i < column_names.size(); ++i) {
        name_to_index[column_names[i]] = i;
        name_to_type[column_names[i]] = column_types[i];
    }
}

const std::vector<std::string> &Scheme::GetSchemeNames() const {
    return column_names;
}

const std::vector<ColumnTypes> &Scheme::GetSchemeTypes() const {
    return column_types;
}

std::vector<Row> Scheme::GiveRows() const {
    std::vector<Row> result;
    result.reserve(column_names.size());

    for (size_t i = 0; i < column_names.size(); ++i) {
        std::string type_str;

        if (column_types[i] == ColumnTypes::Int16) {
            type_str = "int16";
        } else if (column_types[i] == ColumnTypes::Int32) {
            type_str = "int32";
        } else if (column_types[i] == ColumnTypes::Int64) {
            type_str = "int64";
        } else if (column_types[i] == ColumnTypes::String) {
            type_str = "string";
        } else if (column_types[i] == ColumnTypes::Timestamp) {
            type_str = "timestamp";
        } else if (column_types[i] == ColumnTypes::Date) {
            type_str = "date";
        } else if (column_types[i] == ColumnTypes::Unknown) {
            type_str = "unknown";
        } else if (column_types[i] == ColumnTypes::Double) {
            type_str = "double";
        } else if (column_types[i] == ColumnTypes::Int128) {
            type_str = "int128";
        }

        result.push_back(Row{column_names[i], type_str});
    }

    return result;
}

size_t Scheme::GetColumnIndexByName(const std::string &name) const {
    if (!name_to_index.count(name)) {
        throw std::invalid_argument("Column not found");
    }
    return name_to_index.at(name);
}

ColumnTypes Scheme::GetColumnTypeByName(const std::string &name) const {
    if (!name_to_type.count(name)) {
        throw std::invalid_argument("Column not found");
    }
    return name_to_type.at(name);
}
