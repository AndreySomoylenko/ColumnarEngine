#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "io/CsvReader.h"
#include "io/CsvWriter.h"

namespace {

using Row = std::vector<std::string>;

class CsvIoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() /
                    "columnar_engine_csv_tests";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(temp_dir_, ec);
    }

    std::filesystem::path Path(const std::string &name) const {
        return temp_dir_ / name;
    }

    void WriteText(const std::filesystem::path &path,
                   const std::string &text) const {
        std::ofstream out(path);
        out << text;
    }

    std::string ReadWholeFile(const std::filesystem::path &path) const {
        std::ifstream in(path);
        return std::string((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());
    }

    std::filesystem::path temp_dir_;
};

} // namespace

TEST_F(CsvIoTest, ReaderParsesQuotedFieldsAndEmptyCells) {
    WriteText(Path("input.csv"),
              "id,,\"name,with,comma\",\"he said \"\"hi\"\"\",\"\"\n");

    CsvReader reader(Path("input.csv").string());
    Row row;

    reader.ReadNext(row);

    EXPECT_EQ(row, (Row{"id", "", "name,with,comma", "he said \"hi\"", ""}));
}

TEST_F(CsvIoTest, ReaderKeepsTrailingEmptyFields) {
    WriteText(Path("input.csv"), "a,b,\n,\n\n");

    CsvReader reader(Path("input.csv").string());
    Row row;

    reader.ReadNext(row);
    EXPECT_EQ(row, (Row{"a", "b", ""}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_EQ(row, (Row{"", ""}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_TRUE(row.empty());
}

TEST_F(CsvIoTest, ReaderSupportsCustomSeparators) {
    WriteText(Path("input.csv"), "\"a;b\";c\n");

    CsvReader reader(Path("input.csv").string(), ';');
    Row row;

    reader.ReadNext(row);
    EXPECT_EQ(row, (Row{"a;b", "c"}));
}

TEST_F(CsvIoTest, ReaderRejectsUnclosedQuotes) {
    WriteText(Path("input.csv"), "\"unterminated\n");

    CsvReader reader(Path("input.csv").string());
    Row row;

    EXPECT_THROW(reader.ReadNext(row), std::invalid_argument);
}

TEST_F(CsvIoTest, ReaderMovesThroughMultipleRows) {
    WriteText(Path("input.csv"), "a,b,c\n1,2,3\n");

    CsvReader reader(Path("input.csv").string());
    Row row;

    reader.ReadNext(row);
    EXPECT_EQ(row, (Row{"a", "b", "c"}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_EQ(row, (Row{"1", "2", "3"}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_TRUE(row.empty());
    EXPECT_TRUE(reader.IsEnd());
}

TEST_F(CsvIoTest, ReaderThrowsForMissingFile) {
    EXPECT_THROW(CsvReader reader(Path("missing.csv").string()),
                 std::invalid_argument);
}

TEST_F(CsvIoTest, WriterQuotesAndEscapesFields) {
    {
        CsvWriter writer(Path("out.csv").string());
        writer.WriteRow({"a", "", "b,c", "he said \"hi\""});
    }

    EXPECT_EQ(ReadWholeFile(Path("out.csv")),
              "\"a\",\"\",\"b,c\",\"he said \"\"hi\"\"\"\n");
}

TEST_F(CsvIoTest, WriterRoundTripsThroughReader) {
    {
        CsvWriter writer(Path("roundtrip.csv").string());
        writer.WriteRow({"1", "Ivanov, Ivan"});
        writer.WriteRow({"2", "Petrov \"Petr\""});
    }

    CsvReader reader(Path("roundtrip.csv").string());
    Row row;

    reader.ReadNext(row);
    EXPECT_EQ(row, (Row{"1", "Ivanov, Ivan"}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_EQ(row, (Row{"2", "Petrov \"Petr\""}));
}

TEST_F(CsvIoTest, WriterThrowsWhenDirectoryIsMissing) {
    const auto bad_path = Path("missing-dir/out.csv");
    EXPECT_THROW(CsvWriter writer(bad_path.string()), std::invalid_argument);
}

TEST_F(CsvIoTest, WriterSupportsCustomSeparators) {
    {
        CsvWriter writer(Path("semi.csv").string(), ';');
        writer.WriteRow({"a;b", "c"});
    }

    EXPECT_EQ(ReadWholeFile(Path("semi.csv")), "\"a;b\";\"c\"\n");
}
