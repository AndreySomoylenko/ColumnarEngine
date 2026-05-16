#include "core/Engine.h"
#include "core/Pipeline.h"
#include "core/ProjectionBuilder.h"
#include "data_structures/Batch.h"
#include "data_structures/Scheme.h"
#include "io/ColumnarReader.h"
#include "io/ColumnarWriter.h"
#include "io/CsvReader.h"
#include "io/CsvWriter.h"
#include "io/IoScanner.h"
#include "operations/Operations.h"
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <vector>

namespace {

Scheme MakeScheme(std::initializer_list<Row> rows) {
    Scheme scheme;
    for (const Row &row : rows) {
        scheme.Add(row);
    }
    return scheme;
}

Scheme CopyScheme(const Scheme &scheme) {
    Scheme result;
    for (const Row &row : scheme.GiveRows()) {
        result.Add(row);
    }
    return result;
}

} // namespace

Engine::Engine(const Filename &data, const Filename &scheme,
               const Filename &columnar) {
    CsvReader scheme_reader(scheme);

    Scheme t_scheme;

    Row cur;

    while (!scheme_reader.IsEnd()) {
        scheme_reader.ReadNext(cur);
        if (cur.empty()) {
            continue;
        }
        t_scheme.Add(cur);
    }

    Batch tmp(t_scheme);

    CsvReader data_reader(data);

    ColumnarWriter writer(columnar);

    cur.reserve(4096);

    while (!data_reader.IsEnd()) {
        cur.clear();

        data_reader.ReadNext(cur);

        if (cur.empty()) {
            continue;
        }

        if (!tmp.EnableToPush()) {
            writer.WriteChunk(tmp);
            tmp.Clear();
        }
        tmp.AddRow(cur);
    }

    if (tmp.VerticalSize() != 0) {
        writer.WriteChunk(tmp);
        tmp.Clear();
    }

    std::move(writer).Close(t_scheme);

    reader_ = ColumnarReader(columnar);
}

Engine::Engine(const Filename &columnar) : reader_(columnar) {}

void Engine::TakeAll(const Filename &result_name) {
    const Scheme &scheme = reader_.GetScheme();

    IoScanner scanner(scheme, reader_);
    CsvWriter result_writer(result_name);

    while (!scanner.IsEnd()) {
        Batch chunk = scanner.ReadNext();
        for (size_t row = 0; row < chunk.VerticalSize(); ++row) {
            result_writer.WriteRow(chunk.GetRow(row));
        }
    }

    const std::filesystem::path result_path(result_name);
    const std::filesystem::path scheme_path =
        result_path.parent_path() /
        ("scheme_" + result_path.filename().string());
    CsvWriter scheme_writer(scheme_path.string());
    for (const Row &row : scheme.GiveRows()) {
        scheme_writer.WriteRow(row);
    }
}

void Engine::Execute(Pipeline &pipeline) { pipeline.Execute(); }

void Engine::ExecuteClickBench() {
    Make01Querry();
    Make02Querry();
    Make03Querry();
    Make04Querry();
    Make05Querry();
    Make06Querry();
    Make07Querry();
    Make08Querry();
    Make09Querry();
    Make10Querry();
    Make11Querry();
    Make12Querry();
    Make13Querry();
    Make14Querry();
    Make15Querry();
    Make16Querry();
    Make17Querry();
    Make18Querry();
    Make19Querry();
    Make20Querry();
    Make21Querry();
    Make22Querry();
    Make23Querry();
    Make24Querry();
    Make25Querry();
    Make26Querry();
    Make27Querry();
    Make28Querry();
    Make29Querry();
    Make30Querry();
    Make31Querry();
    Make32Querry();
    Make33Querry();
    Make34Querry();
    Make35Querry();
    Make36Querry();
    Make37Querry();
    Make38Querry();
    Make39Querry();
    Make40Querry();
    Make41Querry();
    Make42Querry();
    Make43Querry();
}

