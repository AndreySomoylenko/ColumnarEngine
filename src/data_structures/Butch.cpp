#include "Butch.h"
#include "data_structures/Column.h"
#include <memory>
#include <stdexcept>

Butch::Butch(const Scheme &scheme, bool reserve)
    : scheme_(scheme) {
    for (auto &x : scheme.GetSchemeTypes()) {

        if (reserve) {
            if (x == ColumnTypes::Int64) {
                columns_.emplace_back(std::make_shared<Int64Column>(kPredictedSize));
            } else if (x == ColumnTypes::String) {
                columns_.emplace_back(std::make_shared<StringColumn>(kPredictedSize));
            } else if (x == ColumnTypes::Unknown) {
                columns_.emplace_back(std::make_shared<StringColumn>(kPredictedSize));
            }
        } else {
            if (x == ColumnTypes::Int64) {
                columns_.emplace_back(std::make_shared<Int64Column>());
            } else if (x == ColumnTypes::String) {
                columns_.emplace_back(std::make_shared<StringColumn>());
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
size_t Butch::VerticalSize() const { return columns_[0]->Size(); }

std::vector<std::shared_ptr<Column>> &Butch::GetColumns() { return columns_; }
const std::vector<std::shared_ptr<Column>> &Butch::GetColumns() const { return columns_; }

void Butch::Clear() {
    for (auto &x : columns_) {
        x->Clear();
    }
}

Raw Butch::GetRaw(const size_t index) {
    Raw result;

    for (size_t i = 0; i < HorizontalSize(); ++i) {
        result.emplace_back(columns_[i]->ToString(index));
    }

    return result;
}

void Butch::AddToColumn(const std::string &value, const size_t index) { columns_[index]->Push(value); }
