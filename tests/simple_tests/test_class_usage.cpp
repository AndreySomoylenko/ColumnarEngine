#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "core/Engine.h"
#include "core/Pipeline.h"
#include "core/ProjectionBuilder.h"
#include "data_structures/Batch.h"
#include "data_structures/ByteVector.h"
#include "data_structures/Column.h"
#include "data_structures/MetaData.h"
#include "data_structures/Scheme.h"
#include "io/ColumnarReader.h"
#include "io/ColumnarWriter.h"
#include "io/CsvReader.h"
#include "io/CsvWriter.h"
#include "io/IoScanner.h"
#include "io/VectorScanner.h"
#include "operations/Operations.h"
#include "utils/HitsColumns.h"
#include "utils/StringConverter.h"

namespace {

class SimpleUsageTest : public ::testing::Test {
  protected:
    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() /
                    "columnar_engine_simple_usage_tests";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(temp_dir_, ec);
    }

    std::filesystem::path Path(const std::string &name) const {
        return temp_dir_ / name;
    }

    void WriteText(const std::string &name, const std::string &text) const {
        std::ofstream out(Path(name));
        out << text;
    }

    std::vector<Row> ReadCsvRows(const std::string &name) const {
        CsvReader reader(Path(name).string());
        std::vector<Row> rows;
        Row row;
        while (!reader.IsEnd()) {
            row.clear();
            reader.ReadNext(row);
            if (!row.empty()) {
                rows.push_back(row);
            }
        }
        return rows;
    }

    std::filesystem::path temp_dir_;
};

Scheme MakeScheme(std::initializer_list<Row> rows) {
    Scheme scheme;
    for (const Row &row : rows) {
        scheme.Add(row);
    }
    return scheme;
}

std::vector<Row> ReadRows(std::vector<Batch> batches) {
    std::vector<Row> rows;
    for (auto &batch : batches) {
        for (size_t row = 0; row < batch.VerticalSize(); ++row) {
            if (batch.IsRowEnabled(row)) {
                rows.push_back(batch.GetRow(row));
            }
        }
    }
    return rows;
}

} // namespace

TEST(SimpleDataStructuresUsageTest, ByteVectorStoresRawValues) {
    ByteVector bytes;
    const std::string value = "abc";

    bytes.Push(value.data(), value.size());

    EXPECT_EQ(bytes.Size(), 1U);
    EXPECT_EQ(bytes.SizeInBytes(), 3U);
    EXPECT_EQ(std::string(bytes.Data(), bytes.SizeInBytes()), "abc");
}

TEST(SimpleDataStructuresUsageTest, SchemeDescribesRows) {
    Scheme scheme;

    scheme.Add({"id", "int64"});
    scheme.Add({"name", "string"});

    EXPECT_EQ(scheme.GetColumnIndexByName("name"), 1U);
    EXPECT_EQ(scheme.GetColumnTypeByName("id"), ColumnTypes::Int64);
    EXPECT_EQ(scheme.GiveRows(),
              (std::vector<Row>{{"id", "int64"}, {"name", "string"}}));
}

TEST(SimpleDataStructuresUsageTest, ColumnsStoreTypedValues) {
    Int16Column int16_column;
    Int32Column int32_column;
    Int64Column int64_column;
    Int128Column int128_column;
    DoubleColumn double_column;
    StringColumn string_column;
    TimeColumn timestamp_column;
    TimeColumn date_column(true);

    int16_column.Push("16");
    int32_column.Push("32");
    int64_column.Push("64");
    int128_column.Push("128");
    double_column.Push("3.5");
    string_column.Push("text");
    timestamp_column.Push("2013-07-14 20:38:47");
    date_column.Push("2013-07-15");

    EXPECT_EQ(int16_column.ToString(0), "16");
    EXPECT_EQ(int32_column.ToString(0), "32");
    EXPECT_EQ(int64_column.ToString(0), "64");
    EXPECT_EQ(int128_column.ToString(0), "128");
    EXPECT_EQ(double_column.ToString(0), "3.500000");
    EXPECT_EQ(string_column.ToString(0), "text");
    EXPECT_EQ(timestamp_column.ToString(0), "2013-07-14 20:38:47");
    EXPECT_EQ(date_column.ToString(0), "2013-07-15");
}