void Engine::Make01Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t adv_engine_id =
        projection.Require(hits::HitsColumn::AdvEngineID);

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Aggregation>(
        MakeAggregation({MakeCountAgg(adv_engine_id, "count")})));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query01.csv");
    Execute(pipeline);
}

void Engine::Make02Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t adv_engine_id =
        projection.Require(hits::HitsColumn::AdvEngineID);

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64NotEqualFilter(adv_engine_id, 0)})));
    operations.emplace_back(std::make_unique<Aggregation>(
        MakeAggregation({MakeCountAgg(adv_engine_id, "count")})));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query02.csv");
    Execute(pipeline);
}

void Engine::Make03Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t adv_engine_id =
        projection.Require(hits::HitsColumn::AdvEngineID);
    const size_t resolution_width =
        projection.Require(hits::HitsColumn::ResolutionWidth);

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Aggregation>(MakeAggregation(
        {MakeSumAgg(adv_engine_id, "sum_adv_engine_id"),
         MakeCountAgg(adv_engine_id, "count"),
         MakeAvgAgg(resolution_width, "avg_resolution_width")})));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query03.csv");
    Execute(pipeline);
}

void Engine::Make04Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Aggregation>(
        MakeAggregation({MakeAvgAgg(user_id, "avg_user_id")})));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query04.csv");
    Execute(pipeline);
}

void Engine::Make05Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Aggregation>(
        MakeAggregation({MakeCountDistinctAgg(user_id, "count_distinct")})));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query05.csv");
    Execute(pipeline);
}

void Engine::Make06Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Aggregation>(MakeAggregation(
        {MakeCountDistinctAgg(search_phrase, "count_distinct")})));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query06.csv");
    Execute(pipeline);
}

void Engine::Make07Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t event_date = projection.Require(hits::HitsColumn::EventDate);

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Aggregation>(MakeAggregation(
        {MakeMinAgg(event_date, ColumnTypes::Date, "min_event_date"),
         MakeMaxAgg(event_date, ColumnTypes::Date, "max_event_date")})));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query07.csv");
    Execute(pipeline);
}

