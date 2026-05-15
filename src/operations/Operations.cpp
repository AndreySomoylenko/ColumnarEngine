#include "Operations.h"
#include "Aggregations.h"
#include "data_structures/Batch.h"
#include "data_structures/Column.h"
#include "data_structures/Scheme.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

using EnabledColumns = std::optional<std::unordered_set<size_t>>;

namespace {

bool CheckFilterCondition(const Batch &batch, const FilterTask &task,
                          size_t row_index) {
    const auto &column = batch.GetColumn(task.column_index);
    const auto value = column->Get(row_index);

    return task.condition(value.data, value.size);
}

bool IsStringType(ColumnTypes type) {
    switch (type) {
    case ColumnTypes::String:
    case ColumnTypes::Unknown:
        return true;
    default:
        return false;
    }
}

bool IsIntType(ColumnTypes type) {
    switch (type) {
    case ColumnTypes::Int16:
    case ColumnTypes::Int32:
    case ColumnTypes::Int64:
    case ColumnTypes::Int128:
        return true;
    default:
        return false;
    }
}

size_t FixedTypeSize(ColumnTypes type) {
    switch (type) {
    case ColumnTypes::Int16:
        return sizeof(int16_t);
    case ColumnTypes::Int32:
        return sizeof(int32_t);
    case ColumnTypes::Int64:
        return sizeof(int64_t);
    case ColumnTypes::Int128:
        return sizeof(__int128);
    case ColumnTypes::Double:
        return sizeof(double);
    case ColumnTypes::Timestamp:
    case ColumnTypes::Date:
        return sizeof(std::chrono::system_clock::time_point);
    default:
        throw std::invalid_argument("Column type doesn't have fixed size");
    }
}

template <typename T>
T ReadColumnValue(const std::shared_ptr<Column> &column, size_t row_index) {
    auto value = column->Get(row_index);

    T result;
    std::memcpy(&result, value.data, sizeof(T));
    return result;
}

std::string ReadStringValue(const std::shared_ptr<Column> &column,
                            size_t row_index) {
    auto value = column->Get(row_index);
    return std::string(value.data, value.size);
}

int64_t ReadIntegerFilterValue(const char *data, size_t size) {
    if (size == sizeof(int16_t)) {
        int16_t value;
        std::memcpy(&value, data, sizeof(value));
        return value;
    }
    if (size == sizeof(int32_t)) {
        int32_t value;
        std::memcpy(&value, data, sizeof(value));
        return value;
    }
    if (size == sizeof(int64_t)) {
        int64_t value;
        std::memcpy(&value, data, sizeof(value));
        return value;
    }

    throw std::invalid_argument("Expected integer value");
}

int64_t ReadIntegerColumnValue(const std::shared_ptr<Column> &column,
                               size_t row_index) {
    auto value = column->Get(row_index);
    return ReadIntegerFilterValue(value.data, value.size);
}

int ParseFilterTwoDigits(const std::string &value, size_t pos) {
    if (pos + 1 >= value.size() || !std::isdigit(value[pos]) ||
        !std::isdigit(value[pos + 1])) {
        throw std::invalid_argument("Incorrect timestamp");
    }
    return (value[pos] - '0') * 10 + (value[pos + 1] - '0');
}

int ParseFilterFourDigits(const std::string &value, size_t pos) {
    if (pos + 3 >= value.size() || !std::isdigit(value[pos]) ||
        !std::isdigit(value[pos + 1]) || !std::isdigit(value[pos + 2]) ||
        !std::isdigit(value[pos + 3])) {
        throw std::invalid_argument("Incorrect timestamp");
    }
    return (value[pos] - '0') * 1000 + (value[pos + 1] - '0') * 100 +
           (value[pos + 2] - '0') * 10 + (value[pos + 3] - '0');
}

std::chrono::system_clock::time_point
ParseFilterTimePoint(const std::string &value, bool is_date) {
    if (is_date) {
        if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
            throw std::invalid_argument("Incorrect date");
        }

        using namespace std::chrono;
        const year_month_day ymd{
            year{ParseFilterFourDigits(value, 0)},
            month{static_cast<unsigned>(ParseFilterTwoDigits(value, 5))},
            day{static_cast<unsigned>(ParseFilterTwoDigits(value, 8))}};
        if (!ymd.ok()) {
            throw std::invalid_argument("Incorrect date");
        }
        return time_point_cast<system_clock::duration>(sys_days{ymd});
    }

    if (value.size() != 19 || value[4] != '-' || value[7] != '-' ||
        (value[10] != ' ' && value[10] != 'T') || value[13] != ':' ||
        value[16] != ':') {
        throw std::invalid_argument("Incorrect timestamp");
    }

    const int year_value = ParseFilterFourDigits(value, 0);
    const int month_value = ParseFilterTwoDigits(value, 5);
    const int day_value = ParseFilterTwoDigits(value, 8);
    const int hour_value = ParseFilterTwoDigits(value, 11);
    const int minute_value = ParseFilterTwoDigits(value, 14);
    const int second_value = ParseFilterTwoDigits(value, 17);

    if (hour_value > 23 || minute_value > 59 || second_value > 59) {
        throw std::invalid_argument("Incorrect timestamp");
    }

    using namespace std::chrono;
    const year_month_day ymd{year{year_value},
                             month{static_cast<unsigned>(month_value)},
                             day{static_cast<unsigned>(day_value)}};
    if (!ymd.ok()) {
        throw std::invalid_argument("Incorrect timestamp");
    }

    const sys_seconds timestamp = sys_days{ymd} + hours{hour_value} +
                                  minutes{minute_value} + seconds{second_value};
    return time_point_cast<system_clock::duration>(timestamp);
}

bool IsRegexSpecial(char value) {
    switch (value) {
    case '.':
    case '^':
    case '$':
    case '|':
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case '*':
    case '+':
    case '?':
    case '\\':
        return true;
    default:
        return false;
    }
}

std::string SqlLikePatternToRegex(std::string_view pattern) {
    std::string result = "^";
    for (char value : pattern) {
        if (value == '%') {
            result += ".*";
        } else if (value == '_') {
            result += '.';
        } else {
            if (IsRegexSpecial(value)) {
                result += '\\';
            }
            result += value;
        }
    }
    result += '$';
    return result;
}

std::vector<std::string> SplitLikePatternByPercent(std::string_view pattern) {
    std::vector<std::string> parts;
    size_t start = 0;

    while (start <= pattern.size()) {
        size_t percent = pattern.find('%', start);
        size_t end =
            percent == std::string_view::npos ? pattern.size() : percent;

        if (end > start) {
            parts.emplace_back(pattern.substr(start, end - start));
        }

        if (percent == std::string_view::npos) {
            break;
        }

        start = percent + 1;
    }

    return parts;
}

bool ContainsLikeUnderscore(std::string_view pattern) {
    return pattern.find('_') != std::string_view::npos;
}

bool ContainsLikePercent(std::string_view pattern) {
    return pattern.find('%') != std::string_view::npos;
}

