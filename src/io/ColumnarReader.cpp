#include "ColumnarReader.h"
#include "data_structures/Column.h"
#include "utils/StringConverter.h"
#include <cassert>
#include <cstring>
#include <iosfwd>
#include <string>

// к этом классу хочется прикрутить еще один слой абстракции, который будет думать о сжатии-расжатии и
// представление в человекочитаемый вид. Но это уже после кт1

std::string ColumnarReader::Unpack(const std::string &from_file, const size_t index) {
    auto type = data_.scheme.GetSchemeTypes()[index];
    if (type == ColumnTypes::Int64) {
        uint64_t tmp;
        memcpy(&tmp, from_file.data(), sizeof(tmp));

        return std::to_string(tmp);
    } else if (type == ColumnTypes::String) {
        return from_file;
    } else if (type == ColumnTypes::Unknown) {
        return from_file;
    }

    return from_file;
}

ColumnarReader::ColumnarReader(const std::string &columnar)
    : is_(columnar, std::ios::binary)
    , cur_butch_(0) {
    uint64_t meta;
    is_.read(reinterpret_cast<char *>(&meta), sizeof(meta));

    StringConverter converter;

    is_.seekg(meta, std::ios::beg);

    while (is_.peek() != ';') {
        uint64_t cur;
        is_.read(reinterpret_cast<char *>(&cur), sizeof(cur));

        data_.chunk_metas.emplace_back(cur);
        if (is_.peek() == ';') {
            break;
        }
        is_.get();
    }
    is_.get();

    while (is_.peek() != EOF) {
        ++data_.column_numbers;
        auto name = converter.ReadNextString(is_, {',', ';', EOF});
        is_.get();
        auto type = converter.ReadNextString(is_, {',', ';', EOF});

        data_.scheme.Add({name, type});

        if (is_.peek() == EOF) {
            break;
        }

        is_.get();
    }
}

Butch ColumnarReader::ReadNext() {
    StringConverter converter;

    is_.clear();

    is_.seekg(data_.chunk_metas[cur_butch_], std::ios::beg);
    uint64_t start;

    is_.read(reinterpret_cast<char *>(&start), sizeof(start));
    is_.seekg(start, std::ios::beg);

    size_t index = 0;

    Butch result(data_.scheme);

    for (size_t i = 0; i < data_.column_numbers; ++i) {
        while (is_.peek() != ';') {
            auto tstr = converter.ReadNextString(is_, {',', ';'});
            result.AddToColumn(Unpack(tstr, i), index);

            if (is_.peek() == ';') {
                break;
            }
            is_.get();
        }
        is_.get();

        ++index;
    }

    ++cur_butch_;

    return result;
}

void ColumnarReader::Reset() { cur_butch_ = 0; }

bool ColumnarReader::IsEnd() { return cur_butch_ >= data_.chunk_metas.size(); }

ColumnarReader::~ColumnarReader() { is_.close(); }

Scheme ColumnarReader::GetScheme() { return data_.scheme; }