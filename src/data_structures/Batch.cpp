#include "Batch.h"
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

Batch::Batch(const Scheme &scheme, bool reserve) : scheme_(scheme) {
    for (auto &x : scheme_.GetSchemeTypes()) {
        columns_.emplace_back(MakeColumn(x, reserve, kPredictedSize));
    }
}

Batch::Batch(Scheme &&scheme, bool reserve) : scheme_(std::move(scheme)) {
    for (auto &x : scheme_.GetSchemeTypes()) {
        columns_.emplace_back(MakeColumn(x, reserve, kPredictedSize));
    }
}

void Batch::AddRow(const Row &row) {
    if (row.size() != columns_.size()) {
        throw std::invalid_argument("Row size isn't correct");
    }

    for (size_t i = 0; i < row.size(); ++i) {
        columns_[i]->Push(row[i]);
    }
}

bool Batch::EnableToPush() { return VerticalSize() < kMaxRowsPerBatch; }

size_t Batch::HorizontalSize() const { return columns_.size(); }
size_t Batch::VerticalSize() const {
    if (columns_.empty()) {
        return 0;
    }
    return columns_[0]->Size();
}

std::vector<std::shared_ptr<Column>> &Batch::GetColumns() { return columns_; }
const std::vector<std::shared_ptr<Column>> &Batch::GetColumns() const {
    return columns_;
}

void Batch::Clear() {
    for (auto &x : columns_) {
        x->Clear();
    }
}

Row Batch::GetRow(const size_t index) {
    if (index >= VerticalSize()) {
        throw std::out_of_range("Incorrect row index");
    }

    Row result;
    for (size_t i = 0; i < HorizontalSize(); ++i) {
        result.emplace_back(columns_[i]->ToString(index));
    }

    return result;
}

void Batch::AddToColumn(const std::string &value, const size_t index) {
    columns_[index]->Push(value);
}

void Batch::AddToColumn(const char *data, size_t sz, const size_t index) {
    columns_[index]->Push(data, sz);
}

void Batch::AddColumn(std::shared_ptr<Column> column, ColumnTypes type,
                      const std::string &name) {
    if (column == nullptr) {
        throw std::invalid_argument("Column is null");
    }
    if (column->GetColumnType() != type) {
        throw std::invalid_argument("Column type doesn't match scheme type");
    }
    if (!columns_.empty() && column->Size() != VerticalSize()) {
        throw std::invalid_argument("Column height doesn't match batch height");
    }

    scheme_.Add({name, GetNameByType(type)});
    columns_.emplace_back(std::move(column));
}

void Batch::RemoveColumn(size_t index) {
    if (index >= HorizontalSize()) {
        throw std::out_of_range("Incorrect column index");
    }

    columns_.erase(columns_.begin() + index);
    scheme_.RemoveColumn(index);
}

void Batch::SetEnabledColumns(
    std::optional<std::unordered_set<size_t>> &&enabled) {
    enabled_ = std::move(enabled);
}

const std::optional<std::unordered_set<size_t>> &
Batch::GetEnabledColumns() const {
    return enabled_;
}

std::optional<std::unordered_set<size_t>> &&Batch::GetEnabledColumns() {
    return std::move(enabled_);
}

bool Batch::IsRowEnabled(size_t index) const {
    if (index >= VerticalSize()) {
        throw std::out_of_range("Incorrect row index");
    }

    return !enabled_.has_value() || enabled_->contains(index);
}

const std::shared_ptr<Column> &Batch::GetColumn(const size_t index) const {
    if (index >= HorizontalSize()) {
        throw std::out_of_range("Incorrect column index");
    }

    return columns_[index];
}

const Scheme &Batch::GetScheme() const { return scheme_; }

const std::vector<std::shared_ptr<Column>>
Batch::GetRowLikeColumnVector(const size_t index) const {
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

void Batch::PushColumnVector(const std::vector<std::shared_ptr<Column>> &row) {
    for (size_t i = 0; i < HorizontalSize(); ++i) {
        auto to_push = row[i]->Get(0);
        columns_[i]->Push(to_push.data, to_push.size);
    }
}

bool Batch::IsEmpty() const { return VerticalSize() == 0; }
