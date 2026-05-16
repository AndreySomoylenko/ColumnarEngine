#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "core/Engine.h"
#include "core/Pipeline.h"
#include "io/CsvReader.h"
#include "operations/Operations.h"

namespace {

class EngineTest : public ::testing::Test {
  protected:
    void SetUp() override {
        original_path_ = std::filesystem::current_path();
        temp_dir_ = std::filesystem::temp_directory_path() /
                    "columnar_engine_core_tests";
        std::filesystem::remove_all(temp_dir_);
        std::filesystem::create_directories(temp_dir_);
        std::filesystem::current_path(temp_dir_);
    }

    void TearDown() override {
        std::filesystem::current_path(original_path_);
        std::error_code ec;
        std::filesystem::remove_all(temp_dir_, ec);
    }

    void WriteText(const std::string &name, const std::string &text) const {
        std::ofstream out(name);
        out << text;
    }

    std::vector<Row> ReadCsv(const std::string &name) const {
        CsvReader reader(name);
        std::vector<Row> rows;
        while (!reader.IsEnd()) {
            Row row;
            reader.ReadNext(row);
            if (!row.empty()) {
                rows.push_back(row);
            }
        }
        return rows;
    }

    std::filesystem::path original_path_;
    std::filesystem::path temp_dir_;
};

} // namespace

TEST_F(EngineTest, ConvertsCsvToColumnarAndExportsAllRowsAndScheme) {
    WriteText("scheme.csv",
              "\"id\",\"int64\"\n\"name\",\"string\"\n\"created_at\","
              "\"timestamp\"\n\"event_date\",\"date\"\n");
    WriteText("data.csv",
              "\"1\",\"Alice\",\"2013-07-14 20:38:47\",\"2013-07-15\"\n"
              "\"2\",\"Bob, Jr.\",\"1971-01-01 14:16:06\",\"1971-01-01\"\n"
              "\"3\",\"he said \"\"hi\"\"\",\"2020-02-29 00:00:00\","
              "\"2020-02-29\"\n");

    Engine engine("data.csv", "scheme.csv", "data.hub");
    engine.TakeAll("out.csv");

    EXPECT_EQ(
        ReadCsv("out.csv"),
        (std::vector<Row>{
            {"1", "Alice", "2013-07-14 20:38:47", "2013-07-15"},
            {"2", "Bob, Jr.", "1971-01-01 14:16:06", "1971-01-01"},
            {"3", "he said \"hi\"", "2020-02-29 00:00:00", "2020-02-29"}}));
    EXPECT_EQ(ReadCsv("scheme_out.csv"),
              (std::vector<Row>{{"id", "int64"},
                                {"name", "string"},
                                {"created_at", "timestamp"},
                                {"event_date", "date"}}));
}

TEST_F(EngineTest, OpensExistingColumnarFiles) {
    WriteText("scheme.csv", "\"id\",\"int64\"\n\"name\",\"string\"\n");
    WriteText("data.csv", "\"10\",\"Ada\"\n");

    Engine built("data.csv", "scheme.csv", "data.hub");
    Engine reopened("data.hub");
    reopened.TakeAll("reopened.csv");

    EXPECT_EQ(ReadCsv("reopened.csv"), (std::vector<Row>{{"10", "Ada"}}));
    EXPECT_EQ(ReadCsv("scheme_reopened.csv"),
              (std::vector<Row>{{"id", "int64"}, {"name", "string"}}));
}

TEST_F(EngineTest, PipelineStreamsUntilBlockingOperationAndWritesCsv) {
    WriteText("scheme.csv", "\"id\",\"int64\"\n\"name\",\"string\"\n");
    WriteText("data.csv", "\"1\",\"Ada\"\n\"2\",\"Bob\"\n\"3\",\"Cara\"\n");

    Engine built("data.csv", "scheme.csv", "data.hub");
    ColumnarReader reader("data.hub");

    Scheme read_scheme;
    read_scheme.Add({"id", "int64"});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(
        std::make_unique<Filter>(MakeFilter({MakeInt64GreaterFilter(0, 1)})));
    operations.emplace_back(std::make_unique<Aggregation>(
        MakeAggregation({MakeCountAgg(0, "rows")})));

    Pipeline pipeline(std::move(operations), reader, read_scheme, "count.csv");
    pipeline.Execute();

    EXPECT_EQ(ReadCsv("count.csv"), (std::vector<Row>{{"2"}}));
}

TEST_F(EngineTest, ExecutesHitsCountAllQuery) {
    WriteText("scheme.csv", "\"AdvEngineID\",\"int16\"\n");
    WriteText("data.csv", "\"0\"\n\"2\"\n\"0\"\n\"5\"\n\"7\"\n\"1\"\n");

    Engine engine("data.csv", "scheme.csv", "hits.hub");
    ColumnarReader reader("hits.hub");

    Scheme read_scheme;
    read_scheme.Add({"AdvEngineID", "int16"});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Aggregation>(
        MakeAggregation({MakeCountAgg(0, "count")})));

    Pipeline pipeline(std::move(operations), reader, read_scheme,
                      "count_all.csv");
    engine.Execute(pipeline);

    EXPECT_EQ(ReadCsv("count_all.csv"), (std::vector<Row>{{"6"}}));
}

TEST_F(EngineTest, ExecutesHitsCountWhereAdvEngineIdIsNotZeroQuery) {
    WriteText("scheme.csv", "\"AdvEngineID\",\"int16\"\n");
    WriteText("data.csv", "\"0\"\n\"2\"\n\"0\"\n\"5\"\n\"7\"\n\"1\"\n");

    Engine engine("data.csv", "scheme.csv", "hits.hub");
    ColumnarReader reader("hits.hub");

    Scheme read_scheme;
    read_scheme.Add({"AdvEngineID", "int16"});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(
        std::make_unique<Filter>(MakeFilter({MakeInt64NotEqualFilter(0, 0)})));
    operations.emplace_back(std::make_unique<Aggregation>(
        MakeAggregation({MakeCountAgg(0, "count")})));

    Pipeline pipeline(std::move(operations), reader, read_scheme,
                      "count_adv_engine_id_not_zero.csv");
    engine.Execute(pipeline);

    EXPECT_EQ(ReadCsv("count_adv_engine_id_not_zero.csv"),
              (std::vector<Row>{{"4"}}));
}
