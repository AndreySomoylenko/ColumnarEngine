#include "core/Engine.h"
#include "io/CSVReader.h"
#include <filesystem>
#include <gtest/gtest.h>

TEST(EngineTest, FullRoundTripCommaSeparator) {
    std::string csv_in = "data_in.csv";
    std::string scheme_in = "scheme_in.csv";
    std::string hub = "storage.hub";
    std::string csv_out = "data_out.csv";
    std::string scheme_out = "scheme_data_out.csv";

    // 1. Создаем файлы с запятой
    {
        std::ofstream s(scheme_in);
        s << "id,int64\n" << "name,string"; // Схема: имя, тип

        std::ofstream d(csv_in);
        d << "1,\"Ivanov, Ivan\"\n" << "2,\"Petrov, Petr\"";
    }

    // 2. Импорт (CSV -> Hub)
    // Если ты не менял дефолт в Engine.h, добавь поддержку выбора разделителя в Engine
    // или временно измени дефолтные параметры в конструкторах ридеров.
    {
        Engine engine(csv_in, scheme_in, hub);
        // engine.TakeAll(csv_out);
    }

    // 3. Экспорт (Hub -> CSV)
    {
        Engine reader(hub);
        reader.TakeAll(csv_out);
    }

    // // 4. Проверка восстановленной схемы
    {
        CSVReader sch_check(scheme_out);
        auto r1 = sch_check.ReadNext();
        EXPECT_EQ(r1[0], "id");
        EXPECT_EQ(r1[1], "int64");
    }

    // // 5. Проверка данных
    {
        CSVReader data_check(csv_out, ',');
        auto r1 = data_check.ReadNext();
        EXPECT_EQ(r1[0], "1");
        EXPECT_EQ(r1[1], "Ivanov, Ivan");
        auto r2 = data_check.ReadNext();
        EXPECT_EQ(r2[0], "2");
        EXPECT_EQ(r2[1], "Petrov, Petr");
    }

    // Чистим файлы
    std::filesystem::remove(csv_in);
    std::filesystem::remove(scheme_in);
    std::filesystem::remove(hub);
    std::filesystem::remove(csv_out);
    std::filesystem::remove(scheme_out);
}