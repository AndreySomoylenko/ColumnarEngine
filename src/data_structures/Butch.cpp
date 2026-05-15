#include "Butch.h"
#include "data_structures/Column.h"
#include <memory>
#include <stdexcept>

namespace {

std::shared_ptr<Column> MakeColumn(ColumnTypes type, bool reserve,
                                   size_t predicted_size) {
    if (reserve) {
        if (type == ColumnTypes::Int16) {
            return std::make_shared<Int16Column>(predicted_size);
        } else if (type == ColumnTypes::Int32) {
            return std::make_shared<Int32Column>(predicted_size);
        } else if (type == ColumnTypes::Int64) {
            return std::make_shared<Int64Column>(predicted_size);
        } else if (type == ColumnTypes::Int128) {
            return std::make_shared<Int128Column>(predicted_size);
        } else if (type == ColumnTypes::Double) {
            return std::make_shared<DoubleColumn>(predicted_size);
        } else if (type == ColumnTypes::String ||
                   type == ColumnTypes::Unknown) {
            return std::make_shared<StringColumn>(predicted_size);
        } else if (type == ColumnTypes::Timestamp) {
            return std::make_shared<TimeColumn>(predicted_size);
        } else if (type == ColumnTypes::Date) {
            return std::make_shared<TimeColumn>(predicted_size, true);
        }
    } else {
        if (type == ColumnTypes::Int16) {
            return std::make_shared<Int16Column>();
        } else if (type == ColumnTypes::Int32) {
            return std::make_shared<Int32Column>();
        } else if (type == ColumnTypes::Int64) {
            return std::make_shared<Int64Column>();
        } else if (type == ColumnTypes::Int128) {
            return std::make_shared<Int128Column>();
        } else if (type == ColumnTypes::Double) {
            return std::make_shared<DoubleColumn>();
        } else if (type == ColumnTypes::String ||
                   type == ColumnTypes::Unknown) {
            return std::make_shared<StringColumn>();
        } else if (type == ColumnTypes::Timestamp) {
            return std::make_shared<TimeColumn>();
        } else if (type == ColumnTypes::Date) {
            return std::make_shared<TimeColumn>(true);
        }
    }

    throw std::invalid_argument("Unsupported column type");
}

} // namespace

Butch::Butch(const Scheme &scheme, bool reserve) : scheme_(scheme) {
    for (auto &x : scheme_.GetSchemeTypes()) {
        columns_.emplace_back(MakeColumn(x, reserve, kPredictedSize));
    }
}

Butch::Butch(Scheme &&scheme, bool reserve) : scheme_(std::move(scheme)) {
    for (auto &x : scheme_.GetSchemeTypes()) {
        columns_.emplace_back(MakeColumn(x, reserve, kPredictedSize));
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
        result.emplace_back(columns_[i]->ToString(index));
    }

    return result;
}

void Butch::AddToColumn(const std::string &value, const size_t index) {
    columns_[index]->Push(value);
}

void Butch::AddToColumn(const char *data, size_t sz, const size_t index) {
    columns_[index]->Push(data, sz);
}

void Butch::AddColumn(std::shared_ptr<Column> column, ColumnTypes type,
                      const std::string &name) {
    if (column == nullptr) {
        throw std::invalid_argument("Column is null");
    }
    if (column->GetColumnType() != type) {
        throw std::invalid_argument("Column type doesn't match scheme type");
    }
    if (!columns_.empty() && column->Size() != VerticalSize()) {
        throw std::invalid_argument("Column height doesn't match butch height");
    }

    scheme_.Add({name, GetNameByType(type)});
    columns_.emplace_back(std::move(column));
}

void Butch::RemoveColumn(size_t index) {
    if (index >= HorizontalSize()) {
        throw std::out_of_range("Incorrect column index");
    }

    columns_.erase(columns_.begin() + index);
    scheme_.RemoveColumn(index);
}

void Butch::SetEnabledColumns(
    std::optional<std::unordered_set<size_t>> &&enabled) {
    enabled_ = std::move(enabled);
}

const std::optional<std::unordered_set<size_t>> &
Butch::GetEnabledColumns() const {
    return enabled_;
}

std::optional<std::unordered_set<size_t>> &&Butch::GetEnabledColumns() {
    return std::move(enabled_);
}

bool Butch::IsRowEnabled(size_t index) const {
    if (index >= VerticalSize()) {
        throw std::out_of_range("Incorrect row index");
    }

    return !enabled_.has_value() || enabled_->contains(index);
}

const std::shared_ptr<Column> &Butch::GetColumn(const size_t index) const {
    if (index >= HorizontalSize()) {
        throw std::out_of_range("Incorrect column index");
    }

    return columns_[index];
}

const Scheme &Butch::GetScheme() const { return scheme_; }

const std::vector<std::shared_ptr<Column>>
Butch::GetRawLikeColumnVector(const size_t index) const {
    std::vector<std::shared_ptr<Column>> result;
    for (size_t i = 0; i < HorizontalSize(); ++i) {
        const auto type = columns_[i]->GetColumnType();
        const auto element = columns_[i]->Get(index);

        switch (type) {
        case ColumnTypes::Int16: {
            auto column = std::make_shared<Int16Column>();
            column->Push(element.data, element.size);
            result.emplace_back(std::move(column));
            break;
        }
        case ColumnTypes::Int32: {
            auto column = std::make_shared<Int32Column>();
            column->Push(element.data, element.size);
            result.emplace_back(std::move(column));
            break;
        }
        case ColumnTypes::Int64: {
            auto column = std::make_shared<Int64Column>();
            column->Push(element.data, element.size);
            result.emplace_back(std::move(column));
            break;
        }
        case ColumnTypes::Int128: {
            auto column = std::make_shared<Int128Column>();
            column->Push(element.data, element.size);
            result.emplace_back(std::move(column));
            break;
        }
        case ColumnTypes::Double: {
            auto column = std::make_shared<DoubleColumn>();
            column->Push(element.data, element.size);
            result.emplace_back(std::move(column));
            break;
        }
        case ColumnTypes::Timestamp: {
            auto column = std::make_shared<TimeColumn>();
            column->Push(element.data, element.size);
            result.emplace_back(std::move(column));
            break;
        }
        case ColumnTypes::Date: {
            auto column = std::make_shared<TimeColumn>(true);
            column->Push(element.data, element.size);
            result.emplace_back(std::move(column));
            break;
        }
        case ColumnTypes::String:
        case ColumnTypes::Unknown: {
            auto column = std::make_shared<StringColumn>();
            column->Push(element.data, element.size);
            result.emplace_back(std::move(column));
            break;
        }
        default:
            throw std::invalid_argument("Unsupported column type");
        }
    }
    return result;
}

void Butch::PushColumnVector(const std::vector<std::shared_ptr<Column>> &raw) {
    for (size_t i = 0; i < HorizontalSize(); ++i) {
        auto to_push = raw[i]->Get(0);
        columns_[i]->Push(to_push.data, to_push.size);
    }
}

bool Butch::IsEmpty() const { return VerticalSize() == 0; }
