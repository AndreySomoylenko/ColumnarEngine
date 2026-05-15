#include "Aggregations.h"

#include <chrono>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {

template <typename T>
T read_value(const std::shared_ptr<Column> &column, size_t i) {
    const char *ptr = std::get<const char *>(column->Get(i));

    T value;
    std::memcpy(&value, ptr, sizeof(T));

    return value;
}

} // namespace

template <> __int128 agg::Sum<__int128>(const std::shared_ptr<Column> &column) {
    __int128 sum = 0;

    for (size_t i = 0; i < column->Size(); ++i) {
        sum += static_cast<__int128>(read_value<int64_t>(column, i));
    }

    return sum;
}

template <> int64_t agg::Min<int64_t>(const std::shared_ptr<Column> &column) {
    if (column->Size() == 0) {
        throw std::invalid_argument("Cannot calculate min of empty column");
    }

    int64_t min_value = read_value<int64_t>(column, 0);

    for (size_t i = 1; i < column->Size(); ++i) {
        int64_t value = read_value<int64_t>(column, i);

        if (value < min_value) {
            min_value = value;
        }
    }

    return min_value;
}

template <>
std::chrono::system_clock::time_point
agg::Min<std::chrono::system_clock::time_point>(
    const std::shared_ptr<Column> &column) {
    if (column->Size() == 0) {
        throw std::invalid_argument("Cannot calculate min of empty column");
    }

    auto min_value =
        read_value<std::chrono::system_clock::time_point>(column, 0);

    for (size_t i = 1; i < column->Size(); ++i) {
        auto value =
            read_value<std::chrono::system_clock::time_point>(column, i);

        if (value < min_value) {
            min_value = value;
        }
    }

    return min_value;
}

template <>
std::string agg::Min<std::string>(const std::shared_ptr<Column> &column) {
    if (column->Size() == 0) {
        throw std::invalid_argument("Cannot calculate min of empty column");
    }

    std::string min_value;
    bool has_value = false;

    for (size_t i = 0; i < column->Size(); ++i) {
        auto [ptr, len] =
            std::get<std::pair<const char *, size_t>>(column->Get(i));
        std::string_view value(ptr, len);

        if (!has_value || value < min_value) {
            min_value.assign(value);
            has_value = true;
        }
    }

    return min_value;
}

template <> int64_t agg::Max<int64_t>(const std::shared_ptr<Column> &column) {
    if (column->Size() == 0) {
        throw std::invalid_argument("Cannot calculate max of empty column");
    }

    int64_t max_value = read_value<int64_t>(column, 0);

    for (size_t i = 1; i < column->Size(); ++i) {
        int64_t value = read_value<int64_t>(column, i);

        if (value > max_value) {
            max_value = value;
        }
    }

    return max_value;
}

template <>
std::chrono::system_clock::time_point
agg::Max<std::chrono::system_clock::time_point>(
    const std::shared_ptr<Column> &column) {
    if (column->Size() == 0) {
        throw std::invalid_argument("Cannot calculate max of empty column");
    }

    auto max_value =
        read_value<std::chrono::system_clock::time_point>(column, 0);

    for (size_t i = 1; i < column->Size(); ++i) {
        auto value =
            read_value<std::chrono::system_clock::time_point>(column, i);

        if (value > max_value) {
            max_value = value;
        }
    }

    return max_value;
}

template <>
std::string agg::Max<std::string>(const std::shared_ptr<Column> &column) {
    if (column->Size() == 0) {
        throw std::invalid_argument("Cannot calculate max of empty column");
    }

    std::string max_value;
    bool has_value = false;

    for (size_t i = 0; i < column->Size(); ++i) {
        auto [ptr, len] =
            std::get<std::pair<const char *, size_t>>(column->Get(i));
        std::string_view value(ptr, len);

        if (!has_value || value > max_value) {
            max_value.assign(value);
            has_value = true;
        }
    }

    return max_value;
}

double agg::Avg(const std::shared_ptr<Column> &column) {
    if (column->Size() == 0) {
        throw std::invalid_argument("Cannot calculate average of empty column");
    }

    __int128 sum = agg::Sum<__int128>(column);
    return static_cast<double>(sum) / static_cast<double>(column->Size());
}

template <>
uint64_t agg::CountDistinct<int64_t>(const std::shared_ptr<Column> &column) {
    std::unordered_set<int64_t> distinct_values;
    distinct_values.reserve(column->Size());

    for (size_t i = 0; i < column->Size(); ++i) {
        distinct_values.insert(read_value<int64_t>(column, i));
    }

    return distinct_values.size();
}

template <>
uint64_t
agg::CountDistinct<std::string>(const std::shared_ptr<Column> &column) {
    std::unordered_set<std::string> distinct_values;
    distinct_values.reserve(column->Size());

    for (size_t i = 0; i < column->Size(); ++i) {
        auto [ptr, len] =
            std::get<std::pair<const char *, size_t>>(column->Get(i));
        distinct_values.emplace(ptr, len);
    }

    return distinct_values.size();
}