#include <memory>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include <gtest/gtest.h>

#include "data_structures/Column.h"
#include "data_structures/Batch.h"
#include "operations/Aggregations.h"
#include "operations/Operations.h"

namespace {

std::shared_ptr<Column>
MakeIntColumn(std::initializer_list<std::string> values) {
    auto column = std::make_shared<Int64Column>();
    for (const auto &value : values) {
        column->Push(value);
    }
    return column;
}

std::shared_ptr<Column>
MakeStringColumn(std::initializer_list<std::string> values) {
    auto column = std::make_shared<StringColumn>();
    for (const auto &value : values) {
        column->Push(value);
    }
    return column;
}

} // namespace

std::vector<Row> ReadRows(std::vector<Batch> batches) {
    std::vector<Row> rows;
    for (auto &batch : batches) {
        for (size_t row = 0; row < batch.VerticalSize(); ++row) {
            rows.push_back(batch.GetRow(row));
        }
    }
    return rows;
}

std::map<std::string, Row> RowsByFirstColumn(std::vector<Batch> batches) {
    std::map<std::string, Row> rows;
    for (const Row &row : ReadRows(std::move(batches))) {
        rows[row[0]] = row;
    }
    return rows;
}

TEST(AggregationTest, SumAndAvgOnInt64Column) {
    auto column = MakeIntColumn({"-10", "5", "3", "2"});

    const __int128 sum = agg::Sum<__int128>(column);
    EXPECT_EQ(static_cast<long long>(sum), 0LL);
    EXPECT_DOUBLE_EQ(agg::Avg<__int128>(column), 0.0);
}

TEST(AggregationTest, MinMaxOnInt64Column) {
    auto column = MakeIntColumn({"17", "-42", "8", "8"});

    EXPECT_EQ(agg::Min<int64_t>(column), -42);
    EXPECT_EQ(agg::Max<int64_t>(column), 17);
}

TEST(AggregationTest, MinMaxOnStringColumn) {
    auto column = MakeStringColumn({"zeta", "alpha", "beta", ""});

    EXPECT_EQ(agg::Min<std::string>(column), "");
    EXPECT_EQ(agg::Max<std::string>(column), "zeta");
}

TEST(AggregationTest, CountDistinctOnSupportedTypes) {
    auto int_column = MakeIntColumn({"1", "2", "2", "3", "1"});
    auto str_column = MakeStringColumn({"foo", "bar", "foo", ""});

    std::unordered_set<int64_t> int_values;
    std::unordered_set<std::string> str_values;

    agg::CountDistinct(int_column, int_values);
    agg::CountDistinct(str_column, str_values);

    EXPECT_EQ(int_values.size(), 3U);
    EXPECT_EQ(str_values.size(), 3U);
}

TEST(AggregationTest, EmptyColumnThrowsForMinMaxAndAvg) {
    auto empty_ints = MakeIntColumn({});
    auto empty_strings = MakeStringColumn({});

    EXPECT_THROW((void)agg::Min<int64_t>(empty_ints), std::invalid_argument);
    EXPECT_THROW((void)agg::Max<int64_t>(empty_ints), std::invalid_argument);
    EXPECT_THROW((void)agg::Min<std::string>(empty_strings),
                 std::invalid_argument);
    EXPECT_THROW((void)agg::Max<std::string>(empty_strings),
                 std::invalid_argument);
    EXPECT_THROW((void)agg::Avg<__int128>(empty_ints), std::invalid_argument);
}

TEST(AggregationTest, AvgThrowsForEmptyEnabledSelection) {
    auto column = MakeIntColumn({"1", "2"});
    EnabledColumns selected = std::unordered_set<size_t>{};

    EXPECT_THROW((void)agg::Avg<__int128>(column, selected),
                 std::invalid_argument);
}