std::function<bool(std::string_view)> MakeSqlLikeMatcher(std::string pattern) {
    const bool has_percent = ContainsLikePercent(pattern);
    const bool has_underscore = ContainsLikeUnderscore(pattern);

    if (!has_percent && !has_underscore) {
        return [pattern = std::move(pattern)](std::string_view value) {
            return value == pattern;
        };
    }

    if (has_underscore) {
        return [regex = std::regex(SqlLikePatternToRegex(pattern))](
                   std::string_view value) {
            return std::regex_match(value.begin(), value.end(), regex);
        };
    }

    const bool anchored_start = !pattern.empty() && pattern.front() != '%';
    const bool anchored_end = !pattern.empty() && pattern.back() != '%';

    auto parts = SplitLikePatternByPercent(pattern);

    if (parts.empty()) {
        return [](std::string_view) { return true; };
    }

    return [parts = std::move(parts), anchored_start,
            anchored_end](std::string_view value) {
        size_t pos = 0;
        size_t begin_part = 0;
        size_t end_part = parts.size();

        if (anchored_start) {
            const auto &first = parts.front();
            if (!value.starts_with(first)) {
                return false;
            }
            pos = first.size();
            begin_part = 1;
        }

        if (anchored_end) {
            --end_part;
        }

        for (size_t i = begin_part; i < end_part; ++i) {
            size_t found = value.find(parts[i], pos);
            if (found == std::string_view::npos) {
                return false;
            }
            pos = found + parts[i].size();
        }

        if (anchored_end) {
            const auto &last = parts.back();
            if (!value.ends_with(last)) {
                return false;
            }

            size_t last_pos = value.size() - last.size();
            if (last_pos < pos) {
                return false;
            }
        }

        return true;
    };
}

std::string AggTypeName(AggType type) {
    switch (type) {
    case AggType::Sum:
        return "sum";
    case AggType::Min:
        return "min";
    case AggType::Max:
        return "max";
    case AggType::CountDistinct:
        return "count_distinct";
    case AggType::Avg:
        return "avg";
    case AggType::Count:
        return "count";
    default:
        throw std::invalid_argument("Unknown aggregation type");
    }
}

ColumnTypes GroupByResultType(AggType agg_type, ColumnTypes column_type) {
    switch (agg_type) {
    case AggType::Count:
    case AggType::CountDistinct:
        return ColumnTypes::Int64;
    case AggType::Avg:
        return ColumnTypes::Int128;
    case AggType::Sum:
        return column_type == ColumnTypes::Double ? ColumnTypes::Double
                                                  : ColumnTypes::Int128;
    case AggType::Min:
    case AggType::Max:
        return column_type;
    default:
        throw std::invalid_argument("Unsupported aggregation type");
    }
}

std::shared_ptr<Column> MakeExpressionColumn(ColumnTypes type) {
    switch (type) {
    case ColumnTypes::Int16:
        return std::make_shared<Int16Column>();
    case ColumnTypes::Int32:
        return std::make_shared<Int32Column>();
    case ColumnTypes::Int64:
        return std::make_shared<Int64Column>();
    case ColumnTypes::Int128:
        return std::make_shared<Int128Column>();
    case ColumnTypes::Double:
        return std::make_shared<DoubleColumn>();
    case ColumnTypes::Timestamp:
        return std::make_shared<TimeColumn>();
    case ColumnTypes::Date:
        return std::make_shared<TimeColumn>(true);
    case ColumnTypes::String:
    case ColumnTypes::Unknown:
        return std::make_shared<StringColumn>();
    default:
        throw std::invalid_argument("Unsupported expression column type");
    }
}

std::chrono::system_clock::time_point
TruncateToMinute(std::chrono::system_clock::time_point value) {
    using namespace std::chrono;
    return time_point_cast<system_clock::duration>(floor<minutes>(value));
}

int64_t ExtractMinuteValue(std::chrono::system_clock::time_point value) {
    using namespace std::chrono;
    const auto timestamp = floor<seconds>(value);
    const auto day = floor<days>(timestamp);
    const hh_mm_ss time{timestamp - day};
    return static_cast<int64_t>(time.minutes().count());
}

std::string ExtractDomain(std::string_view url) {
    const std::string_view http = "http://";
    const std::string_view https = "https://";
    if (url.starts_with(http)) {
        url.remove_prefix(http.size());
    } else if (url.starts_with(https)) {
        url.remove_prefix(https.size());
    }

    const std::string_view www = "www.";
    if (url.starts_with(www)) {
        url.remove_prefix(www.size());
    }

    const size_t slash = url.find('/');
    if (slash != std::string_view::npos) {
        url = url.substr(0, slash);
    }

    return std::string(url);
}

} // namespace

template <typename T>
void UpdateMax(ResultAggVariant &result, const std::shared_ptr<Column> &column,
               const EnabledColumns &enabled) {
    T value = agg::Max<T>(column, enabled);

    if (auto *current = std::get_if<T>(&result)) {
        *current = std::max(*current, value);
    } else {
        result = std::move(value);
    }
}

template <typename T>
void UpdateMin(ResultAggVariant &result, const std::shared_ptr<Column> &column,
               const EnabledColumns &enabled) {
    T value = agg::Min<T>(column, enabled);

    if (auto *current = std::get_if<T>(&result)) {
        *current = std::min(*current, value);
    } else {
        result = std::move(value);
    }
}

OperationType StreamingOperation::GetType() const {
    return OperationType::NonBlocking;
}
OperationType BlockingOperation::GetType() const {
    return OperationType::Blocking;
}

Aggregation::Aggregation(std::vector<AggTask> &&tasks)
    : tasks_(std::move(tasks)), results_(tasks_.size()),
      column_active_size_(tasks_.size()) {}

AggTask MakeAggTask(size_t column_index, AggType agg_type,
                    ColumnTypes return_type, std::string alias) {
    return AggTask{column_index, agg_type, return_type, std::move(alias)};
}

AggTask MakeSumAgg(size_t column_index, std::string alias,
                   ColumnTypes return_type) {
    return MakeAggTask(column_index, AggType::Sum, return_type,
                       std::move(alias));
}

AggTask MakeMinAgg(size_t column_index, ColumnTypes return_type,
                   std::string alias) {
    return MakeAggTask(column_index, AggType::Min, return_type,
                       std::move(alias));
}

AggTask MakeMaxAgg(size_t column_index, ColumnTypes return_type,
                   std::string alias) {
    return MakeAggTask(column_index, AggType::Max, return_type,
                       std::move(alias));
}

AggTask MakeCountAgg(size_t column_index, std::string alias) {
    return MakeAggTask(column_index, AggType::Count, ColumnTypes::Int64,
                       std::move(alias));
}

AggTask MakeCountDistinctAgg(size_t column_index, std::string alias) {
    return MakeAggTask(column_index, AggType::CountDistinct, ColumnTypes::Int64,
                       std::move(alias));
}

AggTask MakeAvgAgg(size_t column_index, std::string alias) {
    return MakeAggTask(column_index, AggType::Avg, ColumnTypes::Int128,
                       std::move(alias));
}

Aggregation MakeAggregation(std::vector<AggTask> &&tasks) {
    return Aggregation(std::move(tasks));
}

void ProcessExtremeAgg(const std::shared_ptr<Column> &column,
                       const EnabledColumns &enabled, ColumnTypes column_type,
                       std::vector<ResultAggVariant> &results_, size_t i,
                       AggType agg_type) {
    switch (agg_type) {
    case AggType::Min:
        switch (column_type) {
        case ColumnTypes::Int16:
            return UpdateMin<int16_t>(results_[i], column, enabled);
        case ColumnTypes::Int32:
            return UpdateMin<int32_t>(results_[i], column, enabled);
        case ColumnTypes::Int64:
            return UpdateMin<int64_t>(results_[i], column, enabled);
        case ColumnTypes::String:
            return UpdateMin<std::string>(results_[i], column, enabled);
        case ColumnTypes::Timestamp:
        case ColumnTypes::Date:
            return UpdateMin<std::chrono::system_clock::time_point>(
                results_[i], column, enabled);
        case ColumnTypes::Int128:
            return UpdateMin<__int128>(results_[i], column, enabled);
        case ColumnTypes::Double:
            return UpdateMin<double>(results_[i], column, enabled);
        default:
            throw std::invalid_argument("Unsupported column type for min");
        }
    case AggType::Max:
        switch (column_type) {
        case ColumnTypes::Int16:
            return UpdateMax<int16_t>(results_[i], column, enabled);
        case ColumnTypes::Int32:
            return UpdateMax<int32_t>(results_[i], column, enabled);
        case ColumnTypes::Int64:
            return UpdateMax<int64_t>(results_[i], column, enabled);
        case ColumnTypes::String:
            return UpdateMax<std::string>(results_[i], column, enabled);
        case ColumnTypes::Timestamp:
        case ColumnTypes::Date:
            return UpdateMax<std::chrono::system_clock::time_point>(
                results_[i], column, enabled);
        case ColumnTypes::Int128:
            return UpdateMax<__int128>(results_[i], column, enabled);
        case ColumnTypes::Double:
            return UpdateMax<double>(results_[i], column, enabled);
        default:
            throw std::invalid_argument("Unsupported column type for max");
        }
    default:
        throw std::invalid_argument("Unsupported aggregation type");
    }
}

