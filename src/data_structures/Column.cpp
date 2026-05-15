#include "Column.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

int ParseTwoDigits(const std::string &value, size_t pos) {
    if (pos + 1 >= value.size() || !std::isdigit(value[pos]) ||
        !std::isdigit(value[pos + 1])) {
        throw std::invalid_argument("Incorrect timestamp");
    }
    return (value[pos] - '0') * 10 + (value[pos + 1] - '0');
}

int ParseFourDigits(const std::string &value, size_t pos) {
    if (pos + 3 >= value.size() || !std::isdigit(value[pos]) ||
        !std::isdigit(value[pos + 1]) || !std::isdigit(value[pos + 2]) ||
        !std::isdigit(value[pos + 3])) {
        throw std::invalid_argument("Incorrect timestamp");
    }
    return (value[pos] - '0') * 1000 + (value[pos + 1] - '0') * 100 +
           (value[pos + 2] - '0') * 10 + (value[pos + 3] - '0');
}

std::chrono::system_clock::time_point ParseTimestamp(const std::string &value,
                                                     bool is_date) {
    if (is_date) {
        if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
            throw std::invalid_argument("Incorrect date");
        }
        using namespace std::chrono;
        const year_month_day ymd{
            year{ParseFourDigits(value, 0)},
            month{static_cast<unsigned>(ParseTwoDigits(value, 5))},
            day{static_cast<unsigned>(ParseTwoDigits(value, 8))}};
        if (!ymd.ok()) {
            throw std::invalid_argument("Incorrect date");
        }
        return time_point_cast<system_clock::duration>(sys_days{ymd});
    }

    if (value.size() != 19 || value[4] != '-' || value[7] != '-' ||
        (value[10] != ' ' && value[10] != 'T') || value[13] != ':' ||
        value[16] != ':') {
        throw std::invalid_argument("Incorrect timestamp");
    }

    const int year_value = ParseFourDigits(value, 0);
    const int month_value = ParseTwoDigits(value, 5);
    const int day_value = ParseTwoDigits(value, 8);
    const int hour_value = ParseTwoDigits(value, 11);
    const int minute_value = ParseTwoDigits(value, 14);
    const int second_value = ParseTwoDigits(value, 17);

    if (hour_value > 23 || minute_value > 59 || second_value > 59) {
        throw std::invalid_argument("Incorrect timestamp");
    }

    using namespace std::chrono;
    const year_month_day ymd{year{year_value},
                             month{static_cast<unsigned>(month_value)},
                             day{static_cast<unsigned>(day_value)}};
    if (!ymd.ok()) {
        throw std::invalid_argument("Incorrect timestamp");
    }

    const sys_seconds timestamp = sys_days{ymd} + hours{hour_value} +
                                  minutes{minute_value} + seconds{second_value};
    return time_point_cast<system_clock::duration>(timestamp);
}

} // namespace

const std::vector<size_t> &Column::GetOffsets() const {
    throw std::logic_error("Column doesn't have offsets");
}

std::string GetNameByType(ColumnTypes type) {
    if (type == ColumnTypes::String) {
        return "string";
    } else if (type == ColumnTypes::Date) {
        return "date";
    } else if (type == ColumnTypes::Double) {
        return "double";
    } else if (type == ColumnTypes::Int128) {
        return "int128";
    } else if (type == ColumnTypes::Int64) {
        return "int64";
    } else if (type == ColumnTypes::Int32) {
        return "int32";
    } else if (type == ColumnTypes::Int16) {
        return "int16";
    } else if (type == ColumnTypes::Timestamp) {
        return "timestamp";
    } else if (type == ColumnTypes::Unknown) {
        return "unknown";
    } else {
        throw std::invalid_argument("Don't mutch any column type");
    }
}

namespace {

std::string FormatTimestamp(std::chrono::system_clock::time_point value,
                            bool is_date) {
    using namespace std::chrono;
    const sys_seconds timestamp = floor<seconds>(value);
    const sys_days days = floor<std::chrono::days>(timestamp);
    const year_month_day ymd{days};
    const hh_mm_ss time{timestamp - days};

    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year())
        << '-' << std::setw(2) << static_cast<unsigned>(ymd.month()) << '-'
        << std::setw(2) << static_cast<unsigned>(ymd.day());
    if (!is_date) {
        out << ' ' << std::setw(2) << time.hours().count() << ':'
            << std::setw(2) << time.minutes().count() << ':' << std::setw(2)
            << time.seconds().count();
    }
    return out.str();
}

template <typename T>
const T &GetFixedValue(const Column &column, size_t index) {
    return *reinterpret_cast<const T *>(column.Get(index).data);
}

