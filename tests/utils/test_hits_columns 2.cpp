#include <filesystem>

#include <gtest/gtest.h>

#include "io/CSVReader.h"
#include "utils/HitsColumns.h"

#ifndef COLUMNAR_TEST_FIXTURES_DIR
#define COLUMNAR_TEST_FIXTURES_DIR "tests/fixtures"
#endif

TEST(HitsColumnsTest, MatchesFixtureSchemeOrder) {
    const auto scheme_path =
        std::filesystem::path(COLUMNAR_TEST_FIXTURES_DIR) / "scheme.csv";
    CSVReader reader(scheme_path.string());

    Row row;
    size_t index = 0;
    while (!reader.IsEnd()) {
        reader.ReadNext(row);
        if (row.empty()) {
            continue;
        }

        ASSERT_LT(index, hits::kHitsColumnCount);
        ASSERT_FALSE(row[0].empty());
        EXPECT_EQ(row[0], hits::HitsColumnNames[index]) << "index " << index;
        ++index;
    }

    EXPECT_EQ(index, hits::kHitsColumnCount);
    EXPECT_EQ(hits::Col(hits::HitsColumn::URL), 13U);
    EXPECT_EQ(hits::Col(hits::HitsColumn::SearchPhrase), 39U);
    EXPECT_EQ(hits::Col(hits::HitsColumn::AdvEngineID), 40U);
    EXPECT_EQ(hits::Name(hits::HitsColumn::EventDate), "EventDate");
}
