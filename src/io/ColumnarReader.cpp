#include "io/ColumnarReader.h"
#include "utils/Compresser.h"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <ios>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

#include "data_structures/ByteVector.h"
#include "data_structures/Column.h"

namespace {

template <typename T> T ReadPod(const char *&cursor) {
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

std::vector<uint8_t> ReadPackedBytes(const char *&cursor) {
    const size_t packed_size = ReadPod<size_t>(cursor);
    std::vector<uint8_t> result(
        reinterpret_cast<const uint8_t *>(cursor),
        reinterpret_cast<const uint8_t *>(cursor + packed_size));
    cursor += packed_size;
    return result;
}

} // namespace

ColumnarReader::ColumnarReader(const std::string &columnar) {
    fd_ = ::open(columnar.c_str(), O_RDONLY);
    if (fd_ == -1) {
        throw std::invalid_argument("You give me really bad file");
    }

    struct stat file_stat {};
    if (::fstat(fd_, &file_stat) == -1 || file_stat.st_size <= 0) {
        Close();
        throw std::invalid_argument("You give me really bad file");
    }

    mapped_size_ = static_cast<size_t>(file_stat.st_size);
    void *mapping =
        ::mmap(nullptr, mapped_size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (mapping == MAP_FAILED) {
        mapped_size_ = 0;
        Close();
        throw std::invalid_argument("You give me really bad file");
    }
    mapped_data_ = static_cast<const char *>(mapping);
    ::madvise(const_cast<char *>(mapped_data_), mapped_size_, MADV_SEQUENTIAL);

    const char *cursor = mapped_data_;
    const char *file_end = mapped_data_ + mapped_size_;

    const std::streamoff meta = ReadPod<std::streamoff>(cursor);
    data_.meta_section_start = std::streampos(meta);
    cursor = mapped_data_ + meta;

    const size_t chunk_count = ReadPod<size_t>(cursor);
    data_.columns_starts.resize(chunk_count);

    const size_t columns_count = ReadPod<size_t>(cursor);
    data_.batch_numbers = chunk_count;
    data_.column_numbers = columns_count;
    for (size_t i = 0; i < chunk_count; ++i) {
        data_.columns_starts[i].resize(columns_count);
        for (size_t j = 0; j < columns_count; ++j) {
            const std::streamoff column_start = ReadPod<std::streamoff>(cursor);
            data_.columns_starts[i][j] = column_start;
        }
    }

    while (cursor < file_end) {
        const size_t name_sz = ReadPod<size_t>(cursor);
        std::string name(cursor, name_sz);
        cursor += name_sz;

        const size_t type_sz = ReadPod<size_t>(cursor);
        std::string type(cursor, type_sz);
        cursor += type_sz;

        data_.scheme.Add(Row{name, type});
    }

    data_.column_numbers = data_.scheme.GetSchemeNames().size();
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

    const size_t columns_count = data_.column_numbers;

    Batch result(scheme, false);

    if (columns_to_read.empty()) {
        ++cur_index;
        return result;
    }

    const auto &types = data_.scheme.GetSchemeTypes();

    const auto ColumnSize = [&](size_t column_index) {
        if (column_index + 1 == columns_count) {
            if (cur_index != data_.batch_numbers - 1) {
                return data_.columns_starts[cur_index + 1][0] -
                       data_.columns_starts[cur_index][column_index];
            }
            return data_.meta_section_start -
                   data_.columns_starts[cur_index][column_index];
        }
        return data_.columns_starts[cur_index][column_index + 1] -
               data_.columns_starts[cur_index][column_index];
    };

    size_t min_column = columns_to_read.front();
    size_t max_column = columns_to_read.front();
    for (size_t column_index : columns_to_read) {
        if (column_index < min_column) {
            min_column = column_index;
        }
        if (column_index > max_column) {
            max_column = column_index;
        }
    }

    const std::streampos read_start =
        data_.columns_starts[cur_index][min_column];
    const std::streampos read_end =
        data_.columns_starts[cur_index][max_column] + ColumnSize(max_column);
    ::madvise(const_cast<char *>(mapped_data_) +
                  static_cast<std::streamoff>(read_start),
              static_cast<size_t>(read_end - read_start), MADV_SEQUENTIAL);

    for (size_t i = 0; i < columns_to_read.size(); ++i) {
        const size_t column_index = columns_to_read[i];
        const std::streampos column_start =
            data_.columns_starts[cur_index][column_index];
        const std::streamoff column_size = ColumnSize(column_index);

        const char *column_data =
            mapped_data_ + static_cast<std::streamoff>(column_start);
        const char *cursor = column_data;

        if (types[column_index] == ColumnTypes::Int16 ||
            types[column_index] == ColumnTypes::Int32 ||
            types[column_index] == ColumnTypes::Int64) {

            Compression::BitPackingMetaData meta;
            meta.bit_width = ReadPod<uint8_t>(cursor);
            meta.real_size = ReadPod<size_t>(cursor);
            std::vector<uint8_t> packed = ReadPackedBytes(cursor);

            result.GetColumns()[i] = Compression::DecompressIntTypesBitPacking(
                packed, meta, types[column_index]);
        } else if (types[column_index] == ColumnTypes::Int128) {
            const size_t col_size = static_cast<size_t>(column_size);
            ByteVector data = CopyByteVector(
                column_data, col_size / sizeof(__int128), col_size);
            result.GetColumns()[i] =
                std::make_shared<Int128Column>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Double) {
            const size_t col_size = static_cast<size_t>(column_size);
            ByteVector data =
                CopyByteVector(column_data, col_size / sizeof(double), col_size);
            result.GetColumns()[i] =
                std::make_shared<DoubleColumn>(std::move(data));

        } else if (types[column_index] == ColumnTypes::Timestamp ||
                   types[column_index] == ColumnTypes::Date) {
            const size_t col_size = static_cast<size_t>(column_size);
            ByteVector data = CopyByteVector(
                column_data,
                col_size / sizeof(std::chrono::system_clock::time_point),
                col_size);
            result.GetColumns()[i] = std::make_shared<TimeColumn>(
                std::move(data), types[column_index] == ColumnTypes::Date);

        } else if (types[column_index] == ColumnTypes::String ||
                   types[column_index] == ColumnTypes::Unknown) {
            Compression::DictStringMetaData meta;

            meta.bit_width = ReadPod<uint8_t>(cursor);
            meta.real_size = ReadPod<size_t>(cursor);

            const size_t dict_size = ReadPod<size_t>(cursor);
            const char *dict_data = cursor;
            cursor += dict_size;

            const size_t offsets_size = ReadPod<size_t>(cursor);
            meta.offsets.resize(offsets_size / sizeof(size_t));
            std::memcpy(meta.offsets.data(), cursor, offsets_size);
            cursor += offsets_size;

            meta.dict =
                CopyByteVector(dict_data, meta.offsets.size(), dict_size);
            std::vector<uint8_t> packed = ReadPackedBytes(cursor);

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

void ColumnarReader::Close() {
    if (mapped_data_ != nullptr) {
        ::munmap(const_cast<char *>(mapped_data_), mapped_size_);
        mapped_data_ = nullptr;
        mapped_size_ = 0;
    }
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

ColumnarReader::ColumnarReader(ColumnarReader &&other) noexcept
    : data_(std::move(other.data_)), fd_(other.fd_),
      mapped_data_(other.mapped_data_), mapped_size_(other.mapped_size_) {
    other.fd_ = -1;
    other.mapped_data_ = nullptr;
    other.mapped_size_ = 0;
}

ColumnarReader &ColumnarReader::operator=(ColumnarReader &&other) noexcept {
    if (this != &other) {
        Close();

        data_ = std::move(other.data_);
        fd_ = other.fd_;
        mapped_data_ = other.mapped_data_;
        mapped_size_ = other.mapped_size_;

        other.fd_ = -1;
        other.mapped_data_ = nullptr;
        other.mapped_size_ = 0;
    }
    return *this;
}

ColumnarReader::~ColumnarReader() { Close(); }

const Scheme &ColumnarReader::GetScheme() const { return data_.scheme; }
