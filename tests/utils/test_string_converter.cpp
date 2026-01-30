#include "utils/StringConverter.h"
#include <fstream>
#include <gtest/gtest.h>

TEST(StringConverterTest, RoundTripWithComma) {
    StringConverter conv;
    std::string filename = "unit_utils.tmp";
    // Тестируем строку, где запятая внутри кавычек (не должна разбиться)
    std::string original = "City, Country";

    {
        std::ofstream out(filename);
        conv.WriteString(out, original); // Запишет "City, Country"
    }

    {
        std::ifstream in(filename);
        // Передаем запятую как разделитель
        std::string restored = conv.ReadNextString(in, {',', EOF});
        EXPECT_EQ(original, restored);
    }
    std::remove(filename.c_str());
}