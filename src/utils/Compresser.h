#pragma once

#include "data_structures/ByteVector.h"
#include "data_structures/Column.h"
#include <cstdint>
#include <memory>
#include <vector>
enum class CompressType { None, Dict, Delta, BitPack };

namespace Compression {

struct DictStringMetaData {
    ByteVector dict;
    std::vector<size_t> offsets;
    uint8_t bit_width;
    size_t real_size;
};

std::pair<std::vector<uint8_t>, DictStringMetaData>
CompressDictFromString(const std::shared_ptr<const StringColumn> &column);

std::shared_ptr<Column>
DecompressStringFromDict(const std::vector<uint8_t> &data,
                         const DictStringMetaData &meta);

struct BitPackingMetaData {
    uint8_t bit_width;
    size_t real_size;
};

std::pair<std::vector<uint8_t>, BitPackingMetaData>
CompressIntTypesBitPacking(const std::shared_ptr<Column> &column);

std::shared_ptr<Column> DecompressIntTypesBitPacking(const std::vector<uint8_t> &data, const BitPackingMetaData &meta, ColumnTypes type);
} // namespace Compression
