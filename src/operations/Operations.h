#pragma once

#include "data_structures/Batch.h"
#include "data_structures/Column.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

enum class OperationType { Blocking, NonBlocking };

class Operation {
  public:
    virtual ~Operation() = default;
    virtual OperationType GetType() const = 0;
};

class StreamingOperation : public Operation {
  public:
    OperationType GetType() const override;

    virtual void Execute(Batch &batch) = 0;
};

class BlockingOperation : public Operation {
  public:
    OperationType GetType() const override;
    virtual void Process(const Batch &batch) = 0;

    virtual std::vector<Batch> Finalize() && = 0;
};

enum class AggType { Sum, Min, Max, CountDistinct, Avg, Count };

struct AggTask {
    size_t column_index;
    AggType agg_type;

    ColumnTypes return_type;

    std::string alias;
};

AggTask MakeAggTask(size_t column_index, AggType agg_type,
                    ColumnTypes return_type, std::string alias);
AggTask MakeSumAgg(size_t column_index, std::string alias,
                   ColumnTypes return_type = ColumnTypes::Int128);
AggTask MakeMinAgg(size_t column_index, ColumnTypes return_type,
                   std::string alias);
AggTask MakeMaxAgg(size_t column_index, ColumnTypes return_type,
                   std::string alias);
AggTask MakeCountAgg(size_t column_index = 0,
                     std::string alias = "result_count");
AggTask MakeCountDistinctAgg(size_t column_index, std::string alias);
AggTask MakeAvgAgg(size_t column_index, std::string alias);

struct TimePointHash {
    size_t
    operator()(const std::chrono::system_clock::time_point &value) const {
        return std::hash<std::chrono::system_clock::duration::rep>{}(
            value.time_since_epoch().count());
    }
};

using ResultAggVariant = std::variant<
    std::monostate, __int128, int16_t, int32_t, int64_t, uint64_t, std::string,
    double, std::chrono::system_clock::time_point, std::pair<__int128, size_t>,
    std::pair<double, size_t>, std::unordered_set<__int128>,
    std::unordered_set<double>,
    std::unordered_set<std::chrono::system_clock::time_point, TimePointHash>,
    std::unordered_set<int16_t>, std::unordered_set<int32_t>,
    std::unordered_set<int64_t>, std::unordered_set<std::string>>;

class Aggregation : public BlockingOperation {
  public:
    Aggregation(std::vector<AggTask> &&tasks);
    void Process(const Batch &batch) override;
    std::vector<Batch> Finalize() && override;

  private:
    std::vector<AggTask> tasks_;
    std::vector<ResultAggVariant> results_;
    std::vector<size_t> column_active_size_;
};

Aggregation MakeAggregation(std::vector<AggTask> &&tasks);

struct FilterTask {
    size_t column_index;
    std::function<bool(const char *, size_t)> condition;
};

FilterTask MakeRawFilter(size_t column_index,
                         std::function<bool(const char *, size_t)> condition);
FilterTask MakeInt64Filter(size_t column_index,
                           std::function<bool(int64_t)> condition);
FilterTask MakeInt64EqualFilter(size_t column_index, int64_t expected);
FilterTask MakeInt64NotEqualFilter(size_t column_index, int64_t expected);
FilterTask MakeInt64LessFilter(size_t column_index, int64_t bound);
FilterTask MakeInt64LessOrEqualFilter(size_t column_index, int64_t bound);
FilterTask MakeInt64GreaterFilter(size_t column_index, int64_t bound);
FilterTask MakeInt64GreaterOrEqualFilter(size_t column_index, int64_t bound);
FilterTask MakeInt64InFilter(size_t column_index,
                             std::unordered_set<int64_t> values);
FilterTask MakeStringFilter(size_t column_index,
                            std::function<bool(std::string_view)> condition);
FilterTask MakeStringEqualFilter(size_t column_index, std::string expected);
FilterTask MakeStringNotEqualFilter(size_t column_index, std::string expected);
FilterTask MakeStringRegexFilter(size_t column_index, std::string pattern);
FilterTask MakeStringNotRegexFilter(size_t column_index, std::string pattern);
FilterTask MakeStringLikeFilter(size_t column_index, std::string pattern);
FilterTask MakeStringNotLikeFilter(size_t column_index, std::string pattern);
FilterTask MakeTimePointFilter(
    size_t column_index,
    std::function<bool(std::chrono::system_clock::time_point)> condition);
FilterTask MakeTimestampRangeFilter(size_t column_index, std::string from,
                                    std::string to);
FilterTask MakeDateRangeFilter(size_t column_index, std::string from,
                               std::string to);

class Filter : public StreamingOperation {
  public:
    Filter() = default;
    explicit Filter(std::vector<FilterTask> &&conditions);

    void Execute(Batch &batch) override;

  private:
    std::vector<FilterTask> conditions_;
};

Filter MakeFilter(std::vector<FilterTask> &&conditions);

enum class SortDirection { Ascending, Descending };

struct SortKey {
    size_t column_index;
    SortDirection direction = SortDirection::Ascending;
};

SortKey MakeSortKey(size_t column_index, SortDirection direction);
SortKey MakeAscendingSortKey(size_t column_index);
SortKey MakeDescendingSortKey(size_t column_index);