TEST(SimpleDataStructuresUsageTest, BatchBuildsRowsFromScheme) {
    Scheme scheme = MakeScheme({{"id", "int64"}, {"name", "string"}});
    Batch batch(scheme, false);

    batch.AddRow({"1", "Alice"});
    batch.AddRow({"2", "Bob"});

    EXPECT_EQ(batch.HorizontalSize(), 2U);
    EXPECT_EQ(batch.VerticalSize(), 2U);
    EXPECT_EQ(batch.GetRow(1), (Row{"2", "Bob"}));
}

TEST(SimpleDataStructuresUsageTest, MetaDataDelegatesColumnLookupToScheme) {
    MetaData meta;
    meta.scheme.Add({"id", "int64"});
    meta.scheme.Add({"name", "string"});

    EXPECT_EQ(meta.GetColumnIndexByName("name"), 1U);
}

TEST_F(SimpleUsageTest, CsvReaderAndCsvWriterRoundTripRows) {
    {
        CsvWriter writer(Path("rows.csv").string());
        writer.WriteRow({"1", "Alice"});
        writer.WriteRow({"2", "Bob, Jr."});
    }

    EXPECT_EQ(ReadCsvRows("rows.csv"),
              (std::vector<Row>{{"1", "Alice"}, {"2", "Bob, Jr."}}));
}

TEST(SimpleUtilsUsageTest, StringConverterQuotesCsvFields) {
    StringConverter converter;

    EXPECT_EQ(converter.TransformStringToFilestring("he said \"hi\""),
              "\"he said \"\"hi\"\"\"");
}

TEST_F(SimpleUsageTest, ColumnarWriterReaderAndIoScannerReadBatches) {
    Scheme scheme = MakeScheme({{"id", "int64"}, {"name", "string"}});
    Batch batch(scheme, false);
    batch.AddRow({"1", "Alice"});
    batch.AddRow({"2", "Bob"});

    {
        ColumnarWriter writer(Path("data.hub").string());
        writer.WriteChunk(batch);
        std::move(writer).Close(scheme);
    }

    ColumnarReader reader(Path("data.hub").string());
    IoScanner scanner(reader.GetScheme(), reader);
    Batch read = scanner.ReadNext();

    EXPECT_EQ(read.GetRow(0), (Row{"1", "Alice"}));
    EXPECT_EQ(read.GetRow(1), (Row{"2", "Bob"}));
    EXPECT_TRUE(scanner.IsEnd());
}

TEST(SimpleIoUsageTest, VectorScannerReadsInMemoryBatches) {
    Scheme scheme = MakeScheme({{"id", "int64"}});
    Batch first(scheme, false);
    first.AddRow({"1"});
    Batch second(scheme, false);
    second.AddRow({"2"});

    VectorScanner scanner(std::vector<Batch>{first, second});

    EXPECT_EQ(scanner.ReadNext().GetRow(0), (Row{"1"}));
    EXPECT_EQ(scanner.ReadNext().GetRow(0), (Row{"2"}));
    EXPECT_TRUE(scanner.IsEnd());
    scanner.Reset();
    EXPECT_FALSE(scanner.IsEnd());
}

TEST(SimpleCoreUsageTest, ProjectionBuilderMapsSourceColumnsToReadScheme) {
    Scheme source;
    for (size_t i = 0; i <= hits::Col(hits::HitsColumn::URL); ++i) {
        source.Add({std::string(hits::HitsColumnNames[i]), "string"});
    }
    ProjectionBuilder projection(source);

    const size_t watch_id = projection.Require(hits::HitsColumn::WatchID);
    const size_t url = projection.Require(hits::HitsColumn::URL);

    EXPECT_EQ(watch_id, 0U);
    EXPECT_EQ(url, 1U);
    EXPECT_EQ(projection.ReadScheme().GiveRows(),
              (std::vector<Row>{{"WatchID", "string"}, {"URL", "string"}}));
}

TEST(SimpleOperationsUsageTest,
     FilterMarksRowsAndAggregationConsumesEnabledRows) {
    Scheme scheme = MakeScheme({{"id", "int64"}, {"value", "int64"}});
    Batch batch(scheme, false);
    batch.AddRow({"1", "10"});
    batch.AddRow({"2", "20"});

    Filter filter({MakeInt64GreaterOrEqualFilter(1, 20)});
    filter.Execute(batch);

    Aggregation aggregation({MakeCountAgg(0, "rows"), MakeSumAgg(1, "sum")});
    aggregation.Process(batch);
    std::vector<Row> rows = ReadRows(std::move(aggregation).Finalize());

    EXPECT_FALSE(batch.IsRowEnabled(0));
    EXPECT_TRUE(batch.IsRowEnabled(1));
    EXPECT_EQ(rows, (std::vector<Row>{{"1", "20"}}));
}

