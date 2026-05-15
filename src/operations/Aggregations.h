#pragma once

#include "data_structures/Column.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

using EnabledColumns = std::optional<std::unordered_set<size_t>>;

namespace agg {

namespace detail {

template <typename T>
concept StringValue = std::same_as<T, std::string>;

template <typename T>
auto ReadValue(const std::shared_ptr<Column> &column, size_t i) {
    auto value_view = column->Get(i);
    if constexpr (StringValue<T>) {
        return std::string_view(value_view.data, value_view.size);
    } else {
        T value;
        std::memcpy(&value, value_view.data, sizeof(T));

        return value;
    }
}

template <typename T, typename Compare>
T Extremum(const std::shared_ptr<Column> &column,
           const EnabledColumns &selected, const char *empty_error,
           Compare compare) {
    if (column->Size() == 0) {
        throw std::invalid_argument(empty_error);
    }

    if constexpr (StringValue<T>) {
        std::string result;
        bool has_value = false;

        const auto visit = [&](size_t i) {
            std::string_view value = ReadValue<T>(column, i);
            if (!has_value || compare(value, result)) {
                result.assign(value);
                has_value = true;
            }
        };

        if (selected.has_value()) {
            for (size_t i : selected.value()) {
                visit(i);
            }

            return result;
        }

        for (size_t i = 0; i < column->Size(); ++i) {
            visit(i);
        }

        return result;
    } else {
        T result = ReadValue<T>(
            column, selected.has_value() ? *selected.value().begin() : 0);

        if (selected.has_value()) {
            for (size_t i : selected.value()) {
                T value = ReadValue<T>(column, i);

                if (compare(value, result)) {
                    result = value;
                }
            }

            return result;
        }

        for (size_t i = 1; i < column->Size(); ++i) {
            T value = ReadValue<T>(column, i);

            if (compare(value, result)) {
                result = value;
            }
        }

        return result;
    }
}

} // namespace detail

template <typename ReadT, typename ResultT>
ResultT SumAs(const std::shared_ptr<Column> &column,
              const EnabledColumns &selected = std::nullopt) {
    ResultT sum = static_cast<ResultT>(0);

    if (selected.has_value()) {
        for (size_t i : selected.value()) {
            sum += static_cast<ResultT>(detail::ReadValue<ReadT>(column, i));
        }

        return sum;
    }

    for (size_t i = 0; i < column->Size(); ++i) {
        sum += static_cast<ResultT>(detail::ReadValue<ReadT>(column, i));
    }

    return sum;
}

template <typename T>
T Sum(const std::shared_ptr<Column> &column,
      const EnabledColumns &selected = std::nullopt) {
    if constexpr (std::same_as<T, __int128>) {
        return SumAs<int64_t, __int128>(column, selected);
    } else {
        return SumAs<T, T>(column, selected);
    }
}

template <typename T>
T Min(const std::shared_ptr<Column> &column,
      const EnabledColumns &selected = std::nullopt) {
    return detail::Extremum<T>(column, selected,
                               "Cannot calculate min of empty column",
                               std::less<>{});
}

template <typename T>
T Max(const std::shared_ptr<Column> &column,
      const EnabledColumns &selected = std::nullopt) {
    return detail::Extremum<T>(column, selected,
                               "Cannot calculate max of empty column",
                               std::greater<>{});
}

template <typename T>
double Avg(const std::shared_ptr<Column> &column,
           const EnabledColumns &selected = std::nullopt) {
    if (column->Size() == 0) {
        throw std::invalid_argument("Cannot calculate average of empty column");
    }
    const size_t count = selected.has_value() ? selected->size() : column->Size();
    if (count == 0) {
        throw std::invalid_argument("Cannot calculate average of empty selection");
    }

    T sum = agg::Sum<T>(column, selected);
    return static_cast<double>(sum) / static_cast<double>(count);
}

template <typename T, typename Hash, typename KeyEqual, typename Allocator>
void CountDistinct(const std::shared_ptr<Column> &column,
                   std::unordered_set<T, Hash, KeyEqual, Allocator> &answer,
                   const EnabledColumns &selected = std::nullopt) {

    if (selected.has_value()) {

        for (size_t i : selected.value()) {
            if constexpr (detail::StringValue<T>) {
                std::string_view value = detail::ReadValue<T>(column, i);
                answer.emplace(value);
            } else {
                answer.insert(detail::ReadValue<T>(column, i));
            }
        }

        return;
    }

    answer.reserve(column->Size());

    for (size_t i = 0; i < column->Size(); ++i) {
        if constexpr (detail::StringValue<T>) {
            std::string_view value = detail::ReadValue<T>(column, i);
            answer.emplace(value);
        } else {
            answer.insert(detail::ReadValue<T>(column, i));
        }
    }
}

inline uint64_t Count(const std::shared_ptr<Column> &column,
                      const EnabledColumns &selected = std::nullopt) {
    if (selected.has_value()) {
        return selected->size();
    }

    return column->Size();
}

} // namespace agg