int CompareStringValues(const Column &lhs, const Column &rhs, size_t index) {
    const auto lhs_value = lhs.Get(index);
    const auto rhs_value = rhs.Get(index);

    const std::string_view lhs_view(lhs_value.data, lhs_value.size);
    const std::string_view rhs_view(rhs_value.data, rhs_value.size);

    if (lhs_view < rhs_view) {
        return -1;
    }
    if (rhs_view < lhs_view) {
        return 1;
    }
    return 0;
}

template <typename T>
int CompareFixedValues(const Column &lhs, const Column &rhs, size_t index) {
    const T &lhs_value = GetFixedValue<T>(lhs, index);
    const T &rhs_value = GetFixedValue<T>(rhs, index);

    if (lhs_value < rhs_value) {
        return -1;
    }
    if (rhs_value < lhs_value) {
        return 1;
    }
    return 0;
}

int CompareValues(const Column &lhs, const Column &rhs, size_t index) {
    switch (lhs.GetColumnType()) {
    case ColumnTypes::Int16:
        return CompareFixedValues<int16_t>(lhs, rhs, index);
    case ColumnTypes::Int32:
        return CompareFixedValues<int32_t>(lhs, rhs, index);
    case ColumnTypes::Int64:
        return CompareFixedValues<int64_t>(lhs, rhs, index);
    case ColumnTypes::Int128:
        return CompareFixedValues<__int128>(lhs, rhs, index);
    case ColumnTypes::String:
    case ColumnTypes::Unknown:
        return CompareStringValues(lhs, rhs, index);
    case ColumnTypes::Timestamp:
    case ColumnTypes::Date:
        return CompareFixedValues<std::chrono::system_clock::time_point>(
            lhs, rhs, index);
    case ColumnTypes::Double:
        return CompareFixedValues<double>(lhs, rhs, index);
    }

    throw std::invalid_argument("Unknown column type");
}

} // namespace

bool Column::operator<(const Column &other) const {
    if (GetColumnType() != other.GetColumnType()) {
        return GetColumnType() < other.GetColumnType();
    }

    const size_t min_size = std::min(Size(), other.Size());
    for (size_t i = 0; i < min_size; ++i) {
        const int compare_result = CompareValues(*this, other, i);
        if (compare_result != 0) {
            return compare_result < 0;
        }
    }

    return Size() < other.Size();
}

bool Column::operator==(const Column &other) const {
    if (GetColumnType() != other.GetColumnType() || Size() != other.Size()) {
        return false;
    }

    for (size_t i = 0; i < Size(); ++i) {
        if (CompareValues(*this, other, i) != 0) {
            return false;
        }
    }

    return true;
}

bool Column::operator!=(const Column &other) const {
    return !(*this == other);
}

bool Column::operator>(const Column &other) const { return other < *this; }

bool Column::operator<=(const Column &other) const { return !(other < *this); }

bool Column::operator>=(const Column &other) const { return !(*this < other); }

size_t Int64Column::Size() const { return data_.Size(); }
size_t StringColumn::Size() const { return data_.Size(); }
size_t TimeColumn::Size() const { return data_.Size(); }

void Int64Column::Push(const std::string &s) {
    int64_t result = std::stoll(s);
    data_.Push(&result, ElemSize);
}

void StringColumn::Push(const std::string &s) {
    offsets_.emplace_back(data_.SizeInBytes());
    data_.Push(s.data(), s.size());
}

void TimeColumn::Push(const std::string &s) {
    const std::chrono::system_clock::time_point result =
        ParseTimestamp(s, is_date_);
    data_.Push(&result, ElemSize);
}

void Int64Column::Push(const char *data, size_t sz) {
    if (sz != ElemSize) {
        throw std::invalid_argument("Incorrect data size");
    }
    data_.Push(data, sz);
}

void StringColumn::Push(const char *data, size_t sz) {
    offsets_.emplace_back(data_.SizeInBytes());
    data_.Push(data, sz);
}

void TimeColumn::Push(const char *data, size_t sz) {
    if (sz != ElemSize) {
        throw std::invalid_argument("Incorrect data size");
    }
    data_.Push(data, sz);
}

std::string Int64Column::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }

    return std::to_string(
        *reinterpret_cast<const int64_t *>(data_.Data() + index * ElemSize));
};

std::string StringColumn::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }

    const size_t start = offsets_[index];
    const size_t end =
        index + 1 < offsets_.size() ? offsets_[index + 1] : data_.SizeInBytes();
    return std::string(data_.Data() + start, end - start);
}