TEST(SimpleOperationsUsageTest, TopKKeepsTheLargestRowsBySortKey) {
    Scheme scheme = MakeScheme({{"id", "int64"}, {"score", "int64"}});
    Batch batch(scheme, false);
    batch.AddRow({"1", "10"});
    batch.AddRow({"2", "30"});
    batch.AddRow({"3", "20"});

    TopK top_k({MakeDescendingSortKey(1)}, 2, scheme);
    top_k.Process(batch);

    EXPECT_EQ(ReadRows(std::move(top_k).Finalize()),
              (std::vector<Row>{{"2", "30"}, {"3", "20"}}));
}

TEST(SimpleOperationsUsageTest, GroupByAggregatesRowsPerKey) {
    Scheme scheme = MakeScheme({{"region", "int64"}, {"value", "int64"}});
    Batch batch(scheme, false);
    batch.AddRow({"1", "10"});
    batch.AddRow({"1", "20"});
    batch.AddRow({"2", "5"});

    GroupBy group_by(MakeGroupByTask({0}, {MakeGroupSum(1)}), scheme);
    group_by.Process(batch);

    std::vector<Row> rows = ReadRows(std::move(group_by).Finalize());
    std::sort(rows.begin(), rows.end());

    EXPECT_EQ(rows, (std::vector<Row>{{"1", "30"}, {"2", "5"}}));
}

TEST(SimpleOperationsUsageTest, OffsetSkipsRowsBeforeFinalizing) {
    Scheme scheme = MakeScheme({{"id", "int64"}});
    Batch batch(scheme, false);
    batch.AddRow({"1"});
    batch.AddRow({"2"});

    Offset offset(1);
    offset.Process(batch);

    EXPECT_EQ(ReadRows(std::move(offset).Finalize()),
              (std::vector<Row>{{"2"}}));
}

TEST(SimpleOperationsUsageTest, ExpressionAddsColumnsAndSelectAnswerKeepsSome) {
    Scheme scheme = MakeScheme({{"id", "int64"}, {"name", "string"}});
    Batch batch(scheme, false);
    batch.AddRow({"1", "Alice"});

    Expression expression({MakeLengthExpression(1, "name_len")});
    expression.Execute(batch);
    SelectAnswer select_answer({0, 2});
    select_answer.Execute(batch);

    EXPECT_EQ(batch.GetScheme().GetSchemeNames(),
              (std::vector<std::string>{"id", "name_len"}));
    EXPECT_EQ(batch.GetRow(0), (Row{"1", "5"}));
}

TEST_F(SimpleUsageTest, PipelineExecutesOperationsAndWritesCsv) {
    Scheme scheme = MakeScheme({{"id", "int64"}, {"name", "string"}});
    Batch batch(scheme, false);
    batch.AddRow({"1", "drop"});
    batch.AddRow({"2", "keep"});

    {
        ColumnarWriter writer(Path("pipeline.hub").string());
        writer.WriteChunk(batch);
        std::move(writer).Close(scheme);
    }

    ColumnarReader reader(Path("pipeline.hub").string());
    std::vector<std::unique_ptr<Operation>> operations;
    operations.push_back(std::make_unique<Filter>(
        std::vector<FilterTask>{MakeStringEqualFilter(1, "keep")}));
    operations.push_back(
        std::make_unique<SelectAnswer>(std::vector<size_t>{0, 1}));
    Pipeline pipeline(std::move(operations), reader, reader.GetScheme(),
                      Path("pipeline.csv").string());

    pipeline.Execute();

    EXPECT_EQ(ReadCsvRows("pipeline.csv"), (std::vector<Row>{{"2", "keep"}}));
}

TEST_F(SimpleUsageTest, EngineConvertsCsvToColumnarAndExportsRows) {
    WriteText("scheme.csv", "\"id\",\"int64\"\n\"name\",\"string\"\n");
    WriteText("data.csv", "\"1\",\"Alice\"\n\"2\",\"Bob\"\n");

    Engine engine(Path("data.csv").string(), Path("scheme.csv").string(),
                  Path("engine.hub").string());
    engine.TakeAll(Path("engine.csv").string());

    EXPECT_EQ(ReadCsvRows("engine.csv"),
              (std::vector<Row>{{"1", "Alice"}, {"2", "Bob"}}));
    EXPECT_EQ(ReadCsvRows("scheme_engine.csv"),
              (std::vector<Row>{{"id", "int64"}, {"name", "string"}}));
}
