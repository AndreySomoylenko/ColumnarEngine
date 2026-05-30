#include "utils/Compresser.h"
#include "data_structures/ByteVector.h"
#include "data_structures/Column.h"
#include "data_structures/Containers.h"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>

std::pair<std::vector<uint8_t>, Compression::DictStringMetaData>
Compression::CompressDictFromString(
    const std::shared_ptr<const StringColumn> &column) {
    if (column->Size() == 0) {
        throw std::logic_error("Can't compress Emty Columns");
    }

    DictStringMetaData meta;
    meta.real_size = column->Size();

    int c = 0;
    HashFlatMap<std::string_view, size_t> indices;
    indices.reserve(column->Size());
    meta.offsets.reserve(column->Size());
    for (int i = 0; i < column->Size(); ++i) {
        const auto &[value, sz] = column->Get(i);
        if (!indices.count(std::string_view(value, sz))) {
            indices[{value, sz}] = c++;
            meta.offsets.emplace_back(meta.dict.SizeInBytes());
            meta.dict.Push(value, sz);
        }
    }

    uint64_t max_code = c == 0 ? 0 : static_cast<uint64_t>(c - 1);
    meta.bit_width = max_code == 0 ? 1 : 64 - __builtin_clzll(max_code);

    size_t bit_count = column->Size() * meta.bit_width;
    std::vector<uint8_t> result((bit_count + 7) / 8, 0);

    size_t cur_ind = 0;

    for (size_t i = 0; i < column->Size(); ++i) {
        const auto [value, size] = column->Get(i);

        uint64_t to_write = indices[{value, size}];

        for (uint8_t bit = 0; bit < meta.bit_width; ++bit) {
            if ((to_write >> bit) & 1ULL) {
                result[cur_ind / 8] |= uint8_t(1U << (cur_ind % 8));
            }
            ++cur_ind;
        }
    }

    return std::make_pair(std::move(result), std::move(meta));
}

std::shared_ptr<Column>
Compression::DecompressStringFromDict(const std::vector<uint8_t> &data,
                                      const DictStringMetaData &meta) {
    ByteVector result;
    std::vector<size_t> offsets;
    size_t cur_ind = 0;
    std::vector<std::string_view> strs;

    const char *dict_data = meta.dict.Data();

    for (int i = 0; i < meta.offsets.size(); ++i) {
        strs.emplace_back(dict_data + meta.offsets[i],
                          (i == meta.offsets.size() - 1
                               ? meta.dict.SizeInBytes()
                               : meta.offsets[i + 1]) -
                              meta.offsets[i]);
    }

    for (int i = 0; i < meta.real_size; ++i) {
        size_t tmp_val = 0;
        for (uint8_t bit = 0; bit < meta.bit_width; ++bit) {
            uint8_t is_set = (data[cur_ind / 8] >> (cur_ind % 8)) & 1U;
            tmp_val |= size_t(is_set) << bit;
            ++cur_ind;
        }

        offsets.emplace_back(result.SizeInBytes());
        result.Push(strs[tmp_val].data(), strs[tmp_val].size());
    }

    return std::make_shared<StringColumn>(std::move(result),
                                          std::move(offsets));
}

std::pair<std::vector<uint8_t>, Compression::BitPackingMetaData>
Compression::CompressIntTypesBitPacking(const std::shared_ptr<Column> &column) {
    if (column->Size() == 0) {
        throw std::logic_error("You can't compress empty column");
    }

    BitPackingMetaData meta{};
    meta.real_size = column->Size();
    uint64_t max_encoded = 0;

    for (size_t i = 0; i < column->Size(); ++i) {

        const auto &[value, sz] = column->Get(i);

        int64_t current = 0;

        switch (column->GetColumnType()) {

        case ColumnTypes::Int16: {
            int16_t v;
            std::memcpy(&v, value, sizeof(v));
            current = v;
            break;
        }

        case ColumnTypes::Int32: {
            int32_t v;
            std::memcpy(&v, value, sizeof(v));
            current = v;
            break;
        }

        case ColumnTypes::Int64: {
            int64_t v;
            std::memcpy(&v, value, sizeof(v));
            current = v;
            break;
        }

        default:
            throw std::logic_error("Non-integer column type");
        }

        uint64_t encoded = (static_cast<uint64_t>(current) << 1) ^
                           static_cast<uint64_t>(current >> 63);

        max_encoded = std::max(max_encoded, encoded);
    }

    meta.bit_width = max_encoded == 0 ? 1 : 64 - __builtin_clzll(max_encoded);
    size_t total_bits = column->Size() * meta.bit_width;

    std::vector<uint8_t> result((total_bits + 7) / 8, 0);
    size_t cur_bit = 0;

    for (size_t i = 0; i < column->Size(); ++i) {

        const auto &[value, sz] = column->Get(i);

        int64_t current = 0;

        switch (column->GetColumnType()) {

        case ColumnTypes::Int16: {
            int16_t v;
            std::memcpy(&v, value, sizeof(v));
            current = v;
            break;
        }

        case ColumnTypes::Int32: {
            int32_t v;
            std::memcpy(&v, value, sizeof(v));
            current = v;
            break;
        }

        case ColumnTypes::Int64: {
            int64_t v;
            std::memcpy(&v, value, sizeof(v));
            current = v;
            break;
        }

        default:
            throw std::logic_error("Non-integer column type");
        }

        uint64_t encoded = (static_cast<uint64_t>(current) << 1) ^
                           static_cast<uint64_t>(current >> 63);

        for (uint8_t bit = 0; bit < meta.bit_width; ++bit) {

            if ((encoded >> bit) & 1ULL) {

                result[cur_bit / 8] |= uint8_t(1U << (cur_bit % 8));
            }

            ++cur_bit;
        }
    }

    return {std::move(result), std::move(meta)};
}

int64_t DecodeZigZag(uint64_t x) {
    return (x >> 1) ^ -static_cast<int64_t>(x & 1);
}

std::shared_ptr<Column>
Compression::DecompressIntTypesBitPacking(const std::vector<uint8_t> &data,
                                          const BitPackingMetaData &meta,
                                          ColumnTypes type) {
    ByteVector result;
    size_t cur_ind = 0;

    for (int i = 0; i < meta.real_size; ++i) {
        uint64_t tmp_val = 0;
        for (uint8_t bit = 0; bit < meta.bit_width; ++bit) {
            uint8_t is_set = (data[cur_ind / 8] >> (cur_ind % 8)) & 1U;
            tmp_val |= uint64_t(is_set) << bit;
            ++cur_ind;
        }

        int64_t orig = DecodeZigZag(tmp_val);

        switch (type) {
        case ColumnTypes::Int16: {
            int16_t to_write = orig;
            result.Push(&to_write, 2);
            break;
        }
        case ColumnTypes::Int32: {
            int32_t to_write = orig;
            result.Push(&to_write, 4);
            break;
        }
        case ColumnTypes::Int64: {
            int64_t to_write = orig;
            result.Push(&to_write, 8);
            break;
        }
        default:
            throw std::logic_error("Wrong colun type");
        }
    }

    switch (type) {
    case ColumnTypes::Int16:
        return std::make_shared<Int16Column>(std::move(result));
    case ColumnTypes::Int32:
        return std::make_shared<Int32Column>(std::move(result));
    case ColumnTypes::Int64:
        return std::make_shared<Int64Column>(std::move(result));
    default:
        throw std::logic_error("Wrong column type");
    }
}
