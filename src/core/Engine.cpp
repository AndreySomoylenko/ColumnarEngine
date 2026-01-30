#include "Engine.h"
#include "data_structures/Butch.h"
#include "data_structures/Scheme.h"
#include "io/CSVReader.h"
#include "io/CSVWriter.h"
#include "io/ColumnarReader.h"
#include "io/ColumnarWriter.h"

Engine::Engine(const Filename &data, const Filename &scheme, const Filename &columnar) {
    CSVReader scheme_reader(scheme);

    Scheme t_scheme;

    while (!scheme_reader.IsEnd()) {
        auto cur = scheme_reader.ReadNext();
        if (cur.empty()) {
            continue;
        }
        t_scheme.Add(cur);
    }

    Butch tmp(t_scheme);

    CSVReader data_reader(data);

    ColumnarWriter writer(columnar);

    while (!data_reader.IsEnd()) {
        auto cur = data_reader.ReadNext();
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
    CSVWriter data_writer(result_name);
    CSVWriter scheme_writer("scheme_" + result_name);

    reader_.Reset();

    while (!reader_.IsEnd()) {
        Butch tmp = reader_.ReadNext();

        for (size_t i = 0; i < tmp.VerticalSize(); ++i) {
            data_writer.WriteRaw(tmp.GetRaw(i));
        }
    }

    Scheme cur = reader_.GetScheme();

    auto to_write = cur.GiveRaws();

    for (auto &x : to_write) {
        scheme_writer.WriteRaw(x);
    }
}
