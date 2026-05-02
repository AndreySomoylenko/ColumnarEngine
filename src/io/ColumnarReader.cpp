#include "ColumnarReader.h"
#include "data_structures/ByteVector.h"
#include "data_structures/Column.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <stdexcept>
#include <string>

ColumnarReader::ColumnarReader(const std::string &columnar)
    : cur_butch_(0) {
    is_.rdbuf()->pubsetbuf(io_buffer_.data(), static_cast<std::streamsize>(io_buffer_.size()));
    is_.open(columnar, std::ios::binary);

    if (!is_.good()) {
        throw std::invalid_argument("You give me really bad file");
    }

    std::streamoff meta;
    is_.read(reinterpret_cast<char *>(&meta), sizeof(meta));
    data_.meta_section_start = std::streampos(meta);
    is_.seekg(meta, std::ios::beg);

    std::streamoff chunk_start;

    size_t chunk_count;
    is_.read(reinterpret_cast<char *>(&chunk_count), sizeof(chunk_count));
    for (size_t i = 0; i < chunk_count; ++i) {
        is_.read(reinterpret_cast<char *>(&chunk_start), sizeof(chunk_start));
        data_.chunk_metas.push_back(std::streampos(chunk_start));
    }

    while (is_.peek() != EOF) {
        size_t name_sz;
        is_.read(reinterpret_cast<char *>(&name_sz), sizeof(name_sz));

        std::string name(name_sz, ' ');
        is_.read(name.data(), name_sz);

        size_t type_sz;
        is_.read(reinterpret_cast<char *>(&type_sz), sizeof(type_sz));

        std::string type(type_sz, ' ');
        is_.read(type.data(), type_sz);

        data_.scheme.Add(Raw{name, type});
    }
}

Butch ColumnarReader::ReadNext() {
    is_.seekg(data_.chunk_metas[cur_butch_], std::ios::beg);

    Butch result(data_.scheme, false);

    auto names = data_.scheme.GetSchemeNames();
    auto types = data_.scheme.GetSchemeTypes();

    std::vector<std::streampos> columns_starts(names.size());

    for (size_t i = 0; i < names.size(); ++i) {
        std::streamoff column_start;
        is_.read(reinterpret_cast<char *>(&column_start), sizeof(column_start));
        columns_starts[i] = std::streampos(column_start);
    }

    for (size_t i = 0; i < names.size(); ++i) {
        is_.seekg(columns_starts[i], std::ios::beg);

        std::streamoff column_size;
        if (i == names.size() - 1) {
            if (cur_butch_ == data_.chunk_metas.size() - 1) {
                column_size = data_.meta_section_start - columns_starts[i];
            } else {
                column_size = data_.chunk_metas[cur_butch_ + 1] - columns_starts[i];
            }
        } else {
            column_size = columns_starts[i + 1] - columns_starts[i];
        }

        if (types[i] == ColumnTypes::Int64) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data(col_size / 8, col_size, malloc(col_size));
            is_.read(const_cast<char *>(data.Data()), col_size);
            result.GetColumns()[i] = std::make_shared<Int64Column>(std::move(data));
        } else if (types[i] == ColumnTypes::String || types[i] == ColumnTypes::Unknown) {
            size_t data_sz;
            is_.read(reinterpret_cast<char *>(&data_sz), sizeof(data_sz));
            void *buffer = malloc(data_sz);
            is_.read(static_cast<char *>(buffer), data_sz);

            size_t offsets_sz;
            is_.read(reinterpret_cast<char *>(&offsets_sz), sizeof(offsets_sz));
            std::vector<size_t> offsets(offsets_sz / sizeof(size_t));
            is_.read(reinterpret_cast<char *>(offsets.data()), offsets_sz);

            ByteVector data = ByteVector(offsets.size(), data_sz, buffer);
            result.GetColumns()[i] = std::make_shared<StringColumn>(std::move(data), std::move(offsets));
        }
    }

    ++cur_butch_;

    return result;
}

void ColumnarReader::Reset() { cur_butch_ = 0; }

bool ColumnarReader::IsEnd() { return cur_butch_ >= data_.chunk_metas.size(); }

ColumnarReader::~ColumnarReader() { is_.close(); }

const Scheme &ColumnarReader::GetScheme() const { return data_.scheme; }