TEST(AggregationOperationTest, ComputesMultipleAggregatesAcrossBatches) {
    Scheme scheme;
    scheme.Add({"region", "int64"});
    scheme.Add({"width", "int64"});
    scheme.Add({"phrase", "string"});

    Batch first(scheme, false);
    first.AddRow({"1", "10", "a"});
    first.AddRow({"2", "20", "b"});

    Batch second(scheme, false);
    second.AddRow({"3", "30", "a"});
    second.AddRow({"4", "40", "c"});

    Aggregation aggregation({AggTask{1, AggType::Sum, ColumnTypes::Int128,
                                     "sum_width"},
                             AggTask{1, AggType::Avg, ColumnTypes::Double,
                                     "avg_width"},
                             AggTask{0, AggType::Min, ColumnTypes::Int64,
                                     "min_region"},
                             AggTask{0, AggType::Max, ColumnTypes::Int64,
                                     "max_region"},
                             AggTask{0, AggType::Count, ColumnTypes::Int64,
                                     "count_rows"},
                             AggTask{2, AggType::CountDistinct,
                                     ColumnTypes::Int64,
                                     "distinct_phrases"}});

    aggregation.Process(first);
    aggregation.Process(second);

    std::vector<Row> rows = ReadRows(std::move(aggregation).Finalize());

    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(rows[0], (Row{"100", "25.000000", "1", "4", "4", "3"}));
}

TEST(AggregationOperationTest, RespectsEnabledRowsFromFilter) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"value", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "10"});
    batch.AddRow({"2", "20"});
    batch.AddRow({"3", "30"});

    Filter filter({FilterTask{
        0, [](const char *data, size_t) {
            int64_t value;
            std::memcpy(&value, data, sizeof(value));
            return value >= 2;
        }}});

    Aggregation aggregation({AggTask{1, AggType::Sum, ColumnTypes::Int128,
                                     "sum_value"},
                             AggTask{1, AggType::Count, ColumnTypes::Int64,
                                     "count_rows"}});

    filter.Execute(batch);
    aggregation.Process(batch);
    std::vector<Row> rows = ReadRows(std::move(aggregation).Finalize());

    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(rows[0], (Row{"50", "2"}));
}

TEST(AggregationOperationTest, AvgThrowsWhenFilterRemovesAllRows) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"value", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "10"});
    batch.AddRow({"2", "20"});

    auto filter = MakeFilter({MakeInt64GreaterFilter(0, 100)});
    filter.Execute(batch);

    auto aggregation = MakeAggregation({MakeAvgAgg(1, "avg_value")});
    aggregation.Process(batch);

    EXPECT_THROW((void)std::move(aggregation).Finalize(),
                 std::invalid_argument);
}

TEST(FilterTest, SelectsRowsFromUnfilteredBatch) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "Alice"});
    batch.AddRow({"2", "Bob"});
    batch.AddRow({"3", "Carol"});

    Filter filter({FilterTask{
        0, [](const char *data, size_t size) {
            EXPECT_EQ(size, sizeof(int64_t));
            int64_t value;
            std::memcpy(&value, data, sizeof(value));
            return value >= 2;
        }}});

    filter.Execute(batch);

    ASSERT_TRUE(batch.GetEnabledColumns().has_value());
    EXPECT_FALSE(batch.IsRowEnabled(0));
    EXPECT_TRUE(batch.IsRowEnabled(1));
    EXPECT_TRUE(batch.IsRowEnabled(2));
    EXPECT_EQ(batch.GetRow(1), (Row{"2", "Bob"}));
}

TEST(FilterTest, IntersectsMultipleConditionsWithoutDroppingColumns) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "Alice"});
    batch.AddRow({"2", "Bob"});
    batch.AddRow({"3", "Bob"});

    Filter filter({FilterTask{
                       0, [](const char *data, size_t) {
                           int64_t value;
                           std::memcpy(&value, data, sizeof(value));
                           return value >= 2;
                       }},
                   FilterTask{
                       1, [](const char *data, size_t size) {
                           return std::string(data, size) == "Bob";
                       }}});

    filter.Execute(batch);

    ASSERT_TRUE(batch.GetEnabledColumns().has_value());
    EXPECT_FALSE(batch.IsRowEnabled(0));
    EXPECT_TRUE(batch.IsRowEnabled(1));
    EXPECT_TRUE(batch.IsRowEnabled(2));
    EXPECT_EQ(batch.GetRow(2), (Row{"3", "Bob"}));
}

