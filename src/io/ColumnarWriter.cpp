#include "ColumnarWriter.h"
#include "data_structures/Column.h"
#include <cstddef>
#include <ios>
#include <stdexcept>

ColumnarWriter::ColumnarWriter(const std::string &filename) {
    os_.rdbuf()->pubsetbuf(io_buffer_.data(), static_cast<std::streamsize>(io_buffer_.size()));
    os_.open(filename, std::ios::binary | std::ios::out);

    if (!os_.good()) {
        throw std::runtime_error("Can't create columnar file");
    }

    std::streampos meta_offset = os_.tellp();
    std::streamoff meta_off = meta_offset;
    os_.write(reinterpret_cast<char *>(&meta_off), sizeof(meta_off));
}

void ColumnarWriter::WriteChunk(const Butch &butch) {
    std::vector<std::streampos> columns_starts;

    columns_starts.emplace_back(os_.tellp());
    for (auto &x : butch.GetColumns()) {
        auto [data, sz] = x->ToWrite();

        if (x->GetColumnType() == ColumnTypes::String) {
            os_.write(reinterpret_cast<const char *>(&sz), sizeof(sz));
            os_.write(data, sz);
            size_t offsets_sz = x->GetOffsets().size() * sizeof(size_t);
            os_.write(reinterpret_cast<const char *>(&offsets_sz), sizeof(offsets_sz));
            os_.write(reinterpret_cast<const char *>(x->GetOffsets().data()), x->GetOffsets().size() * sizeof(size_t));
        } else {
            os_.write(data, sz);
        }
        columns_starts.emplace_back(os_.tellp());
    }

    columns_starts.pop_back();

    chunk_starts.emplace_back(os_.tellp());

    for (size_t i = 0; i < columns_starts.size(); ++i) {
        std::streamoff off = columns_starts[i];
        os_.write(reinterpret_cast<char *>(&off), sizeof(off));
    }
}

void ColumnarWriter::Close(const Scheme &scheme) {
    auto meta_start = os_.tellp();
    size_t chunk_count = chunk_starts.size();

    os_.write(reinterpret_cast<char *>(&chunk_count), sizeof(chunk_count));
    for (size_t i = 0; i < chunk_starts.size(); ++i) {
        std::streamoff to_write = chunk_starts[i];
        os_.write(reinterpret_cast<char *>(&to_write), sizeof(to_write));
    }

    auto names = scheme.GetSchemeNames();
    auto types = scheme.GetSchemeTypes();

    std::string int64 = "int64";
    std::string str = "string";
    std::string unknown = "unknown";

    size_t int64_sz = int64.size();
    size_t str_sz = str.size();
    size_t unknown_sz = unknown.size();

    for (size_t i = 0; i < names.size(); ++i) {
        size_t name_sz = names[i].size();
        os_.write(reinterpret_cast<char *>(&name_sz), sizeof(name_sz));
        os_.write(names[i].data(), names[i].size());

        if (types[i] == ColumnTypes::Int64) {
            os_.write(reinterpret_cast<char *>(&int64_sz), sizeof(int64_sz));
            os_.write(int64.data(), int64.size());
        } else if (types[i] == ColumnTypes::String) {
            os_.write(reinterpret_cast<char *>(&str_sz), sizeof(str_sz));
            os_.write(str.data(), str.size());
        } else {
            os_.write(reinterpret_cast<char *>(&unknown_sz), sizeof(unknown_sz));
            os_.write(unknown.data(), unknown.size());
        }
    }

    os_.seekp(0, std::ios::beg);
    std::streamoff to_write = meta_start;
    os_.write(reinterpret_cast<char *>(&to_write), sizeof(to_write));

    os_.close();
}
