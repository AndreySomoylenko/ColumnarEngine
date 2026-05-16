#include "io/ColumnarReader.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <memory>
#include <stdexcept>
#include <string>

#include "data_structures/ByteVector.h"
#include "data_structures/Column.h"

namespace {

using OwnedBuffer = std::unique_ptr<void, decltype(&std::free)>;

OwnedBuffer ReadBuffer(std::istream &is, size_t byte_count) {
    OwnedBuffer buffer(nullptr, &std::free);
    if (byte_count > 0) {
        buffer.reset(std::malloc(byte_count));
        if (buffer == nullptr) {
            throw std::bad_alloc();
        }
    }

    if (!is.read(static_cast<char *>(buffer.get()), byte_count)) {
        throw std::invalid_argument("You give me really bad file");
    }

    return buffer;
}

ByteVector ReadByteVector(std::istream &is, size_t element_count,
                          size_t byte_count) {
    OwnedBuffer buffer = ReadBuffer(is, byte_count);
    return ByteVector(element_count, byte_count, buffer.release());
}

} // namespace

ColumnarReader::ColumnarReader(const std::string &columnar) {
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

        data_.scheme.Add(Row{name, type});
    }

    data_.column_numbers = data_.scheme.GetSchemeNames().size();

    is_.clear();
}

Batch ColumnarReader::ReadNext(const Scheme &scheme, size_t &cur_index) {
    if (IsEnd(cur_index)) {
        throw std::out_of_range("No more data to read");
    }

    std::vector<size_t> columns_to_read;
    size_t columns_count = data_.scheme.GetSchemeNames().size();
    for (auto &name : scheme.GetSchemeNames()) {
        columns_to_read.push_back(data_.GetColumnIndexByName(name));
    }

    is_.seekg(data_.chunk_metas[cur_index], std::ios::beg);

    Batch result(scheme, false);

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
            column_size =
                data_.chunk_metas[cur_index] - columns_starts[column_index];
        } else {
            column_size =
                columns_starts[column_index + 1] - columns_starts[column_index];
        }

        if (column_size < 0) {
            throw std::invalid_argument("You give me really bad file");
        }

        if (types[column_index] == ColumnTypes::Int16) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data =
                ReadByteVector(is_, col_size / sizeof(int16_t), col_size);
            result.GetColumns()[i] =
                std::make_shared<Int16Column>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Int32) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data =
                ReadByteVector(is_, col_size / sizeof(int32_t), col_size);
            result.GetColumns()[i] =
                std::make_shared<Int32Column>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Int64) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data =
                ReadByteVector(is_, col_size / sizeof(int64_t), col_size);
            result.GetColumns()[i] =
                std::make_shared<Int64Column>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Int128) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data =
                ReadByteVector(is_, col_size / sizeof(__int128), col_size);
            result.GetColumns()[i] =
                std::make_shared<Int128Column>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Double) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data =
                ReadByteVector(is_, col_size / sizeof(double), col_size);
            result.GetColumns()[i] =
                std::make_shared<DoubleColumn>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Timestamp ||
                   types[column_index] == ColumnTypes::Date) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data = ReadByteVector(
                is_, col_size / sizeof(std::chrono::system_clock::time_point),
                col_size);
            result.GetColumns()[i] = std::make_shared<TimeColumn>(
                std::move(data), types[column_index] == ColumnTypes::Date);

        } else if (types[column_index] == ColumnTypes::String ||
                   types[column_index] == ColumnTypes::Unknown) {
            size_t data_sz;
            if (!is_.read(reinterpret_cast<char *>(&data_sz),
                          sizeof(data_sz))) {
                throw std::invalid_argument("You give me really bad file");
            }
            OwnedBuffer buffer = ReadBuffer(is_, data_sz);

            size_t offsets_sz;
            if (!is_.read(reinterpret_cast<char *>(&offsets_sz),
                          sizeof(offsets_sz))) {
                throw std::invalid_argument("You give me really bad file");
            }
            std::vector<size_t> offsets(offsets_sz / sizeof(size_t));
            if (!is_.read(reinterpret_cast<char *>(offsets.data()),
                          offsets_sz)) {
                throw std::invalid_argument("You give me really bad file");
            }

            ByteVector data(offsets.size(), data_sz, buffer.release());
            result.GetColumns()[i] = std::make_shared<StringColumn>(
                std::move(data), std::move(offsets));
        }
    }

    ++cur_index;

    return result;
}

bool ColumnarReader::IsEnd(size_t cur_batch) const {
    return cur_batch >= data_.chunk_metas.size();
}

ColumnarReader::~ColumnarReader() { is_.close(); }

const Scheme &ColumnarReader::GetScheme() const { return data_.scheme; }