template <typename T>
void UpdateSum(ResultAggVariant &result, const std::shared_ptr<Column> &column,
               const EnabledColumns &enabled) {
    if (auto *current = std::get_if<T>(&result)) {
        *current += agg::Sum<T>(column, enabled);
    } else {
        result = agg::Sum<T>(column, enabled);
    }
}

template <typename ReadT, typename ResultT>
void UpdateSumAs(ResultAggVariant &result,
                 const std::shared_ptr<Column> &column,
                 const EnabledColumns &enabled) {
    if (auto *current = std::get_if<ResultT>(&result)) {
        *current += agg::SumAs<ReadT, ResultT>(column, enabled);
    } else {
        result = agg::SumAs<ReadT, ResultT>(column, enabled);
    }
}

void ProcessSumAgg(const std::shared_ptr<Column> &column,
                   const EnabledColumns &enabled, ColumnTypes column_type,
                   std::vector<ResultAggVariant> &results_, size_t i) {
    switch (column_type) {
    case ColumnTypes::Int16:
        return UpdateSumAs<int16_t, __int128>(results_[i], column, enabled);
    case ColumnTypes::Int32:
        return UpdateSumAs<int32_t, __int128>(results_[i], column, enabled);
    case ColumnTypes::Int64:
        return UpdateSumAs<int64_t, __int128>(results_[i], column, enabled);
    case ColumnTypes::Int128:
        return UpdateSumAs<__int128, __int128>(results_[i], column, enabled);
    case ColumnTypes::Double:
        return UpdateSum<double>(results_[i], column, enabled);
    default:
        throw std::invalid_argument("Unsupported column type for sum");
    }
}

template <typename T>
void UpdateAgg(ResultAggVariant &result, const std::shared_ptr<Column> &column,
               const EnabledColumns &enabled) {
    if (auto *current = std::get_if<std::pair<T, size_t>>(&result)) {
        (*current).first += agg::Sum<T>(column, enabled);
        (*current).second +=
            (enabled.has_value() ? enabled->size() : column->Size());
    } else {
        result = std::make_pair(
            agg::Sum<T>(column, enabled),
            (enabled.has_value() ? enabled->size() : column->Size()));
    }
}

template <typename ReadT, typename ResultT>
void UpdateAggAs(ResultAggVariant &result,
                 const std::shared_ptr<Column> &column,
                 const EnabledColumns &enabled) {
    if (auto *current = std::get_if<std::pair<ResultT, size_t>>(&result)) {
        (*current).first += agg::SumAs<ReadT, ResultT>(column, enabled);
        (*current).second +=
            (enabled.has_value() ? enabled->size() : column->Size());
    } else {
        result = std::make_pair(
            agg::SumAs<ReadT, ResultT>(column, enabled),
            (enabled.has_value() ? enabled->size() : column->Size()));
    }
}

void ProcessAvgAgg(const std::shared_ptr<Column> &column,
                   const EnabledColumns &enabled, ColumnTypes column_type,
                   std::vector<ResultAggVariant> &results_, size_t i) {
    switch (column_type) {
    case ColumnTypes::Int16:
        return UpdateAggAs<int16_t, __int128>(results_[i], column, enabled);
    case ColumnTypes::Int32:
        return UpdateAggAs<int32_t, __int128>(results_[i], column, enabled);
    case ColumnTypes::Int64:
        return UpdateAggAs<int64_t, __int128>(results_[i], column, enabled);
    case ColumnTypes::Int128:
        return UpdateAggAs<__int128, __int128>(results_[i], column, enabled);
    case ColumnTypes::Double:
        return UpdateAgg<double>(results_[i], column, enabled);
    default:
        throw std::invalid_argument("Unsupported column type for average");
    }
}

template <typename T, typename SetT = std::unordered_set<T>>
void UpdateDistinct(ResultAggVariant &result,
                    const std::shared_ptr<Column> &column,
                    const EnabledColumns &enabled) {

    if (!std::holds_alternative<SetT>(result)) {
        result = SetT{};
    }
    auto &current = std::get<SetT>(result);

    agg::CountDistinct(column, current, enabled);
}

void ProcessCountDistinctAgg(const std::shared_ptr<Column> &column,
                             const EnabledColumns &enabled,
                             ColumnTypes column_type,
                             std::vector<ResultAggVariant> &results_,
                             size_t i) {
    switch (column_type) {
    case ColumnTypes::Int64:
        return UpdateDistinct<int64_t>(results_[i], column, enabled);
    case ColumnTypes::Int16:
        return UpdateDistinct<int16_t>(results_[i], column, enabled);
    case ColumnTypes::Int32:
        return UpdateDistinct<int32_t>(results_[i], column, enabled);
    case ColumnTypes::Double:
        return UpdateDistinct<double>(results_[i], column, enabled);
    case ColumnTypes::Int128:
        return UpdateDistinct<__int128>(results_[i], column, enabled);
    case ColumnTypes::Date:
    case ColumnTypes::Timestamp:
        return UpdateDistinct<
            std::chrono::system_clock::time_point,
            std::unordered_set<std::chrono::system_clock::time_point,
                               TimePointHash>>(results_[i], column, enabled);
    case ColumnTypes::String:
        return UpdateDistinct<std::string>(results_[i], column, enabled);
    default:
        throw std::invalid_argument("Unknown column type");
    }
}

void Aggregation::Process(const Batch &batch) {
    for (size_t i = 0; i < tasks_.size(); ++i) {
        const auto &task = tasks_[i];
        ColumnTypes column_type =
            batch.GetColumn(task.column_index)->GetColumnType();
        auto column = batch.GetColumn(task.column_index);
        auto enabled = batch.GetEnabledColumns();
        column_active_size_[i] +=
            (enabled.has_value() ? enabled->size() : column->Size());
        switch (task.agg_type) {
        case AggType::Max:
            ProcessExtremeAgg(column, enabled, column_type, results_, i,
                              AggType::Max);
            break;
        case AggType::Min:
            ProcessExtremeAgg(column, enabled, column_type, results_, i,
                              AggType::Min);
            break;
        case AggType::Sum:
            ProcessSumAgg(column, enabled, column_type, results_, i);
            break;
        case AggType::Avg:
            ProcessAvgAgg(column, enabled, column_type, results_, i);
            break;
        case AggType::CountDistinct:
            ProcessCountDistinctAgg(column, enabled, column_type, results_, i);
            break;
        case AggType::Count:
            if (auto *result = std::get_if<uint64_t>(&results_[i])) {
                *result += agg::Count(column, enabled);
            } else {
                results_[i] = agg::Count(column, enabled);
            }
            break;
        default:
            throw std::invalid_argument("Unsupported aggregation type");
        }
    }
}

