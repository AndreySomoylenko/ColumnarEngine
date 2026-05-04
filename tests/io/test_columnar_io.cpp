#include <filesystem>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "data_structures/Butch.h"
#include "data_structures/Scheme.h"
#include "io/ColumnarReader.h"
#include "io/ColumnarWriter.h"

namespace {

class ColumnarIoTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() / "columnar_engine_columnar_tests";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(temp_dir_, ec);
    }

    std::filesystem::path Path(const std::string &name) const { return temp_dir_ / name; }

    Scheme MakeScheme() const {
        Scheme scheme;
        scheme.Add({"id", "int64"});
        scheme.Add({"name", "string"});
        scheme.Add({"payload", "unknown"});
        return scheme;
    }

    std::filesystem::path temp_dir_;
};

} // namespace

TEST_F(ColumnarIoTest, WritesAndReadsMultipleChunksWithScheme) {
    Scheme scheme = MakeScheme();

    Butch first(scheme, false);
    first.AddRaw({"1", "Alice", "abc"});
    first.AddRaw({"2", "Bob", ""});

    Butch second(scheme, false);
    second.AddRaw({"3", "Caroline", "longer payload"});

    {
        ColumnarWriter writer(Path("data.hub").string());
        writer.WriteChunk(first);
        writer.WriteChunk(second);
        writer.Close(scheme);
    }

    ColumnarReader reader(Path("data.hub").string());

    EXPECT_EQ(reader.GetScheme().GiveRaws(),
              (std::vector<Raw>{{"id", "int64"}, {"name", "string"}, {"payload", "unknown"}}));
    EXPECT_FALSE(reader.IsEnd());

    Butch read_first = reader.ReadNext();
    EXPECT_EQ(read_first.HorizontalSize(), 3U);
    EXPECT_EQ(read_first.VerticalSize(), 2U);
    EXPECT_EQ(read_first.GetRaw(0), (Raw{"1", "Alice", "abc"}));
    EXPECT_EQ(read_first.GetRaw(1), (Raw{"2", "Bob", ""}));

    Butch read_second = reader.ReadNext();
    EXPECT_EQ(read_second.VerticalSize(), 1U);
    EXPECT_EQ(read_second.GetRaw(0), (Raw{"3", "Caroline", "longer payload"}));
    EXPECT_TRUE(reader.IsEnd());

    reader.Reset();
    EXPECT_FALSE(reader.IsEnd());
    EXPECT_EQ(reader.ReadNext().GetRaw(0), (Raw{"1", "Alice", "abc"}));
}

TEST_F(ColumnarIoTest, SupportsStringOnlySchemas) {
    Scheme scheme;
    scheme.Add({"first", "string"});
    scheme.Add({"second", "string"});

    Butch butch(scheme, false);
    butch.AddRaw({"abc", "de"});
    butch.AddRaw({"", "a longer value"});

    {
        ColumnarWriter writer(Path("strings.hub").string());
        writer.WriteChunk(butch);
        writer.Close(scheme);
    }

    ColumnarReader reader(Path("strings.hub").string());
    Butch read = reader.ReadNext();

    EXPECT_EQ(read.VerticalSize(), 2U);
    EXPECT_EQ(read.GetRaw(0), (Raw{"abc", "de"}));
    EXPECT_EQ(read.GetRaw(1), (Raw{"", "a longer value"}));
}

TEST_F(ColumnarIoTest, ThrowsForMissingFilesAndUnwritableDestinations) {
    EXPECT_THROW(ColumnarReader reader(Path("missing.hub").string()), std::invalid_argument);
    EXPECT_THROW(ColumnarWriter writer(Path("missing-dir/out.hub").string()), std::runtime_error);
}