std::string TimeColumn::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }

    return FormatTimestamp(
        *reinterpret_cast<const std::chrono::system_clock::time_point *>(
            data_.Data() + index * ElemSize),
        is_date_);
}

void Int64Column::Clear() { data_.Clear(); }
void StringColumn::Clear() {
    data_.Clear();
    offsets_.clear();
}
void TimeColumn::Clear() { data_.Clear(); }

std::pair<const char *, size_t> Int64Column::ToWrite() {
    return {data_.Data(), data_.SizeInBytes()};
}

std::pair<const char *, size_t> StringColumn::ToWrite() {
    return {data_.Data(), data_.SizeInBytes()};
}

std::pair<const char *, size_t> TimeColumn::ToWrite() {
    return {data_.Data(), data_.SizeInBytes()};
}

StringColumn::StringColumn(const size_t sz) { data_.Reserve(sz * 100); }
StringColumn::StringColumn(const char *data, size_t sz, size_t count) {
    data_.Reserve(std::max<size_t>(1, sz * count));
    offsets_.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        Push(data, sz);
    }
}
Int64Column::Int64Column(const size_t sz) { data_.Reserve(sz, 8); }
Int64Column::Int64Column(const char *data, size_t sz, size_t count) {
    data_.Reserve(count, ElemSize);
    for (size_t i = 0; i < count; ++i) {
        Push(data, sz);
    }
}
TimeColumn::TimeColumn(bool is_date) : is_date_(is_date) {}
TimeColumn::TimeColumn(const size_t sz, bool is_date) : is_date_(is_date) {
    data_.Reserve(sz, ElemSize);
}
TimeColumn::TimeColumn(const char *data, size_t sz, size_t count, bool is_date)
    : is_date_(is_date) {
    data_.Reserve(count, ElemSize);
    for (size_t i = 0; i < count; ++i) {
        Push(data, sz);
    }
}

ColumnTypes StringColumn::GetColumnType() const { return ColumnTypes::String; }
ColumnTypes Int64Column::GetColumnType() const { return ColumnTypes::Int64; }
ColumnTypes TimeColumn::GetColumnType() const {
    return is_date_ ? ColumnTypes::Date : ColumnTypes::Timestamp;
}

Int64Column::Int64Column(ByteVector &&data) { data_ = std::move(data); }
TimeColumn::TimeColumn(ByteVector &&data, bool is_date) : is_date_(is_date) {
    data_ = std::move(data);
}
StringColumn::StringColumn(ByteVector &&data, std::vector<size_t> &&offsets) {
    data_ = std::move(data);
    offsets_ = std::move(offsets);
}

const std::vector<size_t> &StringColumn::GetOffsets() const { return offsets_; }

ColumnValueView Int64Column::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return {data_.Data() + index * ElemSize, ElemSize};
}

ColumnValueView StringColumn::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    const size_t start = offsets_[index];
    const size_t end =
        index + 1 < offsets_.size() ? offsets_[index + 1] : data_.SizeInBytes();
    return {data_.Data() + start, end - start};
}

ColumnValueView TimeColumn::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return {data_.Data() + index * ElemSize, ElemSize};
}

Int128Column::Int128Column(const size_t sz) { data_.Reserve(sz, 16); }
Int128Column::Int128Column(const char *data, size_t sz, size_t count) {
    data_.Reserve(count, ElemSize);
    for (size_t i = 0; i < count; ++i) {
        Push(data, sz);
    }
}
Int128Column::Int128Column(ByteVector &&data) { data_ = std::move(data); }
size_t Int128Column::Size() const { return data_.Size(); }
ColumnTypes Int128Column::GetColumnType() const { return ColumnTypes::Int128; }

void Int128Column::Push(const std::string &s) {
    __int128 result = 0;
    bool negative = false;
    size_t pos = 0;
    if (!s.empty() && s[0] == '-') {
        negative = true;
        pos = 1;
    }
    for (; pos < s.size(); ++pos) {
        if (!std::isdigit(s[pos])) {
            throw std::invalid_argument("Incorrect int128 value");
        }
        result = result * 10 + (s[pos] - '0');
    }
    if (negative) {
        result = -result;
    }
    data_.Push(&result, ElemSize);
}

std::pair<const char *, size_t> Int128Column::ToWrite() {
    return {data_.Data(), data_.SizeInBytes()};
}

void Int128Column::Push(const char *data, size_t sz) {
    if (sz != ElemSize) {
        throw std::invalid_argument("Incorrect data size");
    }
    data_.Push(data, sz);
}