TEST(FilterTest, CanSelectNoRows) {
    Scheme scheme;
    scheme.Add({"id", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"1"});
    batch.AddRow({"2"});

    Filter filter({FilterTask{
        0, [](const char *, size_t) { return false; }}});

    filter.Execute(batch);

    ASSERT_TRUE(batch.GetEnabledColumns().has_value());
    EXPECT_FALSE(batch.IsRowEnabled(0));
    EXPECT_FALSE(batch.IsRowEnabled(1));
}

TEST(FilterBuilderTest, SupportsComparisonsInLikeRegexAndDateRanges) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"url", "string"});
    scheme.Add({"event_date", "date"});
    scheme.Add({"phrase", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "https://google.com/search", "2013-07-01", "hello"});
    batch.AddRow({"2", "https://ya.ru", "2013-07-15", ""});
    batch.AddRow({"3", "https://maps.google.test", "2013-08-01", "maps"});
    batch.AddRow({"4", "https://example.com", "2013-07-31", "hello"});

    auto filter = MakeFilter({
        MakeInt64InFilter(0, {1, 2, 4}),
        MakeInt64NotEqualFilter(0, 2),
        MakeStringLikeFilter(1, "%google%"),
        MakeStringNotLikeFilter(1, "%.google.%"),
        MakeDateRangeFilter(2, "2013-07-01", "2013-07-31"),
        MakeStringNotEqualFilter(3, ""),
        MakeStringRegexFilter(3, "^he.*o$"),
    });
    filter.Execute(batch);

    EXPECT_TRUE(batch.IsRowEnabled(0));
    EXPECT_FALSE(batch.IsRowEnabled(1));
    EXPECT_FALSE(batch.IsRowEnabled(2));
    EXPECT_FALSE(batch.IsRowEnabled(3));
}

TEST(FilterBuilderTest, Int64FiltersAcceptNarrowIntegerColumns) {
    Scheme scheme;
    scheme.Add({"flag", "int16"});
    scheme.Add({"counter", "int32"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "62"});
    batch.AddRow({"0", "62"});
    batch.AddRow({"1", "7"});

    auto filter = MakeFilter({
        MakeInt64NotEqualFilter(0, 0),
        MakeInt64EqualFilter(1, 62),
    });
    filter.Execute(batch);

    EXPECT_TRUE(batch.IsRowEnabled(0));
    EXPECT_FALSE(batch.IsRowEnabled(1));
    EXPECT_FALSE(batch.IsRowEnabled(2));
}

TEST(TopKTest, SupportsDescendingOrder) {
    Scheme scheme;
    scheme.Add({"score", "int64"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "low"});
    batch.AddRow({"3", "high"});
    batch.AddRow({"2", "mid"});

    TopK top_k({SortKey{0, SortDirection::Descending}}, 2, scheme);
    top_k.Process(batch);

    std::vector<Batch> result = std::move(top_k).Finalize();

    ASSERT_EQ(result.size(), 1U);
    ASSERT_EQ(result[0].VerticalSize(), 2U);
    EXPECT_EQ(result[0].GetRow(0), (Row{"3", "high"}));
    EXPECT_EQ(result[0].GetRow(1), (Row{"2", "mid"}));
}

