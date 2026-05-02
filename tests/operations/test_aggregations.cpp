#include <memory>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "data_structures/Column.h"
#include "operations/Aggregations.h"

namespace {

std::shared_ptr<Column> MakeIntColumn(std::initializer_list<std::string> values) {
    auto column = std::make_shared<Int64Column>();
    for (const auto &value : values) {
        column->Push(value);
    }
    return column;
}

std::shared_ptr<Column> MakeStringColumn(std::initializer_list<std::string> values) {
    auto column = std::make_shared<StringColumn>();
    for (const auto &value : values) {
        column->Push(value);
    }
    return column;
}

} // namespace

TEST(AggregationTest, SumAndAvgOnInt64Column) {
    auto column = MakeIntColumn({"-10", "5", "3", "2"});

    const __int128 sum = agg::Sum<__int128>(column);
    EXPECT_EQ(static_cast<long long>(sum), 0LL);
    EXPECT_DOUBLE_EQ(agg::Avg(column), 0.0);
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

    EXPECT_EQ(agg::CountDistinct<int64_t>(int_column), 3U);
    EXPECT_EQ(agg::CountDistinct<std::string>(str_column), 3U);
}

TEST(AggregationTest, EmptyColumnThrowsForMinMaxAndAvg) {
    auto empty_ints = MakeIntColumn({});
    auto empty_strings = MakeStringColumn({});

    EXPECT_THROW((void)agg::Min<int64_t>(empty_ints), std::invalid_argument);
    EXPECT_THROW((void)agg::Max<int64_t>(empty_ints), std::invalid_argument);
    EXPECT_THROW((void)agg::Min<std::string>(empty_strings), std::invalid_argument);
    EXPECT_THROW((void)agg::Max<std::string>(empty_strings), std::invalid_argument);
    EXPECT_THROW((void)agg::Avg(empty_ints), std::invalid_argument);
}