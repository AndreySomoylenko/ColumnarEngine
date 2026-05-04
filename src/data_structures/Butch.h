#pragma once

#include "Column.h"
#include "Scheme.h"
#include <unordered_set>

using Raw = std::vector<std::string>;

class Butch {
  public:
    Butch(std::vector<size_t> &&column_indexes, const Scheme &scheme, bool reserve = true);

    Butch(const std::vector<size_t> &column_indexes, const Scheme &scheme, bool reserve = true);
    void AddRaw(const Raw &raw);
    void AddToColumn(const std::string &value, const size_t index);
    bool EnableToPush();
    size_t HorizontalSize() const;
    size_t VerticalSize() const;
    std::vector<std::shared_ptr<Column>> &GetColumns();
    const std::vector<std::shared_ptr<Column>> &GetColumns() const;

    Raw GetRaw(const size_t index);

    void SetEnabledColumns(std::unordered_set<size_t> &&enabled);

    void Clear();

  private:
    std::vector<std::shared_ptr<Column>> columns_;
    static constexpr size_t kMaxRowsPerBatch = 4096;
    static constexpr size_t kPredictedSize = 300000;

    std::optional<std::unordered_set<size_t>> enabled_;

    const std::vector<size_t> column_indexes_;
};