TEST(TopKTest, SupportsAscendingOrderAndKeepsDuplicateKeys) {
    Scheme scheme;
    scheme.Add({"score", "int64"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"2", "two"});
    batch.AddRow({"1", "one-a"});
    batch.AddRow({"1", "one-b"});
    batch.AddRow({"3", "three"});

    TopK top_k({SortKey{0, SortDirection::Ascending}}, 3, scheme);
    top_k.Process(batch);

    std::vector<Batch> result = std::move(top_k).Finalize();

    ASSERT_EQ(result.size(), 1U);
    ASSERT_EQ(result[0].VerticalSize(), 3U);
    EXPECT_EQ(result[0].GetRow(0), (Row{"1", "one-a"}));
    EXPECT_EQ(result[0].GetRow(1), (Row{"1", "one-b"}));
    EXPECT_EQ(result[0].GetRow(2), (Row{"2", "two"}));
}

TEST(TopKTest, SupportsMixedDirectionSortKeys) {
    Scheme scheme;
    scheme.Add({"group", "int64"});
    scheme.Add({"score", "int64"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "10", "a"});
    batch.AddRow({"1", "20", "b"});
    batch.AddRow({"2", "15", "c"});
    batch.AddRow({"2", "30", "d"});

    TopK top_k({SortKey{0, SortDirection::Ascending},
                SortKey{1, SortDirection::Descending}},
               3, scheme);
    top_k.Process(batch);

    std::vector<Batch> result = std::move(top_k).Finalize();

    ASSERT_EQ(result.size(), 1U);
    ASSERT_EQ(result[0].VerticalSize(), 3U);
    EXPECT_EQ(result[0].GetRow(0), (Row{"1", "20", "b"}));
    EXPECT_EQ(result[0].GetRow(1), (Row{"1", "10", "a"}));
    EXPECT_EQ(result[0].GetRow(2), (Row{"2", "30", "d"}));
}

TEST(TopKTest, RespectsEnabledRowsAndReturnsAllRowsWhenKIsLarge) {
    Scheme scheme;
    scheme.Add({"score", "int64"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "hidden"});
    batch.AddRow({"3", "top"});
    batch.AddRow({"2", "mid"});

    Filter filter({FilterTask{
        1, [](const char *data, size_t size) {
            return std::string(data, size) != "hidden";
        }}});

    TopK top_k({SortKey{0, SortDirection::Descending}}, 10, scheme);
    filter.Execute(batch);
    top_k.Process(batch);

    std::vector<Batch> result = std::move(top_k).Finalize();

    ASSERT_EQ(result.size(), 1U);
    ASSERT_EQ(result[0].VerticalSize(), 2U);
    EXPECT_EQ(result[0].GetRow(0), (Row{"3", "top"}));
    EXPECT_EQ(result[0].GetRow(1), (Row{"2", "mid"}));
}

TEST(GroupByTest, CountsRowsByStringKey) {
    Scheme scheme;
    scheme.Add({"phrase", "string"});
    scheme.Add({"user_id", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"search", "1"});
    batch.AddRow({"mail", "2"});
    batch.AddRow({"search", "3"});

    GroupBy group_by({{AggType::Count}, {0}}, scheme);
    group_by.Process(batch);

    std::map<std::string, std::string> actual;
    for (const Row &row : ReadRows(std::move(group_by).Finalize())) {
        actual[row[0]] = row[1];
    }

    EXPECT_EQ(actual,
              (std::map<std::string, std::string>{{"mail", "1"},
                                                  {"search", "2"}}));
}

TEST(GroupByTest, SumsNumericColumnByIntKey) {
    Scheme scheme;
    scheme.Add({"region", "int64"});
    scheme.Add({"adv_engine", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "10"});
    batch.AddRow({"2", "7"});
    batch.AddRow({"1", "5"});

    GroupBy group_by({{AggType::Sum}, {0}, 1}, scheme);
    group_by.Process(batch);

    std::map<std::string, std::string> actual;
    for (const Row &row : ReadRows(std::move(group_by).Finalize())) {
        actual[row[0]] = row[1];
    }

    EXPECT_EQ(actual,
              (std::map<std::string, std::string>{{"1", "15"}, {"2", "7"}}));
}

