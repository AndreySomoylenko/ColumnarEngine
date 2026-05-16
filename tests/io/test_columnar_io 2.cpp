#include <filesystem>
#include <numeric>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "data_structures/Batch.h"
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
        const auto rows = scheme.GiveRows();
        for (size_t index : indexes) {
            result.Add(rows[index]);
        }
        return result;
    }

    Scheme AllColumns(const Scheme &scheme) const {
        Scheme result;
        for (const auto &row : scheme.GiveRows()) {
            result.Add(row);
        }
        return result;
    }

    std::filesystem::path temp_dir_;
};

} // namespace

TEST_F(ColumnarIoTest, WritesAndReadsMultipleChunksWithScheme) {
    Scheme scheme = MakeScheme();

    Batch first(scheme, false);
    first.AddRow({"1", "Alice", "abc"});
    first.AddRow({"2", "Bob", ""});

    Batch second(scheme, false);
    second.AddRow({"3", "Caroline", "longer payload"});

    {
        ColumnarWriter writer(Path("data.hub").string());
        writer.WriteChunk(first);
        writer.WriteChunk(second);
        writer.Close(scheme);
    }

    ColumnarReader reader(Path("data.hub").string());

    EXPECT_EQ(reader.GetScheme().GiveRows(),
              (std::vector<Row>{{"id", "int64"},
                                {"name", "string"},
                                {"payload", "unknown"}}));
    Scheme columns = AllColumns(reader.GetScheme());
    IOScanner scanner(columns, reader);
    EXPECT_FALSE(scanner.IsEnd());

    Batch read_first = scanner.ReadNext();
    EXPECT_EQ(read_first.HorizontalSize(), 3U);
    EXPECT_EQ(read_first.VerticalSize(), 2U);
    EXPECT_EQ(read_first.GetRow(0), (Row{"1", "Alice", "abc"}));
    EXPECT_EQ(read_first.GetRow(1), (Row{"2", "Bob", ""}));

    Batch read_second = scanner.ReadNext();
    EXPECT_EQ(read_second.VerticalSize(), 1U);
    EXPECT_EQ(read_second.GetRow(0), (Row{"3", "Caroline", "longer payload"}));
    EXPECT_TRUE(scanner.IsEnd());
    EXPECT_THROW(scanner.ReadNext(), std::out_of_range);

    scanner.Reset();
    EXPECT_FALSE(scanner.IsEnd());
    EXPECT_EQ(scanner.ReadNext().GetRow(0), (Row{"1", "Alice", "abc"}));
}

TEST_F(ColumnarIoTest, SupportsStringOnlySchemas) {
    Scheme scheme;
    scheme.Add({"first", "string"});
    scheme.Add({"second", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"abc", "de"});
    batch.AddRow({"", "a longer value"});

    {
        ColumnarWriter writer(Path("strings.hub").string());
        writer.WriteChunk(batch);
        writer.Close(scheme);
    }

    ColumnarReader reader(Path("strings.hub").string());
    Scheme columns = AllColumns(reader.GetScheme());
    IOScanner scanner(columns, reader);
    Batch read = scanner.ReadNext();

    EXPECT_EQ(read.VerticalSize(), 2U);
    EXPECT_EQ(read.GetRow(0), (Row{"abc", "de"}));
    EXPECT_EQ(read.GetRow(1), (Row{"", "a longer value"}));
}

TEST_F(ColumnarIoTest, ScannerReadsSelectedColumnsOnly) {
    Scheme scheme = MakeScheme();

    Batch batch(scheme, false);
    batch.AddRow({"1", "Alice", "abc"});
    batch.AddRow({"2", "Bob", "payload"});

    {
        ColumnarWriter writer(Path("projected.hub").string());
        writer.WriteChunk(batch);
        writer.Close(scheme);
    }

    ColumnarReader reader(Path("projected.hub").string());
    Scheme columns = Project(reader.GetScheme(), {1, 2});
    IOScanner scanner(columns, reader);

    Batch read = scanner.ReadNext();

    EXPECT_EQ(read.HorizontalSize(), 2U);
    EXPECT_EQ(read.VerticalSize(), 2U);
    EXPECT_EQ(read.GetRow(0), (Row{"Alice", "abc"}));
    EXPECT_EQ(read.GetRow(1), (Row{"Bob", "payload"}));
    EXPECT_TRUE(scanner.IsEnd());
}

TEST_F(ColumnarIoTest, ReadsAndWritesTimestampColumns) {
    Scheme scheme;
    scheme.Add({"id", "int64"});
    scheme.Add({"created_at", "timestamp"});
    scheme.Add({"event_date", "date"});
    scheme.Add({"name", "string"});

    Batch batch(scheme, false);
    batch.AddRow({"1", "2013-07-14 20:38:47", "2013-07-15", "Alice"});
    batch.AddRow({"2", "1971-01-01 14:16:06", "1971-01-01", "Bob"});

    {
        ColumnarWriter writer(Path("timestamps.hub").string());
        writer.WriteChunk(batch);
        writer.Close(scheme);
    }

    ColumnarReader reader(Path("timestamps.hub").string());
    EXPECT_EQ(reader.GetScheme().GiveRows(),
              (std::vector<Row>{{"id", "int64"},
                                {"created_at", "timestamp"},
                                {"event_date", "date"},
                                {"name", "string"}}));

    Scheme all_columns = AllColumns(reader.GetScheme());
    IOScanner all_scanner(all_columns, reader);
    Batch all_read = all_scanner.ReadNext();

    EXPECT_EQ(all_read.GetRow(0),
              (Row{"1", "2013-07-14 20:38:47", "2013-07-15", "Alice"}));
    EXPECT_EQ(all_read.GetRow(1),
              (Row{"2", "1971-01-01 14:16:06", "1971-01-01", "Bob"}));

    Scheme timestamp_column = Project(reader.GetScheme(), {1});
    IOScanner timestamp_scanner(timestamp_column, reader);
    Batch timestamp_read = timestamp_scanner.ReadNext();

    EXPECT_EQ(timestamp_read.HorizontalSize(), 1U);
    EXPECT_EQ(timestamp_read.GetRow(0), (Row{"2013-07-14 20:38:47"}));
    EXPECT_EQ(timestamp_read.GetRow(1), (Row{"1971-01-01 14:16:06"}));

    Scheme date_column = Project(reader.GetScheme(), {2});
    IOScanner date_scanner(date_column, reader);
    Batch date_read = date_scanner.ReadNext();

    EXPECT_EQ(date_read.HorizontalSize(), 1U);
    EXPECT_EQ(date_read.GetRow(0), (Row{"2013-07-15"}));
    EXPECT_EQ(date_read.GetRow(1), (Row{"1971-01-01"}));
}

TEST_F(ColumnarIoTest, ThrowsForMissingFilesAndUnwritableDestinations) {
    EXPECT_THROW(ColumnarReader reader(Path("missing.hub").string()),
                 std::invalid_argument);
    EXPECT_THROW(ColumnarWriter writer(Path("missing-dir/out.hub").string()),
                 std::runtime_error);
}
