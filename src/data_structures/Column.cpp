#include "Column.h"
#include "utils/StringConverter.h"
#include <cstddef>
#include <stdexcept>
#include <string>

size_t Int64Column::Size() { return data_.size(); }
size_t StringColumn::Size() { return data_.size(); }

char *Int64Column::Data() { return reinterpret_cast<char *>(data_.data()); }
char *StringColumn::Data() { return reinterpret_cast<char *>(data_.data()); }

size_t Int64Column::Push(const std::string &s) {
    data_.emplace_back(std::stoll(s));
    return 8;
}

size_t StringColumn::Push(const std::string &s) {
    data_.emplace_back(s);
    return s.size();
}

std::string Int64Column::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }

    return std::to_string(data_[index]);
};

std::string StringColumn::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }

    return data_[index];
}

void Int64Column::Clear() { data_.clear(); }
void StringColumn::Clear() { data_.clear(); }

std::pair<char *, size_t> Int64Column::ToWrite(const size_t index) {
    return {reinterpret_cast<char *>(&data_[index]), 8};
}

std::pair<char *, size_t> StringColumn::ToWrite(const size_t index) {
    StringConverter converter;
    cache_ = converter.TransformStringToFilestring(data_[index]);
    return {cache_.data(), cache_.size()};
}