size_t CountDistinctResultSize(const ResultAggVariant &result) {
    if (auto *value = std::get_if<std::unordered_set<int16_t>>(&result)) {
        return value->size();
    } else if (auto *value =
                   std::get_if<std::unordered_set<int32_t>>(&result)) {
        return value->size();
    } else if (auto *value =
                   std::get_if<std::unordered_set<int64_t>>(&result)) {
        return value->size();
    } else if (auto *value =
                   std::get_if<std::unordered_set<__int128>>(&result)) {
        return value->size();
    } else if (auto *value = std::get_if<std::unordered_set<double>>(&result)) {
        return value->size();
    } else if (auto *value = std::get_if<std::unordered_set<
                   std::chrono::system_clock::time_point, TimePointHash>>(
                   &result)) {
        return value->size();
    } else if (auto *value =
                   std::get_if<std::unordered_set<std::string>>(&result)) {
        return value->size();
    }

    throw std::invalid_argument("Unexpected count distinct state");
}

std::vector<Batch> Aggregation::Finalize() && {
    Scheme result_scheme;
    for (auto &task : tasks_) {
        auto column_type = task.agg_type == AggType::Avg ? ColumnTypes::Int128
                                                         : task.return_type;
        result_scheme.Add({task.alias, GetNameByType(column_type)});
    }

    Batch result(result_scheme, false);

    for (int i = 0; i < tasks_.size(); ++i) {
        switch (tasks_[i].agg_type) {
        case AggType::CountDistinct: {
            int64_t current =
                static_cast<int64_t>(CountDistinctResultSize(results_[i]));
            result.AddToColumn(reinterpret_cast<char *>(&current),
                               sizeof(current), i);
            break;
        }
        case AggType::Avg:
            if (auto *current =
                    std::get_if<std::pair<__int128, size_t>>(&results_[i])) {
                auto &[sum, count] = *current;
                if (count == 0) {
                    throw std::invalid_argument(
                        "Cannot calculate average of empty selection");
                }
                __int128 avg = sum / static_cast<__int128>(count);
                result.AddToColumn(reinterpret_cast<char *>(&avg), sizeof(avg),
                                   i);
            } else if (auto *current = std::get_if<std::pair<double, size_t>>(
                           &results_[i])) {
                auto &[sum, count] = *current;
                if (count == 0) {
                    throw std::invalid_argument(
                        "Cannot calculate average of empty selection");
                }
                __int128 avg = static_cast<__int128>(sum / count);
                result.AddToColumn(reinterpret_cast<char *>(&avg), sizeof(avg),
                                   i);
            } else {
                throw std::invalid_argument(
                    "Unsupported column type for average");
            }
            break;
        default:
            if (auto *value = std::get_if<int16_t>(&results_[i])) {
                result.AddToColumn(reinterpret_cast<const char *>(value),
                                   sizeof(int16_t), i);
            } else if (auto *value = std::get_if<int32_t>(&results_[i])) {
                result.AddToColumn(reinterpret_cast<const char *>(value),
                                   sizeof(int32_t), i);
            } else if (auto *value = std::get_if<int64_t>(&results_[i])) {
                result.AddToColumn(reinterpret_cast<const char *>(value),
                                   sizeof(int64_t), i);
            } else if (auto *value = std::get_if<__int128>(&results_[i])) {
                result.AddToColumn(reinterpret_cast<const char *>(value),
                                   sizeof(__int128), i);
            } else if (auto *value = std::get_if<double>(&results_[i])) {
                result.AddToColumn(reinterpret_cast<const char *>(value),
                                   sizeof(double), i);
            } else if (auto *value = std::get_if<std::string>(&results_[i])) {
                result.AddToColumn(value->data(), value->size(), i);
            } else if (auto *value =
                           std::get_if<std::chrono::system_clock::time_point>(
                               &results_[i])) {
                result.AddToColumn(
                    reinterpret_cast<const char *>(value),
                    sizeof(std::chrono::system_clock::time_point), i);
            } else if (auto *value = std::get_if<uint64_t>(&results_[i])) {
                result.AddToColumn(reinterpret_cast<const char *>(value),
                                   sizeof(uint64_t), i);
            }
            break;
        }
    }

    return {std::move(result)};
};

Filter::Filter(std::vector<FilterTask> &&conditions)
    : conditions_(std::move(conditions)) {}

FilterTask MakeRawFilter(size_t column_index,
                         std::function<bool(const char *, size_t)> condition) {
    return FilterTask{column_index, std::move(condition)};
}

FilterTask MakeInt64Filter(size_t column_index,
                           std::function<bool(int64_t)> condition) {
    return MakeRawFilter(column_index, [condition = std::move(condition)](
                                           const char *data, size_t size) {
        return condition(ReadIntegerFilterValue(data, size));
    });
}

FilterTask MakeInt64EqualFilter(size_t column_index, int64_t expected) {
    return MakeInt64Filter(
        column_index, [expected](int64_t value) { return value == expected; });
}

FilterTask MakeInt64NotEqualFilter(size_t column_index, int64_t expected) {
    return MakeInt64Filter(
        column_index, [expected](int64_t value) { return value != expected; });
}

FilterTask MakeInt64LessFilter(size_t column_index, int64_t bound) {
    return MakeInt64Filter(column_index,
                           [bound](int64_t value) { return value < bound; });
}

FilterTask MakeInt64LessOrEqualFilter(size_t column_index, int64_t bound) {
    return MakeInt64Filter(column_index,
                           [bound](int64_t value) { return value <= bound; });
}

FilterTask MakeInt64GreaterFilter(size_t column_index, int64_t bound) {
    return MakeInt64Filter(column_index,
                           [bound](int64_t value) { return value > bound; });
}

FilterTask MakeInt64GreaterOrEqualFilter(size_t column_index, int64_t bound) {
    return MakeInt64Filter(column_index,
                           [bound](int64_t value) { return value >= bound; });
}

FilterTask MakeInt64InFilter(size_t column_index,
                             std::unordered_set<int64_t> values) {
    return MakeInt64Filter(column_index,
                           [values = std::move(values)](int64_t value) {
                               return values.contains(value);
                           });
}

FilterTask MakeStringFilter(size_t column_index,
                            std::function<bool(std::string_view)> condition) {
    return MakeRawFilter(column_index, [condition = std::move(condition)](
                                           const char *data, size_t size) {
        return condition(std::string_view(data, size));
    });
}

FilterTask MakeStringEqualFilter(size_t column_index, std::string expected) {
    return MakeStringFilter(
        column_index, [expected = std::move(expected)](std::string_view value) {
            return value == expected;
        });
}

FilterTask MakeStringNotEqualFilter(size_t column_index, std::string expected) {
    return MakeStringFilter(
        column_index, [expected = std::move(expected)](std::string_view value) {
            return value != expected;
        });
}

FilterTask MakeStringRegexFilter(size_t column_index, std::string pattern) {
    return MakeStringFilter(
        column_index,
        [regex = std::regex(std::move(pattern))](std::string_view value) {
            return std::regex_search(value.begin(), value.end(), regex);
        });
}

FilterTask MakeStringNotRegexFilter(size_t column_index, std::string pattern) {
    return MakeStringFilter(
        column_index,
        [regex = std::regex(std::move(pattern))](std::string_view value) {
            return !std::regex_search(value.begin(), value.end(), regex);
        });
}

FilterTask MakeStringLikeFilter(size_t column_index, std::string pattern) {
    return MakeStringFilter(column_index,
                            MakeSqlLikeMatcher(std::move(pattern)));
}

FilterTask MakeStringNotLikeFilter(size_t column_index, std::string pattern) {
    auto matcher = MakeSqlLikeMatcher(std::move(pattern));

    return MakeStringFilter(
        column_index, [matcher = std::move(matcher)](std::string_view value) {
            return !matcher(value);
        });
}

