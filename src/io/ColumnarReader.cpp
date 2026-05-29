#include "io/ColumnarReader.h"
#include "utils/Compresser.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ios>
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
    is_.read(reinterpret_cast<char *>(&chunk_count), sizeof(chunk_count));
    data_.columns_starts.resize(chunk_count);

    size_t columns_count;
    is_.read(reinterpret_cast<char *>(&columns_count), sizeof(columns_count));
    data_.batch_numbers = chunk_count;
    data_.column_numbers = columns_count;
    for (size_t i = 0; i < chunk_count; ++i) {
        data_.columns_starts[i].resize(columns_count);
        for (size_t j = 0; j < columns_count; ++j) {
            std::streamoff column_start;
            is_.read(reinterpret_cast<char *>(&column_start),
                     sizeof(column_start));
            data_.columns_starts[i][j] = column_start;
        }
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
    size_t columns_count = data_.column_numbers;
    for (auto &name : scheme.GetSchemeNames()) {
        columns_to_read.push_back(data_.GetColumnIndexByName(name));
    }

    Batch result(scheme, false);

    auto types = data_.scheme.GetSchemeTypes();

    for (size_t i = 0; i < columns_to_read.size(); ++i) {
        const size_t column_index = columns_to_read[i];
        is_.seekg(data_.columns_starts[cur_index][column_index], std::ios::beg);

        std::streamoff column_size;

        if (column_index + 1 == columns_count) {
            if (cur_index != data_.batch_numbers - 1) {
                column_size = data_.columns_starts[cur_index + 1][0] -
                              data_.columns_starts[cur_index][column_index];
            } else {
                column_size = data_.meta_section_start -
                              data_.columns_starts[cur_index][column_index];
            }
        } else {
            column_size = data_.columns_starts[cur_index][column_index + 1] -
                          data_.columns_starts[cur_index][column_index];
        }

        if (column_size < 0) {
            throw std::invalid_argument("You give me really bad file");
        }

        if (types[column_index] == ColumnTypes::Int16 ||
            types[column_index] == ColumnTypes::Int32 ||
            types[column_index] == ColumnTypes::Int64) {

            Compression::BitPackingMetaData meta;

            is_.read(reinterpret_cast<char *>(&meta.bit_width),
                     sizeof(meta.bit_width));

            is_.read(reinterpret_cast<char *>(&meta.real_size),
                     sizeof(meta.real_size));

            size_t packed_size;

            is_.read(reinterpret_cast<char *>(&packed_size),
                     sizeof(packed_size));

            std::vector<uint8_t> packed(packed_size);

            is_.read(reinterpret_cast<char *>(packed.data()), packed_size);

            result.GetColumns()[i] = Compression::DecompressIntTypesBitPacking(
                packed, meta, types[column_index]);
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
            Compression::DictStringMetaData meta;

            if (!is_.read(reinterpret_cast<char *>(&meta.bit_width),
                          sizeof(meta.bit_width))) {
                throw std::invalid_argument("You give me really bad file");
            }

            if (!is_.read(reinterpret_cast<char *>(&meta.real_size),
                          sizeof(meta.real_size))) {
                throw std::invalid_argument("You give me really bad file");
            }

            size_t dict_size;
            if (!is_.read(reinterpret_cast<char *>(&dict_size),
                          sizeof(dict_size))) {
                throw std::invalid_argument("You give me really bad file");
            }

            OwnedBuffer dict_buffer = ReadBuffer(is_, dict_size);

            size_t offsets_size;
            if (!is_.read(reinterpret_cast<char *>(&offsets_size),
                          sizeof(offsets_size))) {
                throw std::invalid_argument("You give me really bad file");
            }

            if (offsets_size % sizeof(size_t) != 0) {
                throw std::invalid_argument("You give me really bad file");
            }

            meta.offsets.resize(offsets_size / sizeof(size_t));
            if (!is_.read(reinterpret_cast<char *>(meta.offsets.data()),
                          offsets_size)) {
                throw std::invalid_argument("You give me really bad file");
            }

            meta.dict = ByteVector(meta.offsets.size(), dict_size,
                                   dict_buffer.release());

            size_t value_size;
            if (!is_.read(reinterpret_cast<char *>(&value_size),
                          sizeof(value_size))) {
                throw std::invalid_argument("You give me really bad file");
            }

            std::vector<uint8_t> packed(value_size);
            if (!is_.read(reinterpret_cast<char *>(packed.data()),
                          value_size)) {
                throw std::invalid_argument("You give me really bad file");
            }

            result.GetColumns()[i] =
                Compression::DecompressStringFromDict(packed, meta);
        }
    }

    ++cur_index;

    return result;
}

bool ColumnarReader::IsEnd(size_t cur_batch) const {
    return cur_batch >= data_.batch_numbers;
}

ColumnarReader::~ColumnarReader() { is_.close(); }

const Scheme &ColumnarReader::GetScheme() const { return data_.scheme; }
