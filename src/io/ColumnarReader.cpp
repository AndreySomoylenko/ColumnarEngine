#include "ColumnarReader.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <stdexcept>
#include <string>

#include "data_structures/ByteVector.h"
#include "data_structures/Column.h"

ColumnarReader::ColumnarReader(const std::string &columnar) {
    is_.rdbuf()->pubsetbuf(io_buffer_.data(),
                           static_cast<std::streamsize>(io_buffer_.size()));
    is_.open(columnar, std::ios::binary);

    if (!is_.good()) {
        throw std::invalid_argument("You give me really bad file");
    }

    std::streamoff meta;
    if (!is_.read(reinterpret_cast<char *>(&meta), sizeof(meta))) {
        throw std::invalid_argument("You give me really bad file");
    }
    data_.meta_section_start = std::streampos(meta);
    is_.seekg(meta, std::ios::beg);

    if (!is_.good()) {
        throw std::invalid_argument("You give me really bad file");
    }

    std::streamoff chunk_start;

    size_t chunk_count;
    if (!is_.read(reinterpret_cast<char *>(&chunk_count),
                  sizeof(chunk_count))) {
        throw std::invalid_argument("You give me really bad file");
    }
    for (size_t i = 0; i < chunk_count; ++i) {
        if (!is_.read(reinterpret_cast<char *>(&chunk_start),
                      sizeof(chunk_start))) {
            throw std::invalid_argument("You give me really bad file");
        }
        data_.chunk_metas.push_back(std::streampos(chunk_start));
    }

    while (true) {
        const int next = is_.peek();
        if (next == EOF) {
            if (!is_.eof()) {
                throw std::invalid_argument("You give me really bad file");
            }
            break;
        }
        size_t name_sz;
        if (!is_.read(reinterpret_cast<char *>(&name_sz), sizeof(name_sz))) {
            throw std::invalid_argument("You give me really bad file");
        }

        std::string name(name_sz, ' ');
        if (!is_.read(name.data(), name_sz)) {
            throw std::invalid_argument("You give me really bad file");
        }

        size_t type_sz;
        if (!is_.read(reinterpret_cast<char *>(&type_sz), sizeof(type_sz))) {
            throw std::invalid_argument("You give me really bad file");
        }
        std::string type(type_sz, ' ');
        if (!is_.read(type.data(), type_sz)) {
            throw std::invalid_argument("You give me really bad file");
        }

        data_.scheme.Add(Raw{name, type});
    }

    data_.column_numbers = data_.scheme.GetSchemeNames().size();

    is_.clear();
}

Butch ColumnarReader::ReadNext(const std::vector<size_t> &columns_to_read,
                               size_t &cur_index) {
    if (IsEnd(cur_index)) {
        throw std::out_of_range("No more data to read");
    }

    const size_t columns_count = data_.scheme.GetSchemeNames().size();
    for (const size_t column : columns_to_read) {
        if (column >= columns_count) {
            throw std::out_of_range("Incorrect column index");
        }
    }

    is_.seekg(data_.chunk_metas[cur_index], std::ios::beg);

    Butch result(columns_to_read, data_.scheme, false);

    auto types = data_.scheme.GetSchemeTypes();

    std::vector<std::streampos> columns_starts(columns_count);

    for (size_t i = 0; i < columns_count; ++i) {
        std::streamoff column_start;
        if (!is_.read(reinterpret_cast<char *>(&column_start),
                      sizeof(column_start))) {
            throw std::invalid_argument("You give me really bad file");
        }
        columns_starts[i] = std::streampos(column_start);
    }

    for (size_t i = 0; i < columns_to_read.size(); ++i) {
        const size_t column_index = columns_to_read[i];
        is_.seekg(columns_starts[column_index], std::ios::beg);

        std::streamoff column_size;
        if (column_index + 1 == columns_count) {
            if (cur_index == data_.chunk_metas.size() - 1) {
                column_size = data_.meta_section_start -
                              columns_starts[column_index];
            } else {
                column_size = data_.chunk_metas[cur_index + 1] -
                              columns_starts[column_index];
            }
        } else {
            column_size =
                columns_starts[column_index + 1] - columns_starts[column_index];
        }

        if (column_size < 0) {
            throw std::invalid_argument("You give me really bad file");
        }

        if (types[column_index] == ColumnTypes::Int64) {
            size_t col_size = static_cast<size_t>(column_size);
            void *buffer = malloc(col_size);

            is_.read(static_cast<char *>(buffer), col_size);
            ByteVector data(col_size / 8, col_size, buffer);
            result.GetColumns()[i] =
                std::make_shared<Int64Column>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Timestamp ||
                   types[column_index] == ColumnTypes::Date) {
            size_t col_size = static_cast<size_t>(column_size);
            void *buffer = malloc(col_size);

            is_.read(static_cast<char *>(buffer), col_size);
            ByteVector data(
                col_size / sizeof(std::chrono::system_clock::time_point),
                col_size, buffer);
            result.GetColumns()[i] = std::make_shared<TimeColumn>(
                std::move(data), types[column_index] == ColumnTypes::Date);

        } else if (types[column_index] == ColumnTypes::String ||
                   types[column_index] == ColumnTypes::Unknown) {
            size_t data_sz;
            is_.read(reinterpret_cast<char *>(&data_sz), sizeof(data_sz));
            void *buffer = malloc(data_sz);

            is_.read(static_cast<char *>(buffer), data_sz);

            size_t offsets_sz;
            is_.read(reinterpret_cast<char *>(&offsets_sz), sizeof(offsets_sz));
            std::vector<size_t> offsets(offsets_sz / sizeof(size_t));
            is_.read(reinterpret_cast<char *>(offsets.data()), offsets_sz);

            ByteVector data = ByteVector(offsets.size(), data_sz, buffer);
            result.GetColumns()[i] = std::make_shared<StringColumn>(
                std::move(data), std::move(offsets));
        }
    }

    ++cur_index;

    return result;
}

bool ColumnarReader::IsEnd(size_t cur_butch) const{
    return cur_butch >= data_.chunk_metas.size();
}

ColumnarReader::~ColumnarReader() { is_.close(); }

const Scheme &ColumnarReader::GetScheme() const { return data_.scheme; }

std::string ColumnarReader::GetNameByIndex(size_t index) const {
    return data_.GetColumnNameByIndex(index);
}

ColumnTypes ColumnarReader::GetTypeByIndex(size_t index) const {
    return data_.GetColumnTypeByIndex(index);
}