TEST(GroupByTest, ComputesMultipleAggregatesPerGroup) {
    Scheme scheme;
    scheme.Add({"region", "int64"});
    scheme.Add({"adv_engine", "int64"});
    scheme.Add({"width", "int64"});
    scheme.Add({"user_id", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "10", "100", "7"});
    batch.AddRow({"1", "5", "300", "7"});
    batch.AddRow({"1", "1", "200", "9"});
    batch.AddRow({"2", "8", "400", "7"});

    GroupBy group_by({{AggType::Sum, AggType::Count, AggType::Avg,
                       AggType::CountDistinct},
                      {0},
                      0,
                      {1, 0, 2, 3}},
                     scheme);
    group_by.Process(batch);

    auto rows = RowsByFirstColumn(std::move(group_by).Finalize());

    ASSERT_EQ(rows.size(), 2U);
    EXPECT_EQ(rows["1"], (Row{"1", "16", "3", "200.000000", "2"}));
    EXPECT_EQ(rows["2"], (Row{"2", "8", "1", "400.000000", "1"}));
}

TEST(GroupByTest, AveragesAndKeepsDistinctStringKeyBoundaries) {
    Scheme scheme;
    scheme.Add({"lhs", "string"});
    scheme.Add({"rhs", "string"});
    scheme.Add({"value", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"ab", "c", "10"});
    batch.AddRow({"a", "bc", "20"});
    batch.AddRow({"ab", "c", "30"});

    GroupBy group_by({{AggType::Avg}, {0, 1}, 2}, scheme);
    group_by.Process(batch);

    std::map<std::pair<std::string, std::string>, std::string> actual;
    for (const Row &row : ReadRows(std::move(group_by).Finalize())) {
        actual[{row[0], row[1]}] = row[2];
    }

    EXPECT_EQ(actual[std::make_pair(std::string("ab"), std::string("c"))],
              "20.000000");
    EXPECT_EQ(actual[std::make_pair(std::string("a"), std::string("bc"))],
              "20.000000");
}

TEST(GroupByTest, CountsDistinctValuesPerGroup) {
    Scheme scheme;
    scheme.Add({"region", "int64"});
    scheme.Add({"user_id", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "10"});
    batch.AddRow({"1", "10"});
    batch.AddRow({"1", "20"});
    batch.AddRow({"2", "10"});

    GroupBy group_by({{AggType::CountDistinct}, {0}, 1}, scheme);
    group_by.Process(batch);

    std::map<std::string, std::string> actual;
    for (const Row &row : ReadRows(std::move(group_by).Finalize())) {
        actual[row[0]] = row[1];
    }

    EXPECT_EQ(actual,
              (std::map<std::string, std::string>{{"1", "2"}, {"2", "1"}}));
}

TEST(GroupByTest, ComputesMinAndMaxForStringAndNumericValues) {
    Scheme scheme;
    scheme.Add({"region", "int64"});
    scheme.Add({"url", "string"});
    scheme.Add({"score", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "zeta", "10"});
    batch.AddRow({"1", "alpha", "30"});
    batch.AddRow({"2", "beta", "20"});

    GroupBy min_url({{AggType::Min}, {0}, 1}, scheme);
    min_url.Process(batch);
    auto min_rows = RowsByFirstColumn(std::move(min_url).Finalize());

    EXPECT_EQ(min_rows["1"], (Row{"1", "alpha"}));
    EXPECT_EQ(min_rows["2"], (Row{"2", "beta"}));

    GroupBy max_score({{AggType::Max}, {0}, 2}, scheme);
    max_score.Process(batch);
    auto max_rows = RowsByFirstColumn(std::move(max_score).Finalize());

    EXPECT_EQ(max_rows["1"], (Row{"1", "30"}));
    EXPECT_EQ(max_rows["2"], (Row{"2", "20"}));
}