FilterTask MakeTimePointFilter(
    size_t column_index,
    std::function<bool(std::chrono::system_clock::time_point)> condition) {
    return MakeRawFilter(column_index, [condition = std::move(condition)](
                                           const char *data, size_t size) {
        if (size != sizeof(std::chrono::system_clock::time_point)) {
            throw std::invalid_argument("Expected time point value");
        }

        std::chrono::system_clock::time_point value;
        std::memcpy(&value, data, sizeof(value));
        return condition(value);
    });
}

FilterTask MakeTimestampRangeFilter(size_t column_index, std::string from,
                                    std::string to) {
    const auto from_value = ParseFilterTimePoint(from, false);
    const auto to_value = ParseFilterTimePoint(to, false);
    return MakeTimePointFilter(
        column_index, [from_value, to_value](auto value) {
            return value >= from_value && value <= to_value;
        });
}

FilterTask MakeDateRangeFilter(size_t column_index, std::string from,
                               std::string to) {
    const auto from_value = ParseFilterTimePoint(from, true);
    const auto to_value = ParseFilterTimePoint(to, true);
    return MakeTimePointFilter(
        column_index, [from_value, to_value](auto value) {
            return value >= from_value && value <= to_value;
        });
}

Filter MakeFilter(std::vector<FilterTask> &&conditions) {
    return Filter(std::move(conditions));
}

void Filter::Execute(Batch &batch) {
    auto enabled = static_cast<const Batch &>(batch).GetEnabledColumns();
    for (auto &task : conditions_) {
        std::unordered_set<size_t> next_enabled;

        if (!enabled.has_value()) {
            for (size_t ind = 0; ind < batch.VerticalSize(); ++ind) {
                if (CheckFilterCondition(batch, task, ind)) {
                    next_enabled.insert(ind);
                }
            }
        } else {
            for (size_t ind : enabled.value()) {
                if (CheckFilterCondition(batch, task, ind)) {
                    next_enabled.insert(ind);
                }
            }
        }

        enabled = std::move(next_enabled);
    }

    batch.SetEnabledColumns(std::move(enabled));
}

TopK::TopK(std::vector<size_t> &&column_indices, size_t k,
           const Scheme &result_scheme, SortDirection direction)
    : sort_keys_(), ans_(CompareForTopK(sort_keys_)), k_(k),
      result_scheme_(result_scheme) {
    sort_keys_.reserve(column_indices.size());
    for (size_t column_index : column_indices) {
        sort_keys_.push_back({column_index, direction});
    }
}

TopK::TopK(std::vector<SortKey> &&sort_keys, size_t k,
           const Scheme &result_scheme)
    : sort_keys_(std::move(sort_keys)), ans_(CompareForTopK(sort_keys_)), k_(k),
      result_scheme_(result_scheme) {}

SortKey MakeSortKey(size_t column_index, SortDirection direction) {
    return SortKey{column_index, direction};
}

SortKey MakeAscendingSortKey(size_t column_index) {
    return MakeSortKey(column_index, SortDirection::Ascending);
}

SortKey MakeDescendingSortKey(size_t column_index) {
    return MakeSortKey(column_index, SortDirection::Descending);
}

TopK MakeTopK(std::vector<SortKey> &&sort_keys, size_t k,
              const Scheme &result_scheme) {
    return TopK(std::move(sort_keys), k, result_scheme);
}

TopK MakeTopK(std::vector<size_t> &&column_indices, size_t k,
              const Scheme &result_scheme, SortDirection direction) {
    return TopK(std::move(column_indices), k, result_scheme, direction);
}

void TopK::Process(const Batch &batch) {
    if (batch.GetEnabledColumns().has_value()) {
        for (auto &ind : batch.GetEnabledColumns().value()) {
            ans_.insert(batch.GetRowLikeColumnVector(ind));
            if (ans_.size() > k_) {
                ans_.erase(std::prev(ans_.end()));
            }
        }
        return;
    } else {
        for (size_t i = 0; i < batch.VerticalSize(); ++i) {
            ans_.insert(batch.GetRowLikeColumnVector(i));
            if (ans_.size() > k_) {
                ans_.erase(std::prev(ans_.end()));
            }
        }
    }
}

std::vector<Batch> TopK::Finalize() && {
    Batch part_of_ans(result_scheme_);

    std::vector<Batch> result;

    for (auto &row : ans_) {
        if (!part_of_ans.EnableToPush()) {
            result.emplace_back(std::move(part_of_ans));
            part_of_ans = Batch(result_scheme_);
        }
        part_of_ans.PushColumnVector(row);
    }

    if (!part_of_ans.IsEmpty()) {
        result.emplace_back(std::move(part_of_ans));
    }

    return result;
}

GroupBy::GroupBy(GroupByTask &&task, const Scheme &scheme)
    : task_(std::move(task)), scheme_(scheme) {}

GroupAggTask MakeGroupAgg(AggType type, size_t column_index) {
    return GroupAggTask{type, column_index};
}

GroupAggTask MakeGroupSum(size_t column_index) {
    return MakeGroupAgg(AggType::Sum, column_index);
}

GroupAggTask MakeGroupMin(size_t column_index) {
    return MakeGroupAgg(AggType::Min, column_index);
}

GroupAggTask MakeGroupMax(size_t column_index) {
    return MakeGroupAgg(AggType::Max, column_index);
}

GroupAggTask MakeGroupCount(size_t column_index) {
    return MakeGroupAgg(AggType::Count, column_index);
}

GroupAggTask MakeGroupCountDistinct(size_t column_index) {
    return MakeGroupAgg(AggType::CountDistinct, column_index);
}

GroupAggTask MakeGroupAvg(size_t column_index) {
    return MakeGroupAgg(AggType::Avg, column_index);
}

GroupByTask MakeGroupByTask(std::vector<size_t> &&group_column_indices,
                            std::vector<GroupAggTask> &&aggregations) {
    GroupByTask task;
    task.column_indices = std::move(group_column_indices);
    task.types_.reserve(aggregations.size());
    task.agg_column_indices.reserve(aggregations.size());

    for (const auto &aggregation : aggregations) {
        task.types_.emplace_back(aggregation.type);
        task.agg_column_indices.emplace_back(aggregation.column_index);
    }

    if (!task.agg_column_indices.empty()) {
        task.agg_column_index = task.agg_column_indices.front();
    }

    return task;
}

GroupByTask MakeGroupByTask(std::vector<size_t> &&group_column_indices,
                            AggType agg_type, size_t agg_column_index) {
    return MakeGroupByTask(std::move(group_column_indices),
                           {MakeGroupAgg(agg_type, agg_column_index)});
}

GroupBy MakeGroupBy(GroupByTask &&task, const Scheme &scheme) {
    return GroupBy(std::move(task), scheme);
}

std::string BuildKey(const Batch &batch,
                     const std::vector<size_t> &column_indices, size_t index);

template <typename T> void UpdateMinValue(ResultAggVariant &result, T value) {
    if (auto *current = std::get_if<T>(&result)) {
        *current = std::min(*current, value);
    } else {
        result = std::move(value);
    }
}

template <typename T> void UpdateMaxValue(ResultAggVariant &result, T value) {
    if (auto *current = std::get_if<T>(&result)) {
        *current = std::max(*current, value);
    } else {
        result = std::move(value);
    }
}

template <typename ReadT, typename ResultT>
void UpdateSumValue(ResultAggVariant &result,
                    const std::shared_ptr<Column> &column, size_t row_index) {
    const ResultT value =
        static_cast<ResultT>(ReadColumnValue<ReadT>(column, row_index));
    if (auto *current = std::get_if<ResultT>(&result)) {
        *current += value;
    } else {
        result = value;
    }
}

