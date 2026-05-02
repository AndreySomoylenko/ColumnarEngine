#include "Column.h"
#include <cstddef>
#include <stdexcept>
#include <string>

size_t Int64Column::Size() const { return data_.Size(); }
size_t StringColumn::Size() const { return data_.Size(); }

void Int64Column::Push(const std::string &s) {
    int64_t result = std::stoll(s);
    data_.Push(&result, ElemSize);
}

void StringColumn::Push(const std::string &s) {
    offsets_.emplace_back(data_.SizeInBytes());
    data_.Push(s.data(), s.size());
}

std::string Int64Column::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }

    return std::to_string(*reinterpret_cast<const int64_t *>(data_.Data() + index * ElemSize));
};

std::string StringColumn::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }

    const size_t start = offsets_[index];
    const size_t end = index + 1 < offsets_.size() ? offsets_[index + 1] : data_.SizeInBytes();
    return std::string(data_.Data() + start, end - start);
}

void Int64Column::Clear() { data_.Clear(); }
void StringColumn::Clear() {
    data_.Clear();
    offsets_.clear();
}

std::pair<const char *, size_t> Int64Column::ToWrite() { return {data_.Data(), data_.SizeInBytes()}; }

std::pair<const char *, size_t> StringColumn::ToWrite() { return {data_.Data(), data_.SizeInBytes()}; }

StringColumn::StringColumn(const size_t sz) { data_.Reserve(sz * 100); }
Int64Column::Int64Column(const size_t sz) { data_.Reserve(sz, 8); }

ColumnTypes StringColumn::GetColumnType() const { return ColumnTypes::String; }
ColumnTypes Int64Column::GetColumnType() const { return ColumnTypes::Int64; }

Int64Column::Int64Column(ByteVector &&data) { data_ = std::move(data); }
StringColumn::StringColumn(ByteVector &&data, std::vector<size_t> &&offsets) {
    data_ = std::move(data);
    offsets_ = std::move(offsets);
}

const std::vector<size_t> &StringColumn::GetOffsets() const { return offsets_; }

const std::vector<size_t> &Int64Column::GetOffsets() const {
    throw std::logic_error("Int64 column doesn't have offsets");
}

std::variant<const char *, std::pair<const char *, size_t>> Int64Column::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return data_.Data() + index * ElemSize;
}

std::variant<const char *, std::pair<const char *, size_t>> StringColumn::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    const size_t start = offsets_[index];
    const size_t end = index + 1 < offsets_.size() ? offsets_[index + 1] : data_.SizeInBytes();
    return std::make_pair(data_.Data() + start, end - start);
}

std::variant<const char *, std::pair<const char *, size_t>> TimeColumn::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return data_.Data() + index * sizeof(std::chrono::system_clock::time_point);
}