void Engine::Make08Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t adv_engine_id =
        projection.Require(hits::HitsColumn::AdvEngineID);

    Scheme result_scheme =
        MakeScheme({{"AdvEngineID", "int16"}, {"result_count", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64NotEqualFilter(adv_engine_id, 0)})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({adv_engine_id}, {MakeGroupCount()}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 100000, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query08.csv");
    Execute(pipeline);
}

void Engine::Make09Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t region_id = projection.Require(hits::HitsColumn::RegionID);
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    Scheme result_scheme =
        MakeScheme({{"RegionID", "int32"}, {"result_count_distinct", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({region_id}, {MakeGroupCountDistinct(user_id)}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query09.csv");
    Execute(pipeline);
}

void Engine::Make10Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t region_id = projection.Require(hits::HitsColumn::RegionID);
    const size_t adv_engine_id =
        projection.Require(hits::HitsColumn::AdvEngineID);
    const size_t resolution_width =
        projection.Require(hits::HitsColumn::ResolutionWidth);
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    Scheme result_scheme = MakeScheme({{"RegionID", "int32"},
                                       {"result_sum", "int128"},
                                       {"result_count", "int64"},
                                       {"result_avg", "int128"},
                                       {"result_count_distinct", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({region_id},
                        {MakeGroupSum(adv_engine_id), MakeGroupCount(),
                         MakeGroupAvg(resolution_width),
                         MakeGroupCountDistinct(user_id)}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query10.csv");
    Execute(pipeline);
}

void Engine::Make11Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t mobile_phone_model =
        projection.Require(hits::HitsColumn::MobilePhoneModel);
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    Scheme result_scheme = MakeScheme(
        {{"MobilePhoneModel", "string"}, {"result_count_distinct", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(mobile_phone_model, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({mobile_phone_model},
                                    {MakeGroupCountDistinct(user_id)}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1), MakeDescendingSortKey(0)}, 10,
                 result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query11.csv");
    Execute(pipeline);
}

void Engine::Make12Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t mobile_phone =
        projection.Require(hits::HitsColumn::MobilePhone);
    const size_t mobile_phone_model =
        projection.Require(hits::HitsColumn::MobilePhoneModel);
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    Scheme result_scheme = MakeScheme({{"MobilePhone", "int16"},
                                       {"MobilePhoneModel", "string"},
                                       {"result_count_distinct", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(mobile_phone_model, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({mobile_phone, mobile_phone_model},
                                    {MakeGroupCountDistinct(user_id)}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2), MakeDescendingSortKey(0),
                  MakeDescendingSortKey(1)},
                 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query12.csv");
    Execute(pipeline);
}

void Engine::Make13Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);

    Scheme result_scheme =
        MakeScheme({{"SearchPhrase", "string"}, {"result_count", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({search_phrase}, {MakeGroupCount()}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query13.csv");
    Execute(pipeline);
}

void Engine::Make14Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    Scheme result_scheme = MakeScheme(
        {{"SearchPhrase", "string"}, {"result_count_distinct", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({search_phrase}, {MakeGroupCountDistinct(user_id)}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query14.csv");
    Execute(pipeline);
}

void Engine::Make15Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t search_engine_id =
        projection.Require(hits::HitsColumn::SearchEngineID);
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);

    Scheme result_scheme = MakeScheme({{"SearchEngineID", "int16"},
                                       {"SearchPhrase", "string"},
                                       {"result_count", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({search_engine_id, search_phrase}, {MakeGroupCount()}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query15.csv");
    Execute(pipeline);
}

void Engine::Make16Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    Scheme result_scheme =
        MakeScheme({{"UserID", "int64"}, {"result_count", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({user_id}, {MakeGroupCount()}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query16.csv");
    Execute(pipeline);
}

void Engine::Make17Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);

    Scheme result_scheme = MakeScheme({{"UserID", "int64"},
                                       {"SearchPhrase", "string"},
                                       {"result_count", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({user_id, search_phrase}, {MakeGroupCount()}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query17.csv");
    Execute(pipeline);
}

void Engine::Make18Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);

    Scheme result_scheme = MakeScheme({{"UserID", "int64"},
                                       {"SearchPhrase", "string"},
                                       {"result_count", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({user_id, search_phrase}, {MakeGroupCount()}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeAscendingSortKey(0), MakeAscendingSortKey(1)}, 10,
                 result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query18.csv");
    Execute(pipeline);
}

void Engine::Make19Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);
    const size_t event_time = projection.Require(hits::HitsColumn::EventTime);
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    const size_t minute_column = 3;

    Scheme result_scheme = MakeScheme({{"UserID", "int64"},
                                       {"m", "int64"},
                                       {"SearchPhrase", "string"},
                                       {"result_count", "int64"}});

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Expression>(
        MakeExpression({MakeExtractMinuteExpression(event_time, "m")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({user_id, minute_column, search_phrase},
                                    {MakeGroupCount()}),
                    MakeScheme({{"UserID", "int64"},
                                {"EventTime", "timestamp"},
                                {"SearchPhrase", "string"},
                                {"m", "int64"}}))));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(3), MakeAscendingSortKey(0),
                  MakeAscendingSortKey(1), MakeAscendingSortKey(2)},
                 10, result_scheme)));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query19.csv");
    Execute(pipeline);
}

void Engine::Make20Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);

    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64EqualFilter(user_id, 435090932899640449LL)})));

    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query20.csv");
    Execute(pipeline);
}

void Engine::Make21Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t url = projection.Require(hits::HitsColumn::URL);
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringLikeFilter(url, "%google%")})));
    operations.emplace_back(std::make_unique<Aggregation>(
        MakeAggregation({MakeCountAgg(url, "count")})));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query21.csv");
    Execute(pipeline);
}

void Engine::Make22Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    const size_t url = projection.Require(hits::HitsColumn::URL);
    Scheme result_scheme = MakeScheme({{"SearchPhrase", "string"},
                                       {"result_min", "string"},
                                       {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringLikeFilter(url, "%google%"),
                    MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({search_phrase}, {MakeGroupMin(url), MakeGroupCount()}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query22.csv");
    Execute(pipeline);
}

void Engine::Make23Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    const size_t url = projection.Require(hits::HitsColumn::URL);
    const size_t title = projection.Require(hits::HitsColumn::Title);
    const size_t user_id = projection.Require(hits::HitsColumn::UserID);
    Scheme result_scheme = MakeScheme({{"SearchPhrase", "string"},
                                       {"result_min", "string"},
                                       {"result_min", "string"},
                                       {"result_count", "int64"},
                                       {"result_count_distinct", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringLikeFilter(title, "%Google%"),
                    MakeStringNotLikeFilter(url, "%.google.%"),
                    MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({search_phrase},
                        {MakeGroupMin(url), MakeGroupMin(title),
                         MakeGroupCount(), MakeGroupCountDistinct(user_id)}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(3)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query23.csv");
    Execute(pipeline);
}

void Engine::Make24Querry() {
    Scheme read_scheme = CopyScheme(reader_.GetScheme());
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(
        std::make_unique<Filter>(MakeFilter({MakeStringLikeFilter(
            static_cast<size_t>(hits::HitsColumn::URL), "%google%")})));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeAscendingSortKey(
                     static_cast<size_t>(hits::HitsColumn::EventTime))},
                 10, read_scheme)));
    Pipeline pipeline(std::move(operations), reader_, read_scheme,
                      "query24.csv");
    Execute(pipeline);
}

void Engine::Make25Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t event_time = projection.Require(hits::HitsColumn::EventTime);
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    Scheme result_scheme =
        MakeScheme({{"EventTime", "timestamp"}, {"SearchPhrase", "string"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeAscendingSortKey(event_time)}, 10, result_scheme)));
    operations.emplace_back(
        std::make_unique<SelectAnswer>(MakeSelectAnswer({search_phrase})));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query25.csv");
    Execute(pipeline);
}

void Engine::Make26Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    Scheme result_scheme = MakeScheme({{"SearchPhrase", "string"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeAscendingSortKey(search_phrase)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query26.csv");
    Execute(pipeline);
}

void Engine::Make27Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t event_time = projection.Require(hits::HitsColumn::EventTime);
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    Scheme result_scheme =
        MakeScheme({{"EventTime", "timestamp"}, {"SearchPhrase", "string"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<TopK>(MakeTopK(
        {MakeAscendingSortKey(event_time), MakeAscendingSortKey(search_phrase)},
        10, result_scheme)));
    operations.emplace_back(
        std::make_unique<SelectAnswer>(MakeSelectAnswer({search_phrase})));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query27.csv");
    Execute(pipeline);
}

void Engine::Make28Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t counter_id = projection.Require(hits::HitsColumn::CounterID);
    const size_t url = projection.Require(hits::HitsColumn::URL);
    const size_t url_len = 2;
    Scheme expr_scheme =
        MakeScheme({{"CounterID", "int32"}, {"URL", "string"}, {"l", "int64"}});
    Scheme result_scheme = MakeScheme({{"CounterID", "int32"},
                                       {"result_avg", "int128"},
                                       {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(url, "")})));
    operations.emplace_back(std::make_unique<Expression>(
        MakeExpression({MakeLengthExpression(url, "l")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({counter_id},
                                    {MakeGroupAvg(url_len), MakeGroupCount()}),
                    expr_scheme)));
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64GreaterFilter(2, 100000)})));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 25, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query28.csv");
    Execute(pipeline);
}

void Engine::Make29Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t referer = projection.Require(hits::HitsColumn::Referer);
    const size_t domain = 1;
    const size_t referer_len = 2;
    Scheme expr_scheme =
        MakeScheme({{"Referer", "string"}, {"k", "string"}, {"l", "int64"}});
    Scheme result_scheme = MakeScheme({{"k", "string"},
                                       {"result_avg", "int128"},
                                       {"result_count", "int64"},
                                       {"result_min", "string"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(referer, "")})));
    operations.emplace_back(std::make_unique<Expression>(
        MakeExpression({MakeExtractDomainExpression(referer, "k"),
                        MakeLengthExpression(referer, "l")})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({domain}, {MakeGroupAvg(referer_len), MakeGroupCount(),
                                   MakeGroupMin(referer)}),
        expr_scheme)));
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64GreaterFilter(2, 100000)})));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 25, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query29.csv");
    Execute(pipeline);
}