template <typename ReadT, typename ResultT>
void UpdateAvgValue(ResultAggVariant &result,
                    const std::shared_ptr<Column> &column, size_t row_index) {
    const ResultT value =
        static_cast<ResultT>(ReadColumnValue<ReadT>(column, row_index));
    if (auto *current = std::get_if<std::pair<ResultT, size_t>>(&result)) {
        current->first += value;
        ++current->second;
    } else {
        result = std::make_pair(value, size_t{1});
    }
}

template <typename T, typename SetT = std::unordered_set<T>>
void UpdateDistinctValue(ResultAggVariant &result,
                         const std::shared_ptr<Column> &column,
                         size_t row_index) {
    if (!std::holds_alternative<SetT>(result)) {
        result = SetT{};
    }

    auto &current = std::get<SetT>(result);
    current.insert(ReadColumnValue<T>(column, row_index));
}

void UpdateDistinctStringValue(ResultAggVariant &result,
                               const std::shared_ptr<Column> &column,
                               size_t row_index) {
    if (!std::holds_alternative<std::unordered_set<std::string>>(result)) {
        result = std::unordered_set<std::string>{};
    }

    auto &current = std::get<std::unordered_set<std::string>>(result);
    current.insert(ReadStringValue(column, row_index));
}

void UpdateExtremeValue(ResultAggVariant &result,
                        const std::shared_ptr<Column> &column, size_t row_index,
                        AggType agg_type) {
    const ColumnTypes type = column->GetColumnType();
    switch (type) {
    case ColumnTypes::Int16: {
        int16_t value = ReadColumnValue<int16_t>(column, row_index);
        agg_type == AggType::Min ? UpdateMinValue(result, value)
                                 : UpdateMaxValue(result, value);
        break;
    }
    case ColumnTypes::Int32: {
        int32_t value = ReadColumnValue<int32_t>(column, row_index);
        agg_type == AggType::Min ? UpdateMinValue(result, value)
                                 : UpdateMaxValue(result, value);
        break;
    }
    case ColumnTypes::Int64: {
        int64_t value = ReadColumnValue<int64_t>(column, row_index);
        agg_type == AggType::Min ? UpdateMinValue(result, value)
                                 : UpdateMaxValue(result, value);
        break;
    }
    case ColumnTypes::Int128: {
        __int128 value = ReadColumnValue<__int128>(column, row_index);
        agg_type == AggType::Min ? UpdateMinValue(result, value)
                                 : UpdateMaxValue(result, value);
        break;
    }
    case ColumnTypes::Double: {
        double value = ReadColumnValue<double>(column, row_index);
        agg_type == AggType::Min ? UpdateMinValue(result, value)
                                 : UpdateMaxValue(result, value);
        break;
    }
    case ColumnTypes::Timestamp:
    case ColumnTypes::Date: {
        auto value = ReadColumnValue<std::chrono::system_clock::time_point>(
            column, row_index);
        agg_type == AggType::Min ? UpdateMinValue(result, value)
                                 : UpdateMaxValue(result, value);
        break;
    }
    case ColumnTypes::String:
    case ColumnTypes::Unknown: {
        std::string value = ReadStringValue(column, row_index);
        agg_type == AggType::Min ? UpdateMinValue(result, value)
                                 : UpdateMaxValue(result, value);
        break;
    }
    default:
        throw std::invalid_argument("Unsupported column type for min/max");
    }
}

void UpdateSumForGroupBy(ResultAggVariant &result,
                         const std::shared_ptr<Column> &column,
                         size_t row_index) {
    const ColumnTypes type = column->GetColumnType();
    switch (type) {
    case ColumnTypes::Int16:
        return UpdateSumValue<int16_t, __int128>(result, column, row_index);
    case ColumnTypes::Int32:
        return UpdateSumValue<int32_t, __int128>(result, column, row_index);
    case ColumnTypes::Int64:
        return UpdateSumValue<int64_t, __int128>(result, column, row_index);
    case ColumnTypes::Int128:
        return UpdateSumValue<__int128, __int128>(result, column, row_index);
    case ColumnTypes::Double:
        return UpdateSumValue<double, double>(result, column, row_index);
    default:
        throw std::invalid_argument("Unsupported column type for sum");
    }
}

void UpdateAvgForGroupBy(ResultAggVariant &result,
                         const std::shared_ptr<Column> &column,
                         size_t row_index) {
    const ColumnTypes type = column->GetColumnType();
    switch (type) {
    case ColumnTypes::Int16:
        return UpdateAvgValue<int16_t, __int128>(result, column, row_index);
    case ColumnTypes::Int32:
        return UpdateAvgValue<int32_t, __int128>(result, column, row_index);
    case ColumnTypes::Int64:
        return UpdateAvgValue<int64_t, __int128>(result, column, row_index);
    case ColumnTypes::Int128:
        return UpdateAvgValue<__int128, __int128>(result, column, row_index);
    case ColumnTypes::Double:
        return UpdateAvgValue<double, double>(result, column, row_index);
    default:
        throw std::invalid_argument("Unsupported column type for average");
    }
}

void UpdateDistinctForGroupBy(ResultAggVariant &result,
                              const std::shared_ptr<Column> &column,
                              size_t row_index) {
    const ColumnTypes type = column->GetColumnType();
    switch (type) {
    case ColumnTypes::Int16:
        return UpdateDistinctValue<int16_t>(result, column, row_index);
    case ColumnTypes::Int32:
        return UpdateDistinctValue<int32_t>(result, column, row_index);
    case ColumnTypes::Int64:
        return UpdateDistinctValue<int64_t>(result, column, row_index);
    case ColumnTypes::Int128:
        return UpdateDistinctValue<__int128>(result, column, row_index);
    case ColumnTypes::Double:
        return UpdateDistinctValue<double>(result, column, row_index);
    case ColumnTypes::Timestamp:
    case ColumnTypes::Date:
        return UpdateDistinctValue<
            std::chrono::system_clock::time_point,
            std::unordered_set<std::chrono::system_clock::time_point,
                               TimePointHash>>(result, column, row_index);
    case ColumnTypes::String:
    case ColumnTypes::Unknown:
        return UpdateDistinctStringValue(result, column, row_index);
    default:
        throw std::invalid_argument("Unsupported column type for distinct");
    }
}

size_t GetAggColumnIndex(const GroupByTask &task, size_t agg_index) {
    if (!task.agg_column_indices.empty()) {
        return task.agg_column_indices.at(agg_index);
    }

    return task.agg_column_index;
}

void UpdateSingleAggValue(size_t row_index, ResultAggVariant &current,
                          AggType agg_type, size_t agg_column_index,
                          const Batch &batch) {
    switch (agg_type) {
    case AggType::Count:
        if (auto *count = std::get_if<uint64_t>(&current)) {
            ++(*count);
        } else {
            current = uint64_t{1};
        }
        return;
    case AggType::Sum: {
        auto column = batch.GetColumn(agg_column_index);
        UpdateSumForGroupBy(current, column, row_index);
        return;
    }
    case AggType::Avg: {
        auto column = batch.GetColumn(agg_column_index);
        UpdateAvgForGroupBy(current, column, row_index);
        return;
    }
    case AggType::Min:
    case AggType::Max: {
        auto column = batch.GetColumn(agg_column_index);
        UpdateExtremeValue(current, column, row_index, agg_type);
        return;
    }
    case AggType::CountDistinct: {
        auto column = batch.GetColumn(agg_column_index);
        UpdateDistinctForGroupBy(current, column, row_index);
        return;
    }
    default:
        throw std::invalid_argument("Unsupported aggregation type");
    }
}

