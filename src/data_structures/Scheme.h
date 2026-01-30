#pragma once

#include "Column.h"
#include <string>
#include <vector>

using Filename = std::string;

using Raw = std::vector<std::string>;

class Scheme {
private:
    std::vector<std::string> column_names;
    std::vector<ColumnTypes> column_types;

public:
    void Add(const Raw &str);
    const std::vector<std::string> GetSchemeNames() const;
    const std::vector<ColumnTypes> GetSchemeTypes() const;
    std::vector<Raw> GiveRaws();
};