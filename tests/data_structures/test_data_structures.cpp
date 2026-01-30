#include "data_structures/Butch.h"
#include "data_structures/Scheme.h"
#include <gtest/gtest.h>

TEST(DataStructuresTest, SchemeWithCommaLogic) {
    Scheme s;
    // Схема в CSV обычно тоже парсится ридером, проверим добавление
    s.Add({"user_id", "int64"});
    s.Add({"price", "int64"});

    EXPECT_EQ(s.GetSchemeTypes().size(), 2);
    EXPECT_EQ(s.GetSchemeNames()[1], "price");
}

TEST(DataStructuresTest, ButchPushAndClear) {
    Scheme s;
    s.Add({"id", "int64"});
    Butch butch(s);

    butch.AddRaw({"100"});
    EXPECT_EQ(butch.VerticalSize(), 1);

    butch.Clear();
    EXPECT_EQ(butch.VerticalSize(), 0);
}