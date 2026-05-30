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

void EnsureAvailable(const char *cursor, const char *end, size_t byte_count) {
    if (byte_count > static_cast<size_t>(end - cursor)) {
        throw std::invalid_argument("You give me really bad file");
    }
}

template <typename T> T ReadPod(const char *&cursor, const char *end) {
    EnsureAvailable(cursor, end, sizeof(T));

    T value;
    std::memcpy(&value, cursor, sizeof(T));
    cursor += sizeof(T);
    return value;
}

ByteVector CopyByteVector(const char *data, size_t element_count,
                          size_t byte_count) {
    void *buffer = nullptr;
    if (byte_count > 0) {
        buffer = std::malloc(byte_count);
        if (buffer == nullptr) {
            throw std::bad_alloc();
        }
        std::memcpy(buffer, data, byte_count);
    }

    return ByteVector(element_count, byte_count, buffer);
}

std::vector<uint8_t> ReadPackedBytes(const char *&cursor, const char *end) {
    const size_t packed_size = ReadPod<size_t>(cursor, end);
    EnsureAvailable(cursor, end, packed_size);

    std::vector<uint8_t> result(
        reinterpret_cast<const uint8_t *>(cursor),
        reinterpret_cast<const uint8_t *>(cursor + packed_size));
    cursor += packed_size;
    return result;
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

std::vector<size_t> ColumnarReader::GetColumnIndices(const Scheme &scheme) const {
    std::vector<size_t> columns_to_read;
    columns_to_read.reserve(scheme.GetSchemeNames().size());
    for (const auto &name : scheme.GetSchemeNames()) {
        columns_to_read.push_back(data_.GetColumnIndexByName(name));
    }
    return columns_to_read;
}

Batch ColumnarReader::ReadNext(const Scheme &scheme,
                               const std::vector<size_t> &columns_to_read,
                               size_t &cur_index) {
    if (IsEnd(cur_index)) {
        throw std::out_of_range("No more data to read");
    }

    size_t columns_count = data_.column_numbers;

    Batch result(scheme, false);

    const auto &types = data_.scheme.GetSchemeTypes();
    const std::streampos batch_start = data_.columns_starts[cur_index][0];
    const std::streampos batch_end =
        cur_index + 1 == data_.batch_numbers
            ? data_.meta_section_start
            : data_.columns_starts[cur_index + 1][0];

    const std::streamoff batch_size = batch_end - batch_start;
    if (batch_size < 0) {
        throw std::invalid_argument("You give me really bad file");
    }

    is_.seekg(batch_start, std::ios::beg);
    OwnedBuffer batch_buffer =
        ReadBuffer(is_, static_cast<size_t>(batch_size));
    const char *batch_data = static_cast<const char *>(batch_buffer.get());

    for (size_t i = 0; i < columns_to_read.size(); ++i) {
        const size_t column_index = columns_to_read[i];
        const std::streampos column_start =
            data_.columns_starts[cur_index][column_index];

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
        if (column_start < batch_start ||
            column_start + column_size > batch_end) {
            throw std::invalid_argument("You give me really bad file");
        }

        const char *column_data =
            batch_data + static_cast<std::streamoff>(column_start - batch_start);
        const char *cursor = column_data;
        const char *column_end =
            column_data + static_cast<size_t>(column_size);

        if (types[column_index] == ColumnTypes::Int16 ||
            types[column_index] == ColumnTypes::Int32 ||
            types[column_index] == ColumnTypes::Int64) {

            Compression::BitPackingMetaData meta;
            meta.bit_width = ReadPod<uint8_t>(cursor, column_end);
            meta.real_size = ReadPod<size_t>(cursor, column_end);
            std::vector<uint8_t> packed = ReadPackedBytes(cursor, column_end);

            result.GetColumns()[i] = Compression::DecompressIntTypesBitPacking(
                packed, meta, types[column_index]);
        } else if (types[column_index] == ColumnTypes::Int128) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data = CopyByteVector(
                column_data, col_size / sizeof(__int128), col_size);
            result.GetColumns()[i] =
                std::make_shared<Int128Column>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Double) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data =
                CopyByteVector(column_data, col_size / sizeof(double), col_size);
            result.GetColumns()[i] =
                std::make_shared<DoubleColumn>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Timestamp ||
                   types[column_index] == ColumnTypes::Date) {
            size_t col_size = static_cast<size_t>(column_size);
            ByteVector data = CopyByteVector(
                column_data,
                col_size / sizeof(std::chrono::system_clock::time_point),
                col_size);
            result.GetColumns()[i] = std::make_shared<TimeColumn>(
                std::move(data), types[column_index] == ColumnTypes::Date);

        } else if (types[column_index] == ColumnTypes::String ||
                   types[column_index] == ColumnTypes::Unknown) {
            Compression::DictStringMetaData meta;

            meta.bit_width = ReadPod<uint8_t>(cursor, column_end);
            meta.real_size = ReadPod<size_t>(cursor, column_end);

            const size_t dict_size = ReadPod<size_t>(cursor, column_end);
            EnsureAvailable(cursor, column_end, dict_size);
            const char *dict_data = cursor;
            cursor += dict_size;

            const size_t offsets_size = ReadPod<size_t>(cursor, column_end);

            if (offsets_size % sizeof(size_t) != 0) {
                throw std::invalid_argument("You give me really bad file");
            }

            meta.offsets.resize(offsets_size / sizeof(size_t));
            EnsureAvailable(cursor, column_end, offsets_size);
            std::memcpy(meta.offsets.data(), cursor, offsets_size);
            cursor += offsets_size;

            meta.dict =
                CopyByteVector(dict_data, meta.offsets.size(), dict_size);
            std::vector<uint8_t> packed = ReadPackedBytes(cursor, column_end);

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
