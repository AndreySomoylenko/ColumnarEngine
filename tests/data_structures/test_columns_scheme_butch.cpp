#include <chrono>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "data_structures/Butch.h"
#include "data_structures/Column.h"
#include "data_structures/Scheme.h"

TEST(SchemeTest, AddsKnownAndUnknownColumnTypes) {
    Scheme scheme;

    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});
    scheme.Add({"created_at", "timestamp"});
    scheme.Add({"event_date", "date"});
    scheme.Add({"payload", "json"});

    EXPECT_EQ(scheme.GetSchemeNames(),
              (std::vector<std::string>{"id", "name", "created_at",
                                        "event_date", "payload"}));
    EXPECT_EQ(
        scheme.GetSchemeTypes(),
        (std::vector<ColumnTypes>{Int64, String, Timestamp, Date, Unknown}));
    EXPECT_EQ(scheme.GiveRaws(), (std::vector<Raw>{{"id", "int64"},
                                                   {"name", "string"},
                                                   {"created_at", "timestamp"},
                                                   {"event_date", "date"},
                                                   {"payload", "unknown"}}));
}

TEST(SchemeTest, RejectsMalformedRows) {
    Scheme scheme;

    EXPECT_THROW(scheme.Add({"only-name"}), std::invalid_argument);
}

TEST(SchemeTest, IgnoresExtraFieldsFromCsvSchemaFiles) {
    Scheme scheme;

    scheme.Add({"WatchID", "int64", ""});

    EXPECT_EQ(scheme.GetSchemeNames(), (std::vector<std::string>{"WatchID"}));
    EXPECT_EQ(scheme.GetSchemeTypes(), (std::vector<ColumnTypes>{Int64}));
}

TEST(SchemeTest, RemovesColumnAndRebuildsIndexes) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});
    scheme.Add({"score", "double"});

    scheme.RemoveColumn(1);

    EXPECT_EQ(scheme.GetSchemeNames(), (std::vector<std::string>{"id", "score"}));
    EXPECT_EQ(scheme.GetSchemeTypes(), (std::vector<ColumnTypes>{Int64, Double}));
    EXPECT_EQ(scheme.GetColumnIndexByName("score"), 1U);
    EXPECT_EQ(scheme.GetColumnTypeByName("score"), Double);
    EXPECT_THROW(scheme.GetColumnIndexByName("name"), std::invalid_argument);
    EXPECT_THROW(scheme.RemoveColumn(2), std::out_of_range);
}

TEST(ColumnTest, Int64ColumnStoresSignedIntegersAndRejectsBadIndex) {
    Int64Column column;

    column.Push("-42");
    column.Push("17");

    EXPECT_EQ(column.Size(), 2U);
    EXPECT_EQ(column.ToString(0), "-42");
    EXPECT_EQ(column.ToString(1), "17");
    EXPECT_EQ(column.ToWrite().second, 16U);
    EXPECT_EQ(column.GetColumnType(), Int64);
    EXPECT_THROW(column.GetOffsets(), std::logic_error);
    EXPECT_THROW(column.ToString(2), std::invalid_argument);
}

TEST(ColumnTest, StringColumnPreservesBoundariesAndOffsets) {
    StringColumn column;

    column.Push("abc");
    column.Push("de");
    column.Push("");

    EXPECT_EQ(column.Size(), 3U);
    EXPECT_EQ(column.ToString(0), "abc");
    EXPECT_EQ(column.ToString(1), "de");
    EXPECT_EQ(column.ToString(2), "");
    EXPECT_EQ(column.GetOffsets(), (std::vector<size_t>{0U, 3U, 5U}));
    EXPECT_EQ(column.GetColumnType(), String);
    EXPECT_THROW(column.ToString(3), std::invalid_argument);
}

TEST(ColumnTest, TimeColumnStoresTimestampsAndRejectsInvalidValues) {
    TimeColumn column;

    column.Push("2013-07-14 20:38:47");
    column.Push("1971-01-01T14:16:06");

    EXPECT_EQ(column.Size(), 2U);
    EXPECT_EQ(column.ToString(0), "2013-07-14 20:38:47");
    EXPECT_EQ(column.ToString(1), "1971-01-01 14:16:06");
    EXPECT_EQ(column.ToWrite().second,
              2U * sizeof(std::chrono::system_clock::time_point));
    EXPECT_EQ(column.GetColumnType(), Timestamp);
    EXPECT_THROW(column.GetOffsets(), std::logic_error);
    EXPECT_THROW(column.ToString(2), std::invalid_argument);
    EXPECT_THROW(column.Push("2013-02-29 20:38:47"), std::invalid_argument);
    EXPECT_THROW(column.Push("2013-07-14 24:00:00"), std::invalid_argument);
    EXPECT_THROW(column.Push("2013-07-14"), std::invalid_argument);
}

TEST(ColumnTest, TimeColumnStoresDatesAtMidnightAndFormatsAsDate) {
    TimeColumn column(true);

    column.Push("2013-07-15");
    column.Push("2020-02-29");

    EXPECT_EQ(column.Size(), 2U);
    EXPECT_EQ(column.ToString(0), "2013-07-15");
    EXPECT_EQ(column.ToString(1), "2020-02-29");
    EXPECT_EQ(column.ToWrite().second,
              2U * sizeof(std::chrono::system_clock::time_point));
    EXPECT_EQ(column.GetColumnType(), Date);
    EXPECT_THROW(column.GetOffsets(), std::logic_error);
    EXPECT_THROW(column.Push("2013-02-29"), std::invalid_argument);
    EXPECT_THROW(column.Push("2013-07-15 00:00:00"), std::invalid_argument);
}

