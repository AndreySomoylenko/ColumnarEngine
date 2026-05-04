#include "Butch.h"
#include "data_structures/Column.h"
#include <memory>
#include <stdexcept>

Butch::Butch(std::vector<size_t> &&column_indexes, const Scheme &scheme,
             bool reserve)
    : column_indexes_(std::move(column_indexes)) {
    const auto &scheme_types = scheme.GetSchemeTypes();
    for (auto &ind : column_indexes_) {
        if (ind >= scheme_types.size()) {
            throw std::out_of_range("Column index out of range for scheme types");
        }

        auto x = scheme_types[ind];

        if (reserve) {
            if (x == ColumnTypes::Int64) {
                columns_.emplace_back(
                    std::make_shared<Int64Column>(kPredictedSize));
            } else if (x == ColumnTypes::String) {
                columns_.emplace_back(
                    std::make_shared<StringColumn>(kPredictedSize));
            } else if (x == ColumnTypes::Timestamp) {
                columns_.emplace_back(
                    std::make_shared<TimeColumn>(kPredictedSize));
            } else if (x == ColumnTypes::Date) {
                columns_.emplace_back(
                    std::make_shared<TimeColumn>(kPredictedSize, true));
            } else if (x == ColumnTypes::Unknown) {
                columns_.emplace_back(
                    std::make_shared<StringColumn>(kPredictedSize));
            }
        } else {
            if (x == ColumnTypes::Int64) {
                columns_.emplace_back(std::make_shared<Int64Column>());
            } else if (x == ColumnTypes::String) {
                columns_.emplace_back(std::make_shared<StringColumn>());
            } else if (x == ColumnTypes::Timestamp) {
                columns_.emplace_back(std::make_shared<TimeColumn>());
            } else if (x == ColumnTypes::Date) {
                columns_.emplace_back(std::make_shared<TimeColumn>(true));
            } else if (x == ColumnTypes::Unknown) {
                columns_.emplace_back(std::make_shared<StringColumn>());
            }
        }
    }
}

Butch::Butch(const std::vector<size_t> &column_indexes, const Scheme &scheme,
             bool reserve)
    : column_indexes_(column_indexes) {
    for (auto &ind : column_indexes_) {
        auto x = scheme.GetSchemeTypes()[ind];

        if (reserve) {
            if (x == ColumnTypes::Int64) {
                columns_.emplace_back(
                    std::make_shared<Int64Column>(kPredictedSize));
            } else if (x == ColumnTypes::String) {
                columns_.emplace_back(
                    std::make_shared<StringColumn>(kPredictedSize));
            } else if (x == ColumnTypes::Timestamp) {
                columns_.emplace_back(
                    std::make_shared<TimeColumn>(kPredictedSize));
            } else if (x == ColumnTypes::Date) {
                columns_.emplace_back(
                    std::make_shared<TimeColumn>(kPredictedSize, true));
            } else if (x == ColumnTypes::Unknown) {
                columns_.emplace_back(
                    std::make_shared<StringColumn>(kPredictedSize));
            }
        } else {
            if (x == ColumnTypes::Int64) {
                columns_.emplace_back(std::make_shared<Int64Column>());
            } else if (x == ColumnTypes::String) {
                columns_.emplace_back(std::make_shared<StringColumn>());
            } else if (x == ColumnTypes::Timestamp) {
                columns_.emplace_back(std::make_shared<TimeColumn>());
            } else if (x == ColumnTypes::Date) {
                columns_.emplace_back(std::make_shared<TimeColumn>(true));
            } else if (x == ColumnTypes::Unknown) {
                columns_.emplace_back(std::make_shared<StringColumn>());
            }
        }
    }
}

void Butch::AddRaw(const Raw &raw) {
    if (raw.size() != columns_.size()) {
        throw std::invalid_argument("Raw size isn't correct");
    }

    for (size_t i = 0; i < raw.size(); ++i) {
        columns_[i]->Push(raw[i]);
    }
}

bool Butch::EnableToPush() { return VerticalSize() < kMaxRowsPerBatch; }

size_t Butch::HorizontalSize() const { return columns_.size(); }
size_t Butch::VerticalSize() const {
    if (columns_.empty()) {
        return 0;
    }
    return columns_[0]->Size();
}

std::vector<std::shared_ptr<Column>> &Butch::GetColumns() { return columns_; }
const std::vector<std::shared_ptr<Column>> &Butch::GetColumns() const {
    return columns_;
}

void Butch::Clear() {
    for (auto &x : columns_) {
        x->Clear();
    }
}

Raw Butch::GetRaw(const size_t index) {
    if (index >= VerticalSize()) {
        throw std::out_of_range("Incorrect row index");
    }

    Raw result;
    for (size_t i = 0; i < HorizontalSize(); ++i) {
        if (enabled_.has_value() && !enabled_->count(i)) {
            continue;
        }
        result.emplace_back(columns_[i]->ToString(index));
    }

    return result;
}

void Butch::AddToColumn(const std::string &value, const size_t index) {
    columns_[index]->Push(value);
}

void Butch::SetEnabledColumns(std::unordered_set<size_t> &&enabled) {
    enabled_ = std::move(enabled);
}