TEST(GroupByTest, RespectsEnabledRowsAcrossMultipleBatches) {
    Scheme scheme;
    scheme.Add({"region", "int64"});
    scheme.Add({"value", "int64"});

    Batch first(scheme, false);
    first.AddRow({"1", "10"});
    first.AddRow({"2", "20"});

    Batch second(scheme, false);
    second.AddRow({"1", "30"});
    second.AddRow({"2", "40"});

    auto keep_region_one = [](const char *data, size_t) {
        int64_t value;
        std::memcpy(&value, data, sizeof(value));
        return value == 1;
    };

    Filter filter({FilterTask{0, keep_region_one}});
    GroupBy group_by({{AggType::Sum}, {0}, 1}, scheme);

    filter.Execute(first);
    group_by.Process(first);
    filter.Execute(second);
    group_by.Process(second);

    std::vector<Row> rows = ReadRows(std::move(group_by).Finalize());

    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(rows[0], (Row{"1", "40"}));
}

TEST(OffsetTest, SkipsRowsAcrossBatchesAndHandlesEmptyResults) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});

    Batch first(scheme, false);
    first.AddRow({"1", "a"});
    first.AddRow({"2", "b"});

    Batch second(scheme, false);
    second.AddRow({"3", "c"});
    second.AddRow({"4", "d"});

    Offset offset(3);
    offset.Process(first);
    offset.Process(second);

    std::vector<Row> rows = ReadRows(std::move(offset).Finalize());

    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(rows[0], (Row{"4", "d"}));

    Offset too_large_offset(10);
    too_large_offset.Process(second);
    EXPECT_TRUE(std::move(too_large_offset).Finalize().empty());
}

TEST(OffsetTest, RespectsEnabledRowsInOriginalOrder) {
    Scheme scheme;
    scheme.Add({"id", "int64"});

    Batch batch(scheme, false);
    batch.AddRow({"1"});
    batch.AddRow({"2"});
    batch.AddRow({"3"});
    batch.AddRow({"4"});

    auto filter = MakeFilter({MakeInt64Filter(
        0, [](int64_t value) { return value == 2 || value == 4; })});
    filter.Execute(batch);

    auto offset = MakeOffset(1);
    offset.Process(batch);
    auto rows = ReadRows(std::move(offset).Finalize());

    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(rows[0], (Row{"4"}));
}

TEST(ExpressionTest, AddsComputedColumnsAndKeepsOriginalRows) {
    Scheme scheme;
    scheme.Add({"width", "int64"});
    scheme.Add({"url", "string"});
    scheme.Add({"event_time", "timestamp"});
    scheme.Add({"search_engine", "int64"});
    scheme.Add({"adv_engine", "int64"});
    scheme.Add({"referer", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"100", "https://www.example.com/path",
                  "2013-07-14 20:38:47", "0", "0",
                  "https://www.ref.example/search"});
    batch.AddRow({"200", "http://other.test/a", "2013-07-14 20:05:11",
                  "1", "0", "https://ignored.test/path"});

    Expression expression(
        {MakeAddInt64ConstantExpression(0, 5, "width_plus_5"),
         MakeSubInt64ConstantExpression(0, 10, "width_minus_10"),
         MakeLengthExpression(1, "url_len"),
         MakeExtractMinuteExpression(2, "minute"),
         MakeDateTruncMinuteExpression(2, "minute_ts"),
         MakeConstantInt64Expression(1, "one"),
         MakeConstantStringExpression("", "empty"),
         MakeCaseWhenBothZeroThenStringElseEmptyExpression(3, 4, 5, "src"),
         MakeExtractDomainExpression(1, "domain")});

    expression.Execute(batch);
    Batch &result = batch;

    ASSERT_EQ(result.VerticalSize(), 2U);
    EXPECT_EQ(result.HorizontalSize(), 15U);
    EXPECT_EQ(result.GetScheme().GetSchemeNames()[6], "width_plus_5");
    EXPECT_EQ(result.GetScheme().GetSchemeNames()[14], "domain");

    EXPECT_EQ(result.GetRow(0),
              (Row{"100",
                   "https://www.example.com/path",
                   "2013-07-14 20:38:47",
                   "0",
                   "0",
                   "https://www.ref.example/search",
                   "105",
                   "90",
                   "28",
                   "38",
                   "2013-07-14 20:38:00",
                   "1",
                   "",
                   "https://www.ref.example/search",
                   "example.com"}));
    EXPECT_EQ(result.GetRow(1)[13], "");
    EXPECT_EQ(result.GetRow(1)[14], "other.test");
}

