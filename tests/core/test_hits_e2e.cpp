#include <cstdlib>
#include <filesystem>
#include <numeric>
#include <system_error>

#include <gtest/gtest.h>

#include "core/Engine.h"
#include "io/CSVReader.h"
#include "io/ColumnarReader.h"
#include "io/Scanner.h"

namespace {

#ifndef COLUMNAR_TEST_FIXTURES_DIR
#define COLUMNAR_TEST_FIXTURES_DIR "tests/fixtures"
#endif

std::filesystem::path EnvPathOrDefault(const char *name, const char *fallback) {
    const char *value = std::getenv(name);
    if (value != nullptr && value[0] != '\0') {
        return value;
    }
    return fallback;
}

Raw ReadFirstRow(const std::filesystem::path &path) {
    CSVReader reader(path.string());
    Raw row;
    reader.ReadNext(row);
    return row;
}

std::string NormalizeValue(const std::string &s) {
    if (s.size() >= 2 && s[0] == '"' && s[s.size() - 1] == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

std::vector<size_t> AllColumns(const ColumnarReader &reader) {
    std::vector<size_t> columns(reader.GetScheme().GetSchemeNames().size());
    std::iota(columns.begin(), columns.end(), 0);
    return columns;
}

} // namespace

TEST(HitsE2ETest, ConvertsRealHitsSampleAndReadsItBack) {
    const auto fixtures_dir = std::filesystem::path(COLUMNAR_TEST_FIXTURES_DIR);
    const auto data_path = EnvPathOrDefault(
        "COLUMNAR_HITS_CSV", (fixtures_dir / "hits_sample.csv").c_str());
    const auto scheme_path = EnvPathOrDefault(
        "COLUMNAR_HITS_SCHEME", (fixtures_dir / "scheme.csv").c_str());

    if (!std::filesystem::exists(data_path) ||
        !std::filesystem::exists(scheme_path)) {
        GTEST_SKIP() << "Real hits CSV or scheme CSV is missing.";
    }

    const auto temp_dir =
        std::filesystem::temp_directory_path() / "columnar_engine_hits_e2e";
    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    std::filesystem::create_directories(temp_dir);

    const auto columnar_path = temp_dir / "hits_sample.hub";
    const Raw expected_first_row = ReadFirstRow(data_path);

    Engine engine(data_path.string(), scheme_path.string(),
                  columnar_path.string());
    ColumnarReader reader(columnar_path.string());
    std::vector<size_t> columns = AllColumns(reader);
    Scanner scanner(columns, reader);

    ASSERT_FALSE(scanner.IsEnd());
    Butch first_chunk = scanner.ReadNext();

    EXPECT_EQ(reader.GetScheme().GetSchemeNames().size(),
              expected_first_row.size());
    ASSERT_GT(first_chunk.VerticalSize(), 0U);
    EXPECT_EQ(first_chunk.GetRaw(0), expected_first_row);

    std::filesystem::remove_all(temp_dir, ec);
}

TEST(HitsE2ETest, ConvertHitsSampleAndPrintStats) {
    const auto fixtures_dir = std::filesystem::path(COLUMNAR_TEST_FIXTURES_DIR);
    const auto data_path = EnvPathOrDefault(
        "COLUMNAR_HITS_CSV", (fixtures_dir / "hits_sample.csv").c_str());
    const auto scheme_path = EnvPathOrDefault(
        "COLUMNAR_HITS_SCHEME", (fixtures_dir / "scheme.csv").c_str());

    if (!std::filesystem::exists(data_path) ||
        !std::filesystem::exists(scheme_path)) {
        GTEST_SKIP() << "Real hits CSV or scheme CSV is missing.";
    }

    const auto temp_dir =
        std::filesystem::temp_directory_path() / "columnar_engine_hits_convert";
    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    std::filesystem::create_directories(temp_dir);

    const auto columnar_path = temp_dir / "hits_sample.hub";

    Engine engine(data_path.string(), scheme_path.string(),
                  columnar_path.string());

    ColumnarReader reader(columnar_path.string());
    std::vector<size_t> columns = AllColumns(reader);
    Scanner scanner(columns, reader);
    size_t total_rows = 0;
    while (!scanner.IsEnd()) {
        Butch chunk = scanner.ReadNext();
        total_rows += chunk.VerticalSize();
    }

    EXPECT_GT(total_rows, 0U);

    std::filesystem::remove_all(temp_dir, ec);
}

TEST(HitsE2ETest, RoundTripCsvToHubToCsvMatches) {
    const auto fixtures_dir = std::filesystem::path(COLUMNAR_TEST_FIXTURES_DIR);
    const auto data_path = EnvPathOrDefault(
        "COLUMNAR_HITS_CSV", (fixtures_dir / "hits_sample.csv").c_str());
    const auto scheme_path = EnvPathOrDefault(
        "COLUMNAR_HITS_SCHEME", (fixtures_dir / "scheme.csv").c_str());

    if (!std::filesystem::exists(data_path) ||
        !std::filesystem::exists(scheme_path)) {
        GTEST_SKIP() << "Real hits CSV or scheme CSV is missing.";
    }

    const auto temp_dir = std::filesystem::temp_directory_path() /
                          "columnar_engine_hits_roundtrip";
    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    std::filesystem::create_directories(temp_dir);

    const auto columnar_path = temp_dir / "hits_sample.hub";
    const auto output_csv_path = temp_dir / "hits_output.csv";

    // Step 1: CSV -> Hub
    {
        Engine engine1(data_path.string(), scheme_path.string(),
                       columnar_path.string());
    }

    // Step 2: Hub -> CSV
    {
        Engine engine2(columnar_path.string());
        engine2.TakeAll(output_csv_path.string());
    }

    // Step 3: Compare original and output
    CSVReader original_reader(data_path.string());
    CSVReader output_reader(output_csv_path.string());

    Raw original_row, output_row;
    size_t row_count = 0;
    const size_t max_rows_to_check =
        1000; // Check first 1000 rows for performance

    while (row_count < max_rows_to_check) {
        original_reader.ReadNext(original_row);
        output_reader.ReadNext(output_row);

        if (original_row.empty() && output_row.empty()) {
            break;
        }

        ASSERT_EQ(original_row.size(), output_row.size())
            << "Row " << row_count << " has different field count";

        for (size_t i = 0; i < original_row.size(); ++i) {
            std::string orig_normalized = NormalizeValue(original_row[i]);
            std::string out_normalized = NormalizeValue(output_row[i]);

            EXPECT_EQ(orig_normalized, out_normalized)
                << "Row " << row_count << ", field " << i << " mismatch"
                << "\n  Original: '" << original_row[i] << "'"
                << "\n  Output:   '" << output_row[i] << "'";
        }

        row_count++;
    }

    EXPECT_GT(row_count, 0U) << "No rows were compared";

    std::filesystem::remove_all(temp_dir, ec);
}