struct CompareForTopK {
    std::vector<SortKey> sort_keys;

    explicit CompareForTopK(const std::vector<SortKey> &keys)
        : sort_keys(keys) {}

    bool operator()(const std::vector<std::shared_ptr<Column>> &a,
                    const std::vector<std::shared_ptr<Column>> &b) const {
        for (const auto &key : sort_keys) {
            const size_t idx = key.column_index;
            if (*a[idx] != *b[idx]) {
                if (key.direction == SortDirection::Ascending) {
                    return *a[idx] < *b[idx];
                }
                return *b[idx] < *a[idx];
            }
        }
        return false;
    }
};

class TopK : public BlockingOperation {
  public:
    TopK(std::vector<size_t> &&column_indices, size_t k,
         const Scheme &result_scheme,
         SortDirection direction = SortDirection::Descending);
    TopK(std::vector<SortKey> &&sort_keys, size_t k,
         const Scheme &result_scheme);
    void Process(const Batch &batch) override;
    std::vector<Batch> Finalize() && override;

  private:
    std::vector<SortKey> sort_keys_;
    std::multiset<std::vector<std::shared_ptr<Column>>, CompareForTopK> ans_;
    size_t k_;

    Scheme result_scheme_;
};

TopK MakeTopK(std::vector<SortKey> &&sort_keys, size_t k,
              const Scheme &result_scheme);
TopK MakeTopK(std::vector<size_t> &&column_indices, size_t k,
              const Scheme &result_scheme,
              SortDirection direction = SortDirection::Descending);

struct GroupAggTask {
    AggType type;
    size_t column_index;
};

GroupAggTask MakeGroupAgg(AggType type, size_t column_index);
GroupAggTask MakeGroupSum(size_t column_index);
GroupAggTask MakeGroupMin(size_t column_index);
GroupAggTask MakeGroupMax(size_t column_index);
GroupAggTask MakeGroupCount(size_t column_index = 0);
GroupAggTask MakeGroupCountDistinct(size_t column_index);
GroupAggTask MakeGroupAvg(size_t column_index);

struct GroupByTask {
    std::vector<AggType> types_;
    std::vector<size_t> column_indices;
    size_t agg_column_index = 0;
    std::vector<size_t> agg_column_indices;
};

GroupByTask MakeGroupByTask(std::vector<size_t> &&group_column_indices,
                            std::vector<GroupAggTask> &&aggregations);
GroupByTask MakeGroupByTask(std::vector<size_t> &&group_column_indices,
                            AggType agg_type, size_t agg_column_index);

class GroupBy : public BlockingOperation {
  public:
    GroupBy(GroupByTask &&task, const Scheme &scheme);
    void Process(const Batch &batch) override;
    std::vector<Batch> Finalize() && override;

  private:
    std::unordered_map<std::string, std::vector<ResultAggVariant>> result_;
    GroupByTask task_;
    Scheme scheme_;
};

GroupBy MakeGroupBy(GroupByTask &&task, const Scheme &scheme);

class Offset : public BlockingOperation {
  public:
    Offset(size_t offset);
    void Process(const Batch &batch) override;
    std::vector<Batch> Finalize() && override;

  private:
    size_t offset_;
    std::vector<Batch> result_;
};

Offset MakeOffset(size_t offset);

struct ExpressionTask {
    std::string alias;
    ColumnTypes result_type;

    std::function<void(const Batch &, size_t, Column &)> eval;
};

class Expression : public StreamingOperation {
  public:
    explicit Expression(std::vector<ExpressionTask> &&tasks);

    void Execute(Batch &batch) override;

  private:
    std::vector<ExpressionTask> tasks_;
};

Expression MakeExpression(std::vector<ExpressionTask> &&tasks);

ExpressionTask MakeCopyColumnExpression(size_t column_index, std::string alias,
                                        ColumnTypes result_type);

ExpressionTask MakeConstantInt64Expression(int64_t value, std::string alias);
ExpressionTask MakeConstantStringExpression(std::string value,
                                            std::string alias);

ExpressionTask MakeAddInt64ConstantExpression(size_t column_index,
                                              int64_t value, std::string alias);
ExpressionTask MakeSubInt64ConstantExpression(size_t column_index,
                                              int64_t value, std::string alias);
ExpressionTask MakeInt128AddInt64ProductExpression(size_t int128_column_index,
                                                   size_t int64_column_index,
                                                   int64_t multiplier,
                                                   std::string alias);

ExpressionTask MakeLengthExpression(size_t column_index, std::string alias);
ExpressionTask MakeExtractMinuteExpression(size_t column_index,
                                           std::string alias);
ExpressionTask MakeDateTruncMinuteExpression(size_t column_index,
                                             std::string alias);

ExpressionTask MakeCaseWhenBothZeroThenStringElseEmptyExpression(
    size_t first_column_index, size_t second_column_index,
    size_t string_column_index, std::string alias);

ExpressionTask MakeExtractDomainExpression(size_t column_index,
                                           std::string alias);

class SelectAnswer : public StreamingOperation {
  public:
    SelectAnswer(std::vector<size_t> &&column_indices);
    void Execute(Batch &batch) override;

  private:
    std::vector<size_t> column_indices_;
};

SelectAnswer MakeSelectAnswer(std::vector<size_t> &&column_indices);