TEST(ColumnTest, ClearResetsColumnContents) {
    Int64Column int_column;
    StringColumn string_column;

    int_column.Push("1");
    string_column.Push("abc");
    int_column.Clear();
    string_column.Clear();

    EXPECT_EQ(int_column.Size(), 0U);
    EXPECT_EQ(string_column.Size(), 0U);
    EXPECT_TRUE(string_column.GetOffsets().empty());

    string_column.Push("xy");
    EXPECT_EQ(string_column.ToString(0), "xy");
    EXPECT_EQ(string_column.GetOffsets(), (std::vector<size_t>{0U}));
}

TEST(ColumnTest, ConstantConstructorsRepeatRawValue) {
    int64_t int_value = 42;
    Int64Column int_column(reinterpret_cast<const char *>(&int_value),
                           sizeof(int_value), 3);

    EXPECT_EQ(int_column.Size(), 3U);
    EXPECT_EQ(int_column.ToString(0), "42");
    EXPECT_EQ(int_column.ToString(2), "42");

    StringColumn string_column("xy", 2, 2);
    EXPECT_EQ(string_column.Size(), 2U);
    EXPECT_EQ(string_column.ToString(0), "xy");
    EXPECT_EQ(string_column.ToString(1), "xy");
}

TEST(ButchTest, BuildsColumnsFromSchemeAndReturnsRows) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});
    scheme.Add({"created_at", "timestamp"});
    scheme.Add({"event_date", "date"});
    scheme.Add({"note", "unknown"});

    Butch butch(scheme, false);
    butch.AddRaw(
        {"1", "Alice", "2013-07-14 20:38:47", "2013-07-15", "payload"});
    butch.AddRaw({"2", "Bob", "1971-01-01 14:16:06", "1971-01-01", ""});

    EXPECT_TRUE(butch.EnableToPush());
    EXPECT_EQ(butch.HorizontalSize(), 5U);
    EXPECT_EQ(butch.VerticalSize(), 2U);
    EXPECT_EQ(butch.GetRaw(0), (Raw{"1", "Alice", "2013-07-14 20:38:47",
                                    "2013-07-15", "payload"}));
    EXPECT_EQ(butch.GetRaw(1),
              (Raw{"2", "Bob", "1971-01-01 14:16:06", "1971-01-01", ""}));
    EXPECT_EQ(butch.GetColumns()[0]->GetColumnType(), Int64);
    EXPECT_EQ(butch.GetColumns()[1]->GetColumnType(), String);
    EXPECT_EQ(butch.GetColumns()[2]->GetColumnType(), Timestamp);
    EXPECT_EQ(butch.GetColumns()[3]->GetColumnType(), Date);
    EXPECT_EQ(butch.GetColumns()[4]->GetColumnType(), String);
}

TEST(ButchTest, RejectsRowsWithWrongWidthAndCanAddToSingleColumn) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});

    Butch butch(scheme, false);

    EXPECT_THROW(butch.AddRaw({"1"}), std::invalid_argument);

    butch.AddToColumn("7", 0);
    butch.AddToColumn("Alice", 1);
    EXPECT_EQ(butch.GetRaw(0), (Raw{"7", "Alice"}));

    butch.Clear();
    EXPECT_EQ(butch.VerticalSize(), 0U);
}

TEST(ButchTest, CanAppendReadyColumnAndExtendScheme) {
    Scheme scheme;
    scheme.Add({"id", "int64"});

    Butch butch(scheme, false);
    butch.AddRaw({"1"});
    butch.AddRaw({"2"});

    int64_t value = 7;
    auto constant = std::make_shared<Int64Column>(
        reinterpret_cast<const char *>(&value), sizeof(value),
        butch.VerticalSize());

    butch.AddColumn(constant, ColumnTypes::Int64, "constant");

    EXPECT_EQ(butch.HorizontalSize(), 2U);
    EXPECT_EQ(butch.GetScheme().GetSchemeNames(),
              (std::vector<std::string>{"id", "constant"}));
    EXPECT_EQ(butch.GetRaw(0), (Raw{"1", "7"}));
    EXPECT_EQ(butch.GetRaw(1), (Raw{"2", "7"}));

    auto wrong_height = std::make_shared<Int64Column>(
        reinterpret_cast<const char *>(&value), sizeof(value), 1);
    EXPECT_THROW(butch.AddColumn(wrong_height, ColumnTypes::Int64, "bad"),
                 std::invalid_argument);
}

TEST(ButchTest, RemovesColumnAndKeepsSchemeInSync) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});
    scheme.Add({"score", "double"});

    Butch butch(scheme, false);
    butch.AddRaw({"1", "Alice", "10.5"});
    butch.AddRaw({"2", "Bob", "20.25"});

    butch.RemoveColumn(1);

    EXPECT_EQ(butch.HorizontalSize(), 2U);
    EXPECT_EQ(butch.VerticalSize(), 2U);
    EXPECT_EQ(butch.GetScheme().GetSchemeNames(),
              (std::vector<std::string>{"id", "score"}));
    EXPECT_EQ(butch.GetScheme().GetColumnIndexByName("score"), 1U);
    EXPECT_EQ(butch.GetRaw(0), (Raw{"1", "10.500000"}));
    EXPECT_EQ(butch.GetRaw(1), (Raw{"2", "20.250000"}));
    EXPECT_THROW(butch.GetScheme().GetColumnIndexByName("name"),
                 std::invalid_argument);
    EXPECT_THROW(butch.RemoveColumn(2), std::out_of_range);
}