void UpdateKeyValue(
    size_t row_index,
    std::unordered_map<std::string, std::vector<ResultAggVariant>> &result,
    const GroupByTask &task, const Batch &batch) {
    auto key = BuildKey(batch, task.column_indices, row_index);
    auto &current = result[key];

    if (current.empty()) {
        current.resize(task.types_.size());
    }

    for (size_t i = 0; i < task.types_.size(); ++i) {
        UpdateSingleAggValue(row_index, current[i], task.types_[i],
                             GetAggColumnIndex(task, i), batch);
    }
}

std::string BuildKey(const Batch &batch,
                     const std::vector<size_t> &column_indices, size_t index) {
    std::string result;
    for (auto &ind : column_indices) {
        auto column = batch.GetColumns()[ind];
        auto value = column->Get(index);
        if (IsStringType(column->GetColumnType())) {
            result.append(reinterpret_cast<const char *>(&value.size),
                          sizeof(value.size));
        }
        result.append(value.data, value.size);
    }

    return result;
}

void GroupBy::Process(const Batch &batch) {
    auto enabled = batch.GetEnabledColumns();
    if (enabled.has_value()) {
        for (auto &ind : enabled.value()) {
            UpdateKeyValue(ind, result_, task_, batch);
        }
        return;
    }

    for (size_t i = 0; i < batch.VerticalSize(); ++i) {
        UpdateKeyValue(i, result_, task_, batch);
    }
}

void WriteKeyToResult(Batch &result, const std::string &key,
                      const GroupByTask &task, const Scheme &source_scheme) {
    size_t key_offset = 0;

    for (size_t result_column = 0; result_column < task.column_indices.size();
         ++result_column) {
        const size_t source_column = task.column_indices[result_column];
        const ColumnTypes type = source_scheme.GetSchemeTypes()[source_column];

        if (IsStringType(type)) {
            size_t string_size;
            std::memcpy(&string_size, key.data() + key_offset,
                        sizeof(string_size));
            key_offset += sizeof(string_size);
            result.AddToColumn(key.data() + key_offset, string_size,
                               result_column);
            key_offset += string_size;
        } else {
            const size_t value_size = FixedTypeSize(type);
            result.AddToColumn(key.data() + key_offset, value_size,
                               result_column);
            key_offset += value_size;
        }
    }
}

void WriteCountDistinctToResult(Batch &result, const ResultAggVariant &value,
                                size_t column_index) {
    size_t distinct_count = 0;
    if (auto *set = std::get_if<std::unordered_set<int16_t>>(&value)) {
        distinct_count = set->size();
    } else if (auto *set = std::get_if<std::unordered_set<int32_t>>(&value)) {
        distinct_count = set->size();
    } else if (auto *set = std::get_if<std::unordered_set<int64_t>>(&value)) {
        distinct_count = set->size();
    } else if (auto *set = std::get_if<std::unordered_set<__int128>>(&value)) {
        distinct_count = set->size();
    } else if (auto *set = std::get_if<std::unordered_set<double>>(&value)) {
        distinct_count = set->size();
    } else if (auto *set = std::get_if<std::unordered_set<
                   std::chrono::system_clock::time_point, TimePointHash>>(
                   &value)) {
        distinct_count = set->size();
    } else if (auto *set =
                   std::get_if<std::unordered_set<std::string>>(&value)) {
        distinct_count = set->size();
    } else {
        throw std::invalid_argument("Unexpected count distinct state");
    }

    int64_t result_value = static_cast<int64_t>(distinct_count);
    result.AddToColumn(reinterpret_cast<const char *>(&result_value),
                       sizeof(result_value), column_index);
}

void WriteAggValueToResult(Batch &result, const ResultAggVariant &value,
                           AggType agg_type, size_t column_index) {
    switch (agg_type) {
    case AggType::Count: {
        uint64_t count = std::get<uint64_t>(value);
        int64_t result_value = static_cast<int64_t>(count);
        result.AddToColumn(reinterpret_cast<const char *>(&result_value),
                           sizeof(result_value), column_index);
        return;
    }
    case AggType::CountDistinct:
        return WriteCountDistinctToResult(result, value, column_index);
    case AggType::Avg:
        if (auto *current = std::get_if<std::pair<__int128, size_t>>(&value)) {
            if (current->second == 0) {
                throw std::invalid_argument(
                    "Cannot calculate average of empty group");
            }
            __int128 avg =
                current->first / static_cast<__int128>(current->second);
            result.AddToColumn(reinterpret_cast<const char *>(&avg),
                               sizeof(avg), column_index);
        } else if (auto *current =
                       std::get_if<std::pair<double, size_t>>(&value)) {
            if (current->second == 0) {
                throw std::invalid_argument(
                    "Cannot calculate average of empty group");
            }
            __int128 avg =
                static_cast<__int128>(current->first / current->second);
            result.AddToColumn(reinterpret_cast<const char *>(&avg),
                               sizeof(avg), column_index);
        } else {
            throw std::invalid_argument("Unexpected average state");
        }
        return;
    case AggType::Sum:
    case AggType::Min:
    case AggType::Max:
        break;
    default:
        throw std::invalid_argument("Unsupported aggregation type");
    }

    if (auto *current = std::get_if<int16_t>(&value)) {
        result.AddToColumn(reinterpret_cast<const char *>(current),
                           sizeof(*current), column_index);
    } else if (auto *current = std::get_if<int32_t>(&value)) {
        result.AddToColumn(reinterpret_cast<const char *>(current),
                           sizeof(*current), column_index);
    } else if (auto *current = std::get_if<int64_t>(&value)) {
        result.AddToColumn(reinterpret_cast<const char *>(current),
                           sizeof(*current), column_index);
    } else if (auto *current = std::get_if<__int128>(&value)) {
        result.AddToColumn(reinterpret_cast<const char *>(current),
                           sizeof(*current), column_index);
    } else if (auto *current = std::get_if<double>(&value)) {
        result.AddToColumn(reinterpret_cast<const char *>(current),
                           sizeof(*current), column_index);
    } else if (auto *current = std::get_if<std::string>(&value)) {
        result.AddToColumn(current->data(), current->size(), column_index);
    } else if (auto *current =
                   std::get_if<std::chrono::system_clock::time_point>(&value)) {
        result.AddToColumn(reinterpret_cast<const char *>(current),
                           sizeof(*current), column_index);
    } else {
        throw std::invalid_argument("Unexpected aggregation state");
    }
}

Scheme BuildGroupByResultScheme(const GroupByTask &task,
                                const Scheme &source_scheme) {
    Scheme result_scheme;
    const auto &names = source_scheme.GetSchemeNames();
    const auto &types = source_scheme.GetSchemeTypes();

    for (size_t column_index : task.column_indices) {
        result_scheme.Add(
            {names[column_index], GetNameByType(types[column_index])});
    }

    for (size_t i = 0; i < task.types_.size(); ++i) {
        const AggType agg_type = task.types_[i];
        const ColumnTypes agg_column_type = [&]() {
            switch (agg_type) {
            case AggType::Count:
                return ColumnTypes::Int64;
            default:
                return types[GetAggColumnIndex(task, i)];
            }
        }();
        result_scheme.Add(
            {"result_" + AggTypeName(agg_type),
             GetNameByType(GroupByResultType(agg_type, agg_column_type))});
    }

    return result_scheme;
}

std::vector<Batch> GroupBy::Finalize() && {
    Scheme result_scheme = BuildGroupByResultScheme(task_, scheme_);
    std::vector<Batch> result;
    Batch current(result_scheme, false);

    for (const auto &[key, values] : result_) {
        if (!current.EnableToPush()) {
            result.emplace_back(std::move(current));
            current = Batch(result_scheme, false);
        }

        WriteKeyToResult(current, key, task_, scheme_);
        for (size_t i = 0; i < values.size(); ++i) {
            WriteAggValueToResult(current, values[i], task_.types_[i],
                                  task_.column_indices.size() + i);
        }
    }

    if (!current.IsEmpty()) {
        result.emplace_back(std::move(current));
    }

    return result;
}

