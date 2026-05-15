#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "io/CSVReader.h"
#include "io/CSVWriter.h"

namespace {

using Raw = std::vector<std::string>;

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

    CSVReader reader(Path("input.csv").string());
    Raw row;

    reader.ReadNext(row);

    EXPECT_EQ(row, (Raw{"id", "", "name,with,comma", "he said \"hi\"", ""}));
}

TEST_F(CsvIoTest, ReaderKeepsTrailingEmptyFields) {
    WriteText(Path("input.csv"), "a,b,\n,\n\n");

    CSVReader reader(Path("input.csv").string());
    Raw row;

    reader.ReadNext(row);
    EXPECT_EQ(row, (Raw{"a", "b", ""}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_EQ(row, (Raw{"", ""}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_TRUE(row.empty());
}

TEST_F(CsvIoTest, ReaderSupportsCustomSeparators) {
    WriteText(Path("input.csv"), "\"a;b\";c\n");

    CSVReader reader(Path("input.csv").string(), ';');
    Raw row;

    reader.ReadNext(row);
    EXPECT_EQ(row, (Raw{"a;b", "c"}));
}

TEST_F(CsvIoTest, ReaderRejectsUnclosedQuotes) {
    WriteText(Path("input.csv"), "\"unterminated\n");

    CSVReader reader(Path("input.csv").string());
    Raw row;

    EXPECT_THROW(reader.ReadNext(row), std::invalid_argument);
}

TEST_F(CsvIoTest, ReaderMovesThroughMultipleRows) {
    WriteText(Path("input.csv"), "a,b,c\n1,2,3\n");

    CSVReader reader(Path("input.csv").string());
    Raw row;

    reader.ReadNext(row);
    EXPECT_EQ(row, (Raw{"a", "b", "c"}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_EQ(row, (Raw{"1", "2", "3"}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_TRUE(row.empty());
    EXPECT_TRUE(reader.IsEnd());
}

TEST_F(CsvIoTest, ReaderThrowsForMissingFile) {
    EXPECT_THROW(CSVReader reader(Path("missing.csv").string()),
                 std::invalid_argument);
}

TEST_F(CsvIoTest, WriterQuotesAndEscapesFields) {
    {
        CSVWriter writer(Path("out.csv").string());
        writer.WriteRaw({"a", "", "b,c", "he said \"hi\""});
    }

    EXPECT_EQ(ReadWholeFile(Path("out.csv")),
              "\"a\",\"\",\"b,c\",\"he said \"\"hi\"\"\"\n");
}

TEST_F(CsvIoTest, WriterRoundTripsThroughReader) {
    {
        CSVWriter writer(Path("roundtrip.csv").string());
        writer.WriteRaw({"1", "Ivanov, Ivan"});
        writer.WriteRaw({"2", "Petrov \"Petr\""});
    }

    CSVReader reader(Path("roundtrip.csv").string());
    Raw row;

    reader.ReadNext(row);
    EXPECT_EQ(row, (Raw{"1", "Ivanov, Ivan"}));

    row.clear();
    reader.ReadNext(row);
    EXPECT_EQ(row, (Raw{"2", "Petrov \"Petr\""}));
}

TEST_F(CsvIoTest, WriterThrowsWhenDirectoryIsMissing) {
    const auto bad_path = Path("missing-dir/out.csv");
    EXPECT_THROW(CSVWriter writer(bad_path.string()), std::invalid_argument);
}

TEST_F(CsvIoTest, WriterSupportsCustomSeparators) {
    {
        CSVWriter writer(Path("semi.csv").string(), ';');
        writer.WriteRaw({"a;b", "c"});
    }

    EXPECT_EQ(ReadWholeFile(Path("semi.csv")), "\"a;b\";\"c\"\n");
}
