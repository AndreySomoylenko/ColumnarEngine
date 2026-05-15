#include "Column.h"
#include <chrono>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

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

std::chrono::system_clock::time_point ParseTimestamp(
    const std::string &value, bool is_date) {
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
    const year_month_day ymd{
        year{year_value}, month{static_cast<unsigned>(month_value)},
        day{static_cast<unsigned>(day_value)}};
    if (!ymd.ok()) {
        throw std::invalid_argument("Incorrect timestamp");
    }

    const sys_seconds timestamp =
        sys_days{ymd} + hours{hour_value} + minutes{minute_value} +
        seconds{second_value};
    return time_point_cast<system_clock::duration>(timestamp);
}

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

} // namespace

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

    return FormatTimestamp(*reinterpret_cast<
                               const std::chrono::system_clock::time_point *>(
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
Int64Column::Int64Column(const size_t sz) { data_.Reserve(sz, 8); }
TimeColumn::TimeColumn(bool is_date) : is_date_(is_date) {}
TimeColumn::TimeColumn(const size_t sz, bool is_date) : is_date_(is_date) {
    data_.Reserve(sz, ElemSize);
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

const std::vector<size_t> &Int64Column::GetOffsets() const {
    throw std::logic_error("Int64 column doesn't have offsets");
}

const std::vector<size_t> &TimeColumn::GetOffsets() const {
    throw std::logic_error("Timestamp column doesn't have offsets");
}

std::variant<const char *, std::pair<const char *, size_t>>
Int64Column::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return data_.Data() + index * ElemSize;
}

std::variant<const char *, std::pair<const char *, size_t>>
StringColumn::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    const size_t start = offsets_[index];
    const size_t end =
        index + 1 < offsets_.size() ? offsets_[index + 1] : data_.SizeInBytes();
    return std::make_pair(data_.Data() + start, end - start);
}

std::variant<const char *, std::pair<const char *, size_t>>
TimeColumn::Get(size_t index) const {
    if (index >= Size()) {
        throw std::invalid_argument("Too big index");
    }
    return data_.Data() + index * sizeof(std::chrono::system_clock::time_point);
}
