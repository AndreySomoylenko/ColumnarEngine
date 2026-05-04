#pragma once

#include "Column.h"
#include "Scheme.h"

using Raw = std::vector<std::string>;

class Butch {
public:
    Butch(const Scheme &schem, bool reserve = true);
    void AddRaw(const Raw &raw);
    void AddToColumn(const std::string &value, const size_t index);
    bool EnableToPush();
    size_t HorizontalSize() const;
    size_t VerticalSize() const;
    std::vector<std::shared_ptr<Column>> &GetColumns();
    const std::vector<std::shared_ptr<Column>> &GetColumns() const;

    Raw GetRaw(const size_t index);

    void Clear();

private:
    std::vector<std::shared_ptr<Column>> columns_;
    static constexpr size_t kMaxRowsPerBatch = 4096;
    static constexpr size_t kPredictedSize = 300000;

    const Scheme &scheme_;
};
