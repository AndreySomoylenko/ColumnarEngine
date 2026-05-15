#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "core/Engine.h"
#include "io/CSVReader.h"

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

    std::vector<Raw> ReadCsv(const std::string &name) const {
        CSVReader reader(name);
        std::vector<Raw> rows;
        while (!reader.IsEnd()) {
            Raw row;
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
        (std::vector<Raw>{
            {"1", "Alice", "2013-07-14 20:38:47", "2013-07-15"},
            {"2", "Bob, Jr.", "1971-01-01 14:16:06", "1971-01-01"},
            {"3", "he said \"hi\"", "2020-02-29 00:00:00", "2020-02-29"}}));
    EXPECT_EQ(ReadCsv("scheme_out.csv"),
              (std::vector<Raw>{{"id", "int64"},
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

    EXPECT_EQ(ReadCsv("reopened.csv"), (std::vector<Raw>{{"10", "Ada"}}));
    EXPECT_EQ(ReadCsv("scheme_reopened.csv"),
              (std::vector<Raw>{{"id", "int64"}, {"name", "string"}}));
}
