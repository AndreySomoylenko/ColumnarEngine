#include "Engine.h"
#include "data_structures/Butch.h"
#include "data_structures/Scheme.h"
#include "io/CSVReader.h"
#include "io/CSVWriter.h"
#include "io/ColumnarReader.h"
#include "io/ColumnarWriter.h"
#include <filesystem>

Engine::Engine(const Filename &data, const Filename &scheme, const Filename &columnar) {
    CSVReader scheme_reader(scheme);

    Scheme t_scheme;

    Raw cur;

    while (!scheme_reader.IsEnd()) {
        scheme_reader.ReadNext(cur);
        if (cur.empty()) {
            continue;
        }
        t_scheme.Add(cur);
    }

    Butch tmp(t_scheme);

    CSVReader data_reader(data);

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
        tmp.AddRaw(cur);
    }

    if (tmp.VerticalSize() != 0) {
        writer.WriteChunk(tmp);
        tmp.Clear();
    }

    writer.Close(t_scheme);

    reader_ = ColumnarReader(columnar);
}

Engine::Engine(const Filename &columnar)
    : reader_(columnar) {}

void Engine::TakeAll(const Filename &result_name) {
    const std::filesystem::path result_path(result_name);
    const std::filesystem::path scheme_path = result_path.parent_path() / ("scheme_" + result_path.filename().string());

    CSVWriter data_writer(result_name);
    CSVWriter scheme_writer(scheme_path.string());

    reader_.Reset();

    while (!reader_.IsEnd()) {
        Butch tmp = reader_.ReadNext();

        for (size_t i = 0; i < tmp.VerticalSize(); ++i) {
            data_writer.WriteRaw(tmp.GetRaw(i));
        }
    }

    const Scheme &cur = reader_.GetScheme();

    auto to_write = cur.GiveRaws();

    for (auto &x : to_write) {
        scheme_writer.WriteRaw(x);
    }
}
