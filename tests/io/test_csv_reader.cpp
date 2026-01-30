#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>

#include "io/CSVReader.h"

using Raw = std::vector<std::string>;

class CSVReaderTest : public ::testing::Test {
protected:
    void TearDown() override { std::remove("test.csv"); }
};

TEST_F(CSVReaderTest, SimpleLine) {
    std::ofstream("test.csv") << "a,b,c\n";

    CSVReader r("test.csv", ','); // Явно передаем запятую
    EXPECT_EQ(r.ReadNext(), (Raw{"a", "b", "c"}));
}

TEST_F(CSVReaderTest, QuotedField) {
    std::ofstream("test.csv") << "\"a,b\",c\n"; // Запятая внутри кавычек

    CSVReader r("test.csv", ',');
    EXPECT_EQ(r.ReadNext(), (Raw{"a,b", "c"}));
}

TEST_F(CSVReaderTest, DoubleQuotesInside) {
    std::ofstream("test.csv") << "\"a\"\"b\",c\n";

    CSVReader r("test.csv", ',');
    EXPECT_EQ(r.ReadNext(), (Raw{"a\"b", "c"}));
}

TEST_F(CSVReaderTest, EmptyFields) {
    std::ofstream("test.csv") << "a,,c\n";

    CSVReader r("test.csv", ',');
    EXPECT_EQ(r.ReadNext(), (Raw{"a", "", "c"}));
}

TEST_F(CSVReaderTest, MultipleLines) {
    std::ofstream("test.csv") << "a,b,c\n1,2,3\n";

    CSVReader r("test.csv", ',');
    EXPECT_EQ(r.ReadNext(), (Raw{"a", "b", "c"}));
    EXPECT_EQ(r.ReadNext(), (Raw{"1", "2", "3"}));
    EXPECT_TRUE(r.IsEnd());
}

TEST_F(CSVReaderTest, BrokenQuotesThrow) {
    std::ofstream("test.csv") << "\"abc,def\n";

    CSVReader r("test.csv", ',');
    EXPECT_THROW(r.ReadNext(), std::runtime_error);
}

TEST_F(CSVReaderTest, NonExistentFileThrows) {
    EXPECT_THROW(CSVReader r("no_such_file.csv", ','), std::invalid_argument);
}