Offset::Offset(size_t offset) : offset_(offset) {}

Offset MakeOffset(size_t offset) { return Offset(offset); }

void Offset::Process(const Batch &batch) {
    const auto PushRow = [&](size_t row) {
        if (offset_ > 0) {
            --offset_;
            return;
        }

        if (result_.empty() || !result_.back().EnableToPush()) {
            result_.emplace_back(batch.GetScheme());
        }
        result_.back().PushColumnVector(batch.GetRowLikeColumnVector(row));
    };

    for (size_t row = 0; row < batch.VerticalSize(); ++row) {
        if (batch.IsRowEnabled(row)) {
            PushRow(row);
        }
    }
}

std::vector<Batch> Offset::Finalize() && {
    if (!result_.empty() && result_.back().IsEmpty()) {
        result_.pop_back();
    }

    return std::move(result_);
}

Expression::Expression(std::vector<ExpressionTask> &&tasks)
    : tasks_(std::move(tasks)) {}

Expression MakeExpression(std::vector<ExpressionTask> &&tasks) {
    return Expression(std::move(tasks));
}

void Expression::Execute(Batch &batch) {
    std::vector<std::shared_ptr<Column>> expression_columns;
    expression_columns.reserve(tasks_.size());
    for (const auto &task : tasks_) {
        expression_columns.emplace_back(MakeExpressionColumn(task.result_type));
    }

    for (size_t row = 0; row < batch.VerticalSize(); ++row) {
        for (size_t task_index = 0; task_index < tasks_.size(); ++task_index) {
            tasks_[task_index].eval(batch, row,
                                    *expression_columns[task_index]);
        }
    }

    for (size_t task_index = 0; task_index < tasks_.size(); ++task_index) {
        batch.AddColumn(std::move(expression_columns[task_index]),
                        tasks_[task_index].result_type,
                        tasks_[task_index].alias);
    }
}

ExpressionTask MakeCopyColumnExpression(size_t column_index, std::string alias,
                                        ColumnTypes result_type) {
    return ExpressionTask{
        std::move(alias), result_type,
        [column_index](const Batch &input, size_t row, Column &output) {
            auto value = input.GetColumn(column_index)->Get(row);
            output.Push(value.data, value.size);
        }};
}

ExpressionTask MakeConstantInt64Expression(int64_t value, std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::Int64,
        [value](const Batch &, size_t, Column &output) {
            output.Push(reinterpret_cast<const char *>(&value), sizeof(value));
        }};
}

ExpressionTask MakeConstantStringExpression(std::string value,
                                            std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::String,
        [value = std::move(value)](const Batch &, size_t, Column &output) {
            output.Push(value.data(), value.size());
        }};
}

ExpressionTask MakeAddInt64ConstantExpression(size_t column_index,
                                              int64_t value,
                                              std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::Int64,
        [column_index, value](const Batch &input, size_t row, Column &output) {
            int64_t result =
                ReadIntegerColumnValue(input.GetColumn(column_index), row) +
                value;
            output.Push(reinterpret_cast<const char *>(&result),
                        sizeof(result));
        }};
}

ExpressionTask MakeSubInt64ConstantExpression(size_t column_index,
                                              int64_t value,
                                              std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::Int64,
        [column_index, value](const Batch &input, size_t row, Column &output) {
            int64_t result =
                ReadIntegerColumnValue(input.GetColumn(column_index), row) -
                value;
            output.Push(reinterpret_cast<const char *>(&result),
                        sizeof(result));
        }};
}

ExpressionTask MakeInt128AddInt64ProductExpression(size_t int128_column_index,
                                                   size_t int64_column_index,
                                                   int64_t multiplier,
                                                   std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::Int128,
        [int128_column_index, int64_column_index,
         multiplier](const Batch &input, size_t row, Column &output) {
            __int128 result = ReadColumnValue<__int128>(
                input.GetColumn(int128_column_index), row);
            result += static_cast<__int128>(ReadIntegerColumnValue(
                          input.GetColumn(int64_column_index), row)) *
                      static_cast<__int128>(multiplier);
            output.Push(reinterpret_cast<const char *>(&result),
                        sizeof(result));
        }};
}

ExpressionTask MakeLengthExpression(size_t column_index, std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::Int64,
        [column_index](const Batch &input, size_t row, Column &output) {
            auto value = input.GetColumn(column_index)->Get(row);
            int64_t result = static_cast<int64_t>(value.size);
            output.Push(reinterpret_cast<const char *>(&result),
                        sizeof(result));
        }};
}

ExpressionTask MakeExtractMinuteExpression(size_t column_index,
                                           std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::Int64,
        [column_index](const Batch &input, size_t row, Column &output) {
            auto timestamp =
                ReadColumnValue<std::chrono::system_clock::time_point>(
                    input.GetColumn(column_index), row);
            int64_t result = ExtractMinuteValue(timestamp);
            output.Push(reinterpret_cast<const char *>(&result),
                        sizeof(result));
        }};
}

ExpressionTask MakeDateTruncMinuteExpression(size_t column_index,
                                             std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::Timestamp,
        [column_index](const Batch &input, size_t row, Column &output) {
            auto timestamp =
                ReadColumnValue<std::chrono::system_clock::time_point>(
                    input.GetColumn(column_index), row);
            auto result = TruncateToMinute(timestamp);
            output.Push(reinterpret_cast<const char *>(&result),
                        sizeof(result));
        }};
}

ExpressionTask MakeCaseWhenBothZeroThenStringElseEmptyExpression(
    size_t first_column_index, size_t second_column_index,
    size_t string_column_index, std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::String,
        [first_column_index, second_column_index,
         string_column_index](const Batch &input, size_t row, Column &output) {
            int64_t first = ReadIntegerColumnValue(
                input.GetColumn(first_column_index), row);
            int64_t second = ReadIntegerColumnValue(
                input.GetColumn(second_column_index), row);
            if (first == 0 && second == 0) {
                auto value = input.GetColumn(string_column_index)->Get(row);
                output.Push(value.data, value.size);
            } else {
                output.Push("", 0);
            }
        }};
}

ExpressionTask MakeExtractDomainExpression(size_t column_index,
                                           std::string alias) {
    return ExpressionTask{
        std::move(alias), ColumnTypes::String,
        [column_index](const Batch &input, size_t row, Column &output) {
            auto value = input.GetColumn(column_index)->Get(row);
            std::string domain =
                ExtractDomain(std::string_view(value.data, value.size));
            output.Push(domain.data(), domain.size());
        }};
}

SelectAnswer::SelectAnswer(std::vector<size_t> &&column_indices)
    : column_indices_(std::move(column_indices)) {
    std::sort(column_indices_.begin(), column_indices_.end());
    column_indices_.erase(
        std::unique(column_indices_.begin(), column_indices_.end()),
        column_indices_.end());
}

void SelectAnswer::Execute(Batch &batch) {
    for (size_t column_index : column_indices_) {
        if (column_index >= batch.HorizontalSize()) {
            throw std::out_of_range("Incorrect column index");
        }
    }

    std::unordered_set<size_t> selected_columns(column_indices_.begin(),
                                                column_indices_.end());
    for (size_t column_index = batch.HorizontalSize(); column_index > 0;
         --column_index) {
        const size_t current = column_index - 1;
        if (!selected_columns.contains(current)) {
            batch.RemoveColumn(current);
        }
    }
}

SelectAnswer MakeSelectAnswer(std::vector<size_t> &&column_indices) {
    return SelectAnswer(std::move(column_indices));
}
