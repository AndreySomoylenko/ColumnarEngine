#pragma once

#include "data_structures/Column.h"
namespace agg {
template <typename T>
T Sum(const std::shared_ptr<Column> &column);

template <typename T>
T Min(const std::shared_ptr<Column> &column);

template <typename T>
T Max(const std::shared_ptr<Column> &column);

double Avg(const std::shared_ptr<Column> &column);

template <typename T>
uint64_t CountDistinct(const std::shared_ptr<Column> &column);

} // namespace agg

// command to clang-format all project files:
// find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i