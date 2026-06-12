#include "io/ColumnarWriter.h"
#include "data_structures/Column.h"
#include "utils/Compresser.h"
#include <cstddef>
#include <ios>
#include <memory>
#include <stdexcept>

ColumnarWriter::ColumnarWriter(const std::string &filename) {
    os_.open(filename, std::ios::binary | std::ios::out);

    if (!os_.good()) {
        throw std::runtime_error("Can't create columnar file");
    }

    batch_count = 0;
    std::streampos meta_offset = os_.tellp();
    std::streamoff meta_off = meta_offset;
    os_.write(reinterpret_cast<char *>(&meta_off), sizeof(meta_off));
}

void ColumnarWriter::WriteChunk(const Batch &batch) {
    std::vector<std::streampos> columns_batch_starts;
    ++batch_count;

    columns_batch_starts.emplace_back(os_.tellp());
    for (auto &x : batch.GetColumns()) {
        auto [data, sz] = x->ToWrite();

        auto type = x->GetColumnType();
        if (type == ColumnTypes::String || type == ColumnTypes::Unknown) {
            auto [value, meta] = Compression::CompressDictFromString(
                std::static_pointer_cast<const StringColumn>(x));

            size_t dict_size = meta.dict.SizeInBytes();
            size_t offsets_size = meta.offsets.size() * sizeof(size_t);
            size_t value_size = value.size();

            os_.write(reinterpret_cast<const char *>(&meta.bit_width),
                      sizeof(meta.bit_width));
            os_.write(reinterpret_cast<const char *>(&meta.real_size),
                      sizeof(meta.real_size));
            os_.write(reinterpret_cast<const char *>(&dict_size),
                      sizeof(size_t));
            os_.write(meta.dict.Data(), meta.dict.SizeInBytes());
            os_.write(reinterpret_cast<const char *>(&offsets_size),
                      sizeof(size_t));
            os_.write(reinterpret_cast<const char *>(meta.offsets.data()),
                      meta.offsets.size() * sizeof(size_t));
            os_.write(reinterpret_cast<const char *>(&value_size),
                      sizeof(value_size));
            os_.write(reinterpret_cast<const char *>(value.data()),
                      value.size());

        } else if (type == ColumnTypes::Int16 || type == ColumnTypes::Int32 ||
                   type == ColumnTypes::Int64) {
            const auto &[value, meta] =
                Compression::CompressIntTypesBitPacking(x);

            size_t value_size = value.size();
            os_.write(reinterpret_cast<const char *>(&meta.bit_width),
                      sizeof(meta.bit_width));
            os_.write(reinterpret_cast<const char *>(&meta.real_size),
                      sizeof(meta.real_size));
            os_.write(reinterpret_cast<const char *>(&value_size),
                      sizeof(value_size));
            os_.write(reinterpret_cast<const char *>(value.data()), value_size);

        } else {
            os_.write(data, sz);
        }
        columns_batch_starts.emplace_back(os_.tellp());
    }

    columns_batch_starts.pop_back();

    column_starts_.emplace_back(std::move(columns_batch_starts));

    for (size_t i = 0; i < columns_batch_starts.size(); ++i) {
        std::streamoff off = columns_batch_starts[i];
        os_.write(reinterpret_cast<char *>(&off), sizeof(off));
    }
}

void ColumnarWriter::Close(const Scheme &scheme) && {
    auto meta_start = os_.tellp();
    size_t columns_count = scheme.GetColumnsNumber();

    os_.write(reinterpret_cast<const char *>(&batch_count),
              sizeof(batch_count));
    os_.write(reinterpret_cast<const char *>(&columns_count),
              sizeof(columns_count));

    for (auto &x : column_starts_) {
        for (auto &y : x) {
            std::streamoff to_write = y;

            os_.write(reinterpret_cast<const char *>(&to_write),
                      sizeof(to_write));
        }
    }

    const auto &names = scheme.GetSchemeNames();
    const auto &types = scheme.GetSchemeTypes();

    for (size_t i = 0; i < names.size(); ++i) {
        size_t name_sz = names[i].size();
        os_.write(reinterpret_cast<char *>(&name_sz), sizeof(name_sz));
        os_.write(names[i].data(), names[i].size());

        std::string type = GetNameByType(types[i]);
        size_t type_sz = type.size();
        os_.write(reinterpret_cast<char *>(&type_sz), sizeof(type_sz));
        os_.write(type.data(), type.size());
    }

    os_.seekp(0, std::ios::beg);
    std::streamoff to_write = meta_start;
    os_.write(reinterpret_cast<char *>(&to_write), sizeof(to_write));

    os_.close();
}
