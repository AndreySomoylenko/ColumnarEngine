#include <gtest/gtest.h>

#include "utils/StringConverter.h"

TEST(StringConverterTest, WrapsPlainStringsInQuotes) {
    StringConverter converter;

    EXPECT_EQ(converter.TransformStringToFilestring("hello"), "\"hello\"");
}

TEST(StringConverterTest, EscapesEmbeddedQuotes) {
    StringConverter converter;

    EXPECT_EQ(converter.TransformStringToFilestring("he said \"hi\""), "\"he said \"\"hi\"\"\"");
}

TEST(StringConverterTest, PreservesCommasAndEmptyStringsInsideQuotedField) {
    StringConverter converter;

    EXPECT_EQ(converter.TransformStringToFilestring("a,b"), "\"a,b\"");
    EXPECT_EQ(converter.TransformStringToFilestring(""), "\"\"");
}