void Engine::Make30Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t width = projection.Require(hits::HitsColumn::ResolutionWidth);
    std::vector<ExpressionTask> result_expressions;
    result_expressions.reserve(89);
    for (int64_t i = 1; i <= 89; ++i) {
        result_expressions.emplace_back(MakeInt128AddInt64ProductExpression(
            0, 1, i, "sum_" + std::to_string(i)));
    }
    std::vector<size_t> answer_columns;
    answer_columns.reserve(90);
    answer_columns.emplace_back(0);
    for (size_t i = 2; i <= 90; ++i) {
        answer_columns.emplace_back(i);
    }
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Aggregation>(MakeAggregation(
        {MakeSumAgg(width, "sum_0"), MakeCountAgg(width, "count")})));
    operations.emplace_back(std::make_unique<Expression>(
        MakeExpression(std::move(result_expressions))));
    operations.emplace_back(std::make_unique<SelectAnswer>(
        MakeSelectAnswer(std::move(answer_columns))));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query30.csv");
    Execute(pipeline);
}

void Engine::Make31Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t search_engine_id =
        projection.Require(hits::HitsColumn::SearchEngineID);
    const size_t client_ip = projection.Require(hits::HitsColumn::ClientIP);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t width = projection.Require(hits::HitsColumn::ResolutionWidth);
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    Scheme result_scheme = MakeScheme({{"SearchEngineID", "int16"},
                                       {"ClientIP", "int32"},
                                       {"result_count", "int64"},
                                       {"result_sum", "int128"},
                                       {"result_avg", "int128"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({search_engine_id, client_ip},
                                    {MakeGroupCount(), MakeGroupSum(is_refresh),
                                     MakeGroupAvg(width)}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query31.csv");
    Execute(pipeline);
}

void Engine::Make32Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t watch_id = projection.Require(hits::HitsColumn::WatchID);
    const size_t client_ip = projection.Require(hits::HitsColumn::ClientIP);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t width = projection.Require(hits::HitsColumn::ResolutionWidth);
    const size_t search_phrase =
        projection.Require(hits::HitsColumn::SearchPhrase);
    Scheme result_scheme = MakeScheme({{"WatchID", "int64"},
                                       {"ClientIP", "int32"},
                                       {"result_count", "int64"},
                                       {"result_sum", "int128"},
                                       {"result_avg", "int128"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeStringNotEqualFilter(search_phrase, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({watch_id, client_ip},
                                    {MakeGroupCount(), MakeGroupSum(is_refresh),
                                     MakeGroupAvg(width)}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query32.csv");
    Execute(pipeline);
}

void Engine::Make33Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t watch_id = projection.Require(hits::HitsColumn::WatchID);
    const size_t client_ip = projection.Require(hits::HitsColumn::ClientIP);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t width = projection.Require(hits::HitsColumn::ResolutionWidth);
    Scheme result_scheme = MakeScheme({{"WatchID", "int64"},
                                       {"ClientIP", "int32"},
                                       {"result_count", "int64"},
                                       {"result_sum", "int128"},
                                       {"result_avg", "int128"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({watch_id, client_ip},
                                    {MakeGroupCount(), MakeGroupSum(is_refresh),
                                     MakeGroupAvg(width)}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query33.csv");
    Execute(pipeline);
}

void Engine::Make34Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t url = projection.Require(hits::HitsColumn::URL);
    Scheme result_scheme =
        MakeScheme({{"URL", "string"}, {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({url}, {MakeGroupCount()}), projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query34.csv");
    Execute(pipeline);
}

void Engine::Make35Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t url = projection.Require(hits::HitsColumn::URL);
    const size_t one = 1;
    Scheme expr_scheme = MakeScheme({{"URL", "string"}, {"one", "int64"}});
    Scheme result_scheme = MakeScheme(
        {{"one", "int64"}, {"URL", "string"}, {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Expression>(
        MakeExpression({MakeConstantInt64Expression(1, "one")})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({one, url}, {MakeGroupCount()}), expr_scheme)));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query35.csv");
    Execute(pipeline);
}

void Engine::Make36Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t client_ip = projection.Require(hits::HitsColumn::ClientIP);
    Scheme expr_scheme = MakeScheme({{"ClientIP", "int32"},
                                     {"ip_1", "int64"},
                                     {"ip_2", "int64"},
                                     {"ip_3", "int64"}});
    Scheme result_scheme = MakeScheme({{"ClientIP", "int32"},
                                       {"ip_1", "int64"},
                                       {"ip_2", "int64"},
                                       {"ip_3", "int64"},
                                       {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Expression>(MakeExpression(
        {MakeSubInt64ConstantExpression(client_ip, 1, "ip_1"),
         MakeSubInt64ConstantExpression(client_ip, 2, "ip_2"),
         MakeSubInt64ConstantExpression(client_ip, 3, "ip_3")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({client_ip, 1, 2, 3}, {MakeGroupCount()}),
                    expr_scheme)));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(4)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query36.csv");
    Execute(pipeline);
}

void Engine::Make37Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t counter_id = projection.Require(hits::HitsColumn::CounterID);
    const size_t event_date = projection.Require(hits::HitsColumn::EventDate);
    const size_t dont_count =
        projection.Require(hits::HitsColumn::DontCountHits);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t url = projection.Require(hits::HitsColumn::URL);
    Scheme result_scheme =
        MakeScheme({{"URL", "string"}, {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64EqualFilter(counter_id, 62),
                    MakeDateRangeFilter(event_date, "2013-07-01", "2013-07-31"),
                    MakeInt64EqualFilter(dont_count, 0),
                    MakeInt64EqualFilter(is_refresh, 0),
                    MakeStringNotEqualFilter(url, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({url}, {MakeGroupCount()}), projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query37.csv");
    Execute(pipeline);
}

void Engine::Make38Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t counter_id = projection.Require(hits::HitsColumn::CounterID);
    const size_t event_date = projection.Require(hits::HitsColumn::EventDate);
    const size_t dont_count =
        projection.Require(hits::HitsColumn::DontCountHits);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t title = projection.Require(hits::HitsColumn::Title);
    Scheme result_scheme =
        MakeScheme({{"Title", "string"}, {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64EqualFilter(counter_id, 62),
                    MakeDateRangeFilter(event_date, "2013-07-01", "2013-07-31"),
                    MakeInt64EqualFilter(dont_count, 0),
                    MakeInt64EqualFilter(is_refresh, 0),
                    MakeStringNotEqualFilter(title, "")})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({title}, {MakeGroupCount()}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 10, result_scheme)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query38.csv");
    Execute(pipeline);
}

void Engine::Make39Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t counter_id = projection.Require(hits::HitsColumn::CounterID);
    const size_t event_date = projection.Require(hits::HitsColumn::EventDate);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t is_link = projection.Require(hits::HitsColumn::IsLink);
    const size_t is_download = projection.Require(hits::HitsColumn::IsDownload);
    const size_t url = projection.Require(hits::HitsColumn::URL);
    Scheme result_scheme =
        MakeScheme({{"URL", "string"}, {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64EqualFilter(counter_id, 62),
                    MakeDateRangeFilter(event_date, "2013-07-01", "2013-07-31"),
                    MakeInt64EqualFilter(is_refresh, 0),
                    MakeInt64NotEqualFilter(is_link, 0),
                    MakeInt64EqualFilter(is_download, 0)})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({url}, {MakeGroupCount()}), projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(1)}, 1010, result_scheme)));
    operations.emplace_back(std::make_unique<Offset>(MakeOffset(1000)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query39.csv");
    Execute(pipeline);
}

void Engine::Make40Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t trafic_source =
        projection.Require(hits::HitsColumn::TraficSourceID);
    const size_t search_engine =
        projection.Require(hits::HitsColumn::SearchEngineID);
    const size_t adv_engine = projection.Require(hits::HitsColumn::AdvEngineID);
    const size_t referer = projection.Require(hits::HitsColumn::Referer);
    const size_t url = projection.Require(hits::HitsColumn::URL);
    const size_t counter_id = projection.Require(hits::HitsColumn::CounterID);
    const size_t event_date = projection.Require(hits::HitsColumn::EventDate);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t src = 8;
    const size_t dst = 9;
    Scheme expr_scheme = MakeScheme({{"TraficSourceID", "int16"},
                                     {"SearchEngineID", "int16"},
                                     {"AdvEngineID", "int16"},
                                     {"Referer", "string"},
                                     {"URL", "string"},
                                     {"CounterID", "int32"},
                                     {"EventDate", "date"},
                                     {"IsRefresh", "int16"},
                                     {"Src", "string"},
                                     {"Dst", "string"}});
    Scheme result_scheme = MakeScheme({{"TraficSourceID", "int16"},
                                       {"SearchEngineID", "int16"},
                                       {"AdvEngineID", "int16"},
                                       {"Src", "string"},
                                       {"Dst", "string"},
                                       {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64EqualFilter(counter_id, 62),
                    MakeDateRangeFilter(event_date, "2013-07-01", "2013-07-31"),
                    MakeInt64EqualFilter(is_refresh, 0)})));
    operations.emplace_back(std::make_unique<Expression>(MakeExpression(
        {MakeCaseWhenBothZeroThenStringElseEmptyExpression(
             search_engine, adv_engine, referer, "Src"),
         MakeCopyColumnExpression(url, "Dst", ColumnTypes::String)})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({trafic_source, search_engine, adv_engine, src, dst},
                        {MakeGroupCount()}),
        expr_scheme)));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(5)}, 1010, result_scheme)));
    operations.emplace_back(std::make_unique<Offset>(MakeOffset(1000)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query40.csv");
    Execute(pipeline);
}

void Engine::Make41Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t url_hash = projection.Require(hits::HitsColumn::URLHash);
    const size_t event_date = projection.Require(hits::HitsColumn::EventDate);
    const size_t counter_id = projection.Require(hits::HitsColumn::CounterID);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t trafic_source =
        projection.Require(hits::HitsColumn::TraficSourceID);
    const size_t referer_hash =
        projection.Require(hits::HitsColumn::RefererHash);
    Scheme result_scheme = MakeScheme({{"URLHash", "int64"},
                                       {"EventDate", "date"},
                                       {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(MakeFilter(
        {MakeInt64EqualFilter(counter_id, 62),
         MakeDateRangeFilter(event_date, "2013-07-01", "2013-07-31"),
         MakeInt64EqualFilter(is_refresh, 0),
         MakeInt64InFilter(trafic_source, {-1, 6}),
         MakeInt64EqualFilter(referer_hash, 3594120000172545465LL)})));
    operations.emplace_back(std::make_unique<GroupBy>(
        MakeGroupBy(MakeGroupByTask({url_hash, event_date}, {MakeGroupCount()}),
                    projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 110, result_scheme)));
    operations.emplace_back(std::make_unique<Offset>(MakeOffset(100)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query41.csv");
    Execute(pipeline);
}

void Engine::Make42Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t window_width =
        projection.Require(hits::HitsColumn::WindowClientWidth);
    const size_t window_height =
        projection.Require(hits::HitsColumn::WindowClientHeight);
    const size_t counter_id = projection.Require(hits::HitsColumn::CounterID);
    const size_t event_date = projection.Require(hits::HitsColumn::EventDate);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t dont_count =
        projection.Require(hits::HitsColumn::DontCountHits);
    const size_t url_hash = projection.Require(hits::HitsColumn::URLHash);
    Scheme result_scheme = MakeScheme({{"WindowClientWidth", "int16"},
                                       {"WindowClientHeight", "int16"},
                                       {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64EqualFilter(counter_id, 62),
                    MakeDateRangeFilter(event_date, "2013-07-01", "2013-07-31"),
                    MakeInt64EqualFilter(is_refresh, 0),
                    MakeInt64EqualFilter(dont_count, 0),
                    MakeInt64EqualFilter(url_hash, 2868770270353813622LL)})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({window_width, window_height}, {MakeGroupCount()}),
        projection.ReadScheme())));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeDescendingSortKey(2)}, 10010, result_scheme)));
    operations.emplace_back(std::make_unique<Offset>(MakeOffset(10000)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query42.csv");
    Execute(pipeline);
}

void Engine::Make43Querry() {
    ProjectionBuilder projection(reader_.GetScheme());
    const size_t event_time = projection.Require(hits::HitsColumn::EventTime);
    const size_t counter_id = projection.Require(hits::HitsColumn::CounterID);
    const size_t event_date = projection.Require(hits::HitsColumn::EventDate);
    const size_t is_refresh = projection.Require(hits::HitsColumn::IsRefresh);
    const size_t dont_count =
        projection.Require(hits::HitsColumn::DontCountHits);
    const size_t minute = 5;
    Scheme expr_scheme = MakeScheme({{"EventTime", "timestamp"},
                                     {"CounterID", "int32"},
                                     {"EventDate", "date"},
                                     {"IsRefresh", "int16"},
                                     {"DontCountHits", "int16"},
                                     {"M", "timestamp"}});
    Scheme result_scheme =
        MakeScheme({{"M", "timestamp"}, {"result_count", "int64"}});
    std::vector<std::unique_ptr<Operation>> operations;
    operations.emplace_back(std::make_unique<Filter>(
        MakeFilter({MakeInt64EqualFilter(counter_id, 62),
                    MakeDateRangeFilter(event_date, "2013-07-14", "2013-07-15"),
                    MakeInt64EqualFilter(is_refresh, 0),
                    MakeInt64EqualFilter(dont_count, 0)})));
    operations.emplace_back(std::make_unique<Expression>(
        MakeExpression({MakeDateTruncMinuteExpression(event_time, "M")})));
    operations.emplace_back(std::make_unique<GroupBy>(MakeGroupBy(
        MakeGroupByTask({minute}, {MakeGroupCount()}), expr_scheme)));
    operations.emplace_back(std::make_unique<TopK>(
        MakeTopK({MakeAscendingSortKey(0)}, 1010, result_scheme)));
    operations.emplace_back(std::make_unique<Offset>(MakeOffset(1000)));
    Pipeline pipeline(std::move(operations), reader_, projection.ReadScheme(),
                      "query43.csv");
    Execute(pipeline);
}
