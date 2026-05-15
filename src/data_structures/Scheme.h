#pragma once

#include "Column.h"
#include <string>
#include <unordered_map>
#include <vector>

using Filename = std::string;

using Row = std::vector<std::string>;

class Scheme {
  private:
    std::vector<std::string> column_names;
    std::vector<ColumnTypes> column_types;
    std::unordered_map<std::string, size_t> name_to_index;
    std::unordered_map<std::string, ColumnTypes> name_to_type;

  public:
    void Add(const Row &str);
    void RemoveColumn(size_t index);
    const std::vector<std::string> &GetSchemeNames() const;
    const std::vector<ColumnTypes> &GetSchemeTypes() const;
    size_t GetColumnIndexByName(const std::string &name) const;
    ColumnTypes GetColumnTypeByName(const std::string &name) const;
    std::vector<Row> GiveRows() const;
};
