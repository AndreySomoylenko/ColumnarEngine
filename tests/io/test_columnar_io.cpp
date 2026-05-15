#include <filesystem>
#include <numeric>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "data_structures/Butch.h"
#include "data_structures/Scheme.h"
#include "io/ColumnarReader.h"
#include "io/ColumnarWriter.h"
#include "io/IOScanner.h"

namespace {

class ColumnarIoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() /
                    "columnar_engine_columnar_tests";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(temp_dir_, ec);
    }

    std::filesystem::path Path(const std::string &name) const {
        return temp_dir_ / name;
    }

    Scheme MakeScheme() const {
        Scheme scheme;
        scheme.Add({"id", "int64"});
        scheme.Add({"name", "string"});
        scheme.Add({"payload", "unknown"});
        return scheme;
    }

    Scheme Project(const Scheme &scheme,
                   std::initializer_list<size_t> indexes) const {
        Scheme result;
        const auto raws = scheme.GiveRaws();
        for (size_t index : indexes) {
            result.Add(raws[index]);
        }
        return result;
    }

    Scheme AllColumns(const Scheme &scheme) const {
        Scheme result;
        for (const auto &raw : scheme.GiveRaws()) {
            result.Add(raw);
        }
        return result;
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
              (std::vector<Raw>{{"id", "int64"},
                                {"name", "string"},
                                {"payload", "unknown"}}));
    Scheme columns = AllColumns(reader.GetScheme());
    IOScanner scanner(columns, reader);
    EXPECT_FALSE(scanner.IsEnd());

    Butch read_first = scanner.ReadNext();
    EXPECT_EQ(read_first.HorizontalSize(), 3U);
    EXPECT_EQ(read_first.VerticalSize(), 2U);
    EXPECT_EQ(read_first.GetRaw(0), (Raw{"1", "Alice", "abc"}));
    EXPECT_EQ(read_first.GetRaw(1), (Raw{"2", "Bob", ""}));

    Butch read_second = scanner.ReadNext();
    EXPECT_EQ(read_second.VerticalSize(), 1U);
    EXPECT_EQ(read_second.GetRaw(0), (Raw{"3", "Caroline", "longer payload"}));
    EXPECT_TRUE(scanner.IsEnd());
    EXPECT_THROW(scanner.ReadNext(), std::out_of_range);

    scanner.Reset();
    EXPECT_FALSE(scanner.IsEnd());
    EXPECT_EQ(scanner.ReadNext().GetRaw(0), (Raw{"1", "Alice", "abc"}));
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
    Scheme columns = AllColumns(reader.GetScheme());
    IOScanner scanner(columns, reader);
    Butch read = scanner.ReadNext();

    EXPECT_EQ(read.VerticalSize(), 2U);
    EXPECT_EQ(read.GetRaw(0), (Raw{"abc", "de"}));
    EXPECT_EQ(read.GetRaw(1), (Raw{"", "a longer value"}));
}

TEST_F(ColumnarIoTest, ScannerReadsSelectedColumnsOnly) {
    Scheme scheme = MakeScheme();

    Butch butch(scheme, false);
    butch.AddRaw({"1", "Alice", "abc"});
    butch.AddRaw({"2", "Bob", "payload"});

    {
        ColumnarWriter writer(Path("projected.hub").string());
        writer.WriteChunk(butch);
        writer.Close(scheme);
    }

    ColumnarReader reader(Path("projected.hub").string());
    Scheme columns = Project(reader.GetScheme(), {1, 2});
    IOScanner scanner(columns, reader);

    Butch read = scanner.ReadNext();

    EXPECT_EQ(read.HorizontalSize(), 2U);
    EXPECT_EQ(read.VerticalSize(), 2U);
    EXPECT_EQ(read.GetRaw(0), (Raw{"Alice", "abc"}));
    EXPECT_EQ(read.GetRaw(1), (Raw{"Bob", "payload"}));
    EXPECT_TRUE(scanner.IsEnd());
}

TEST_F(ColumnarIoTest, ReadsAndWritesTimestampColumns) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"created_at", "timestamp"});
    scheme.Add({"event_date", "date"});
    scheme.Add({"name", "string"});

    Butch butch(scheme, false);
    butch.AddRaw({"1", "2013-07-14 20:38:47", "2013-07-15", "Alice"});
    butch.AddRaw({"2", "1971-01-01 14:16:06", "1971-01-01", "Bob"});

    {
        ColumnarWriter writer(Path("timestamps.hub").string());
        writer.WriteChunk(butch);
        writer.Close(scheme);
    }

    ColumnarReader reader(Path("timestamps.hub").string());
    EXPECT_EQ(reader.GetScheme().GiveRaws(),
              (std::vector<Raw>{{"id", "int64"},
                                {"created_at", "timestamp"},
                                {"event_date", "date"},
                                {"name", "string"}}));

    Scheme all_columns = AllColumns(reader.GetScheme());
    IOScanner all_scanner(all_columns, reader);
    Butch all_read = all_scanner.ReadNext();

    EXPECT_EQ(all_read.GetRaw(0),
              (Raw{"1", "2013-07-14 20:38:47", "2013-07-15", "Alice"}));
    EXPECT_EQ(all_read.GetRaw(1),
              (Raw{"2", "1971-01-01 14:16:06", "1971-01-01", "Bob"}));

    Scheme timestamp_column = Project(reader.GetScheme(), {1});
    IOScanner timestamp_scanner(timestamp_column, reader);
    Butch timestamp_read = timestamp_scanner.ReadNext();

    EXPECT_EQ(timestamp_read.HorizontalSize(), 1U);
    EXPECT_EQ(timestamp_read.GetRaw(0), (Raw{"2013-07-14 20:38:47"}));
    EXPECT_EQ(timestamp_read.GetRaw(1), (Raw{"1971-01-01 14:16:06"}));

    Scheme date_column = Project(reader.GetScheme(), {2});
    IOScanner date_scanner(date_column, reader);
    Butch date_read = date_scanner.ReadNext();

    EXPECT_EQ(date_read.HorizontalSize(), 1U);
    EXPECT_EQ(date_read.GetRaw(0), (Raw{"2013-07-15"}));
    EXPECT_EQ(date_read.GetRaw(1), (Raw{"1971-01-01"}));
}

TEST_F(ColumnarIoTest, ThrowsForMissingFilesAndUnwritableDestinations) {
    EXPECT_THROW(ColumnarReader reader(Path("missing.hub").string()),
                 std::invalid_argument);
    EXPECT_THROW(ColumnarWriter writer(Path("missing-dir/out.hub").string()),
                 std::runtime_error);
}