std::string Int128Column::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    const __int128 *data =
        reinterpret_cast<const __int128 *>(data_.Data() + index * ElemSize);
    __int128 value = *data;
    if (value == 0) {
        return "0";
    }

    std::string result;
    bool negative = value < 0;
    if (negative) {
        value = -value;
    }

    while (value > 0) {
        result = static_cast<char>('0' + value % 10) + result;
        value /= 10;
    }

    return negative ? "-" + result : result;
}

void Int128Column::Clear() { data_.Clear(); }

ColumnValueView Int128Column::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return {data_.Data() + index * ElemSize, ElemSize};
}

DoubleColumn::DoubleColumn(const size_t sz) { data_.Reserve(sz, ElemSize); }
DoubleColumn::DoubleColumn(const char *data, size_t sz, size_t count) {
    data_.Reserve(count, ElemSize);
    for (size_t i = 0; i < count; ++i) {
        Push(data, sz);
    }
}
DoubleColumn::DoubleColumn(ByteVector &&data) { data_ = std::move(data); }

size_t DoubleColumn::Size() const { return data_.Size(); }

void DoubleColumn::Push(const std::string &s) {
    double result = std::stod(s);
    data_.Push(&result, ElemSize);
}

void DoubleColumn::Push(const char *data, size_t sz) {
    if (sz != ElemSize) {
        throw std::invalid_argument("Incorrect data size");
    }
    data_.Push(data, sz);
}

std::string DoubleColumn::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    const double *data =
        reinterpret_cast<const double *>(data_.Data() + index * ElemSize);
    return std::to_string(*data);
}

void DoubleColumn::Clear() { data_.Clear(); }

ColumnValueView DoubleColumn::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return {data_.Data() + index * ElemSize, ElemSize};
}

std::pair<const char *, size_t> DoubleColumn::ToWrite() {
    return {data_.Data(), data_.SizeInBytes()};
}

ColumnTypes DoubleColumn::GetColumnType() const { return ColumnTypes::Double; }

Int16Column::Int16Column(const size_t sz) { data_.Reserve(sz, 2); }
Int32Column::Int32Column(const size_t sz) { data_.Reserve(sz, 4); }
Int16Column::Int16Column(const char *data, size_t sz, size_t count) {
    data_.Reserve(count, ElemSize);
    for (size_t i = 0; i < count; ++i) {
        Push(data, sz);
    }
}
Int32Column::Int32Column(const char *data, size_t sz, size_t count) {
    data_.Reserve(count, ElemSize);
    for (size_t i = 0; i < count; ++i) {
        Push(data, sz);
    }
}
Int16Column::Int16Column(ByteVector &&data) { data_ = std::move(data); }
Int32Column::Int32Column(ByteVector &&data) { data_ = std::move(data); }

size_t Int16Column::Size() const { return data_.Size(); }
size_t Int32Column::Size() const { return data_.Size(); }

ColumnTypes Int16Column::GetColumnType() const { return ColumnTypes::Int16; }
ColumnTypes Int32Column::GetColumnType() const { return ColumnTypes::Int32; }

void Int16Column::Push(const std::string &s) {
    int16_t result = static_cast<int16_t>(std::stoi(s));
    data_.Push(&result, ElemSize);
}

void Int32Column::Push(const std::string &s) {
    int32_t result = static_cast<int32_t>(std::stol(s));
    data_.Push(&result, ElemSize);
}

void Int16Column::Push(const char *data, size_t sz) {
    if (sz != ElemSize) {
        throw std::invalid_argument("Incorrect data size");
    }
    data_.Push(data, sz);
}

void Int32Column::Push(const char *data, size_t sz) {
    if (sz != ElemSize) {
        throw std::invalid_argument("Incorrect data size");
    }
    data_.Push(data, sz);
}

std::string Int16Column::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    const int16_t *data =
        reinterpret_cast<const int16_t *>(data_.Data() + index * ElemSize);
    return std::to_string(*data);
}

std::string Int32Column::ToString(const size_t index) {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    const int32_t *data =
        reinterpret_cast<const int32_t *>(data_.Data() + index * ElemSize);
    return std::to_string(*data);
}

void Int16Column::Clear() { data_.Clear(); }
void Int32Column::Clear() { data_.Clear(); }

ColumnValueView Int16Column::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return {data_.Data() + index * ElemSize, ElemSize};
}

ColumnValueView Int32Column::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return {data_.Data() + index * ElemSize, ElemSize};
}

std::pair<const char *, size_t> Int16Column::ToWrite() {
    return {data_.Data(), data_.SizeInBytes()};
}

std::pair<const char *, size_t> Int32Column::ToWrite() {
    return {data_.Data(), data_.SizeInBytes()};
}
