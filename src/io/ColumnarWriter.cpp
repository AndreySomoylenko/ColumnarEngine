#include "ColumnarWriter.h"
#include "data_structures/Column.h"
#include <cstddef>
#include <ios>
#include <stdexcept>

ColumnarWriter::ColumnarWriter(const std::string &filename)
    : os_(filename, std::ios::binary | std::ios::out) {
    if (!os_.good()) {
        throw std::runtime_error("Can't create columnar file");
    }

    char *semicon = new char(';');

    uint64_t meta_offset = os_.tellp();
    os_.write(reinterpret_cast<char *>(&meta_offset), sizeof(meta_offset));
    os_.write(semicon, 1);
}

void ColumnarWriter::WriteChunk(const Butch &butch) {
    char coma = ',';
    char semicon = ';';

    std::vector<uint64_t> columns_starts;

    columns_starts.emplace_back(os_.tellp());
    for (auto &x : butch.GetColumns()) {
        for (size_t i = 0; i < butch.VerticalSize(); ++i) {

            auto to_write = x->ToWrite(i);

            os_.write(to_write.first, to_write.second);

            if (i != butch.VerticalSize() - 1) {
                os_.write(&coma, 1);
            } else {
                os_.write(&semicon, 1);
            }
        }
        columns_starts.emplace_back(os_.tellp());
    }

    columns_starts.pop_back();

    chunk_starts.emplace_back(os_.tellp());

    for (size_t i = 0; i < columns_starts.size(); ++i) {
        uint64_t to_write = static_cast<uint64_t>(columns_starts[i]);
        os_.write(reinterpret_cast<char *>(&to_write), sizeof(to_write));

        if (i != columns_starts.size() - 1) {
            os_.write(&coma, 1);
        } else {
            os_.write(&semicon, 1);
        }
    }
}

void ColumnarWriter::Close(const Scheme &scheme) {
    char coma = ',';
    char semicon = ';';
    auto meta_start = os_.tellp();
    for (size_t i = 0; i < chunk_starts.size(); ++i) {
        uint64_t to_write = static_cast<uint64_t>(chunk_starts[i]);
        os_.write(reinterpret_cast<char *>(&to_write), sizeof(to_write));

        if (i != chunk_starts.size() - 1) {
            os_.write(&coma, 1);
        } else {
            os_.write(&semicon, 1);
        }
    }

    if (chunk_starts.size() == 0) {
        os_.write(&semicon, 1);
    }

    auto names = scheme.GetSchemeNames();
    auto types = scheme.GetSchemeTypes();

    std::string int64 = "int64";
    std::string str = "string";
    std::string unknown = "unknown";

    for (size_t i = 0; i < names.size(); ++i) {
        os_.write(names[i].data(), names[i].size());

        os_.write(&coma, 1);

        if (types[i] == ColumnTypes::Int64) {
            os_.write(int64.data(), int64.size());
        } else if (types[i] == ColumnTypes::String) {
            os_.write(str.data(), str.size());
        } else {
            os_.write(unknown.data(), unknown.size());
        }

        if (i != names.size() - 1) {
            os_.write(&semicon, 1);
        }
    }

    os_.seekp(0, std::ios::beg);
    uint64_t to_write = static_cast<uint64_t>(meta_start);
    os_.write(reinterpret_cast<char *>(&to_write), sizeof(to_write));

    os_.close();
}