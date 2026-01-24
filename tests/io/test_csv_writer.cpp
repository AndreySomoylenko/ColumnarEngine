#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>

#include "io/CSVWriter.h"

using Raw = std::vector<std::string>;

class CSVWriterTest : public ::testing::Test {
protected:
    void TearDown() override {
        std::remove("out.csv");
    }
};

TEST_F(CSVWriterTest, SimpleWrite) {
    CSVWriter w("out.csv");
    w.WriteRaw({"a","b","c"});

    std::ifstream in("out.csv");
    std::string line;
    std::getline(in, line);

    EXPECT_EQ(line, "\"a\";\"b\";\"c\"");
}

TEST_F(CSVWriterTest, EmptyFieldsWrite) {
    CSVWriter w("out.csv");
    w.WriteRaw({"a","","c"});

    std::ifstream in("out.csv");
    std::string line;
    std::getline(in, line);

    EXPECT_EQ(line, "\"a\";\"\";\"c\"");
}

TEST_F(CSVWriterTest, QuotesInsideField) {
    CSVWriter w("out.csv");
    w.WriteRaw({"a\"b","c"});

    std::ifstream in("out.csv");
    std::string line;
    std::getline(in, line);

    EXPECT_EQ(line, "\"a\"\"b\";\"c\"");
}

TEST_F(CSVWriterTest, MultipleWrites) {
    CSVWriter w("out.csv");
    w.WriteRaw({"a","b"});
    w.WriteRaw({"1","2"});

    std::ifstream in("out.csv");
    std::string line1, line2;
    std::getline(in, line1);
    std::getline(in, line2);

    EXPECT_EQ(line1, "\"a\";\"b\"");
    EXPECT_EQ(line2, "\"1\";\"2\"");
}

TEST_F(CSVWriterTest, CannotCreateFileThrows) {
    EXPECT_THROW(CSVWriter w("/invalid_path/out.csv"), std::invalid_argument);
}