TEST(ExpressionTest, AppendsComputedColumnsAndRespectsFilter) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "drop"});
    batch.AddRow({"2", "keep"});

    Filter filter({FilterTask{
        1, [](const char *data, size_t size) {
            return std::string(data, size) == "keep";
        }}});

    Expression expression({MakeConstantStringExpression("x", "marker"),
                           MakeCopyColumnExpression(0, "id_copy",
                                                    ColumnTypes::Int64)});

    filter.Execute(batch);
    expression.Execute(batch);

    ASSERT_EQ(batch.VerticalSize(), 2U);
    EXPECT_EQ(batch.HorizontalSize(), 4U);
    EXPECT_FALSE(batch.IsRowEnabled(0));
    EXPECT_TRUE(batch.IsRowEnabled(1));
    EXPECT_EQ(batch.GetScheme().GetSchemeNames(),
              (std::vector<std::string>{"id", "name", "marker", "id_copy"}));
    EXPECT_EQ(batch.GetRow(1), (Row{"2", "keep", "x", "2"}));
}

TEST(OperationBuilderTest, BuildsOperationsWithReadableHelpers) {
    Scheme scheme;
    scheme.Add({"region", "int64"});
    scheme.Add({"value", "int64"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "10", "drop"});
    batch.AddRow({"1", "20", "keep"});
    batch.AddRow({"2", "30", "keep"});

    auto filter = MakeFilter(
        {MakeInt64Filter(1, [](int64_t value) { return value >= 20; }),
         MakeStringFilter(2, [](std::string_view value) {
             return value == "keep";
         })});
    filter.Execute(batch);

    auto aggregation = MakeAggregation(
        {MakeSumAgg(1, "sum_value"), MakeCountAgg(0, "rows")});
    aggregation.Process(batch);
    EXPECT_EQ(ReadRows(std::move(aggregation).Finalize())[0],
              (Row{"50", "2"}));

    auto group_by = MakeGroupBy(
        MakeGroupByTask({0}, {MakeGroupSum(1), MakeGroupCount(0)}), scheme);
    group_by.Process(batch);
    auto grouped_rows = RowsByFirstColumn(std::move(group_by).Finalize());
    EXPECT_EQ(grouped_rows["1"], (Row{"1", "20", "1"}));
    EXPECT_EQ(grouped_rows["2"], (Row{"2", "30", "1"}));

    auto top_k = MakeTopK({MakeDescendingSortKey(1)}, 1, scheme);
    top_k.Process(batch);
    EXPECT_EQ(ReadRows(std::move(top_k).Finalize())[0],
              (Row{"2", "30", "keep"}));

    auto expression = MakeExpression({MakeLengthExpression(2, "name_len")});
    expression.Execute(batch);
    EXPECT_EQ(batch.GetRow(1), (Row{"1", "20", "keep", "4"}));

    auto select_answer = MakeSelectAnswer({0, 1, 3});
    select_answer.Execute(batch);
    EXPECT_EQ(batch.GetScheme().GetSchemeNames(),
              (std::vector<std::string>{"region", "value", "name_len"}));
    EXPECT_EQ(batch.GetRow(1), (Row{"1", "20", "4"}));

    Scheme offset_scheme;
    offset_scheme.Add({"id", "int64"});
    Batch offset_input(offset_scheme, false);
    offset_input.AddRow({"1"});
    offset_input.AddRow({"2"});
    auto offset = MakeOffset(1);
    offset.Process(offset_input);
    EXPECT_EQ(ReadRows(std::move(offset).Finalize())[0], (Row{"2"}));
}
