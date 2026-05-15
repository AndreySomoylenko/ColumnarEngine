#pragma once

#include "Column.h"
#include "Scheme.h"
#include <optional>
#include <unordered_set>

using Row = std::vector<std::string>;

class Batch {
  public:
    Batch(const Scheme &scheme, bool reserve = true);

    Batch(Scheme &&scheme, bool reserve = true);
    void AddRow(const Row &row);
    void AddToColumn(const std::string &value, const size_t index);
    void AddToColumn(const char *data, size_t sz, const size_t index);
    void AddColumn(std::shared_ptr<Column> column, ColumnTypes type,
                   const std::string &name);
    void RemoveColumn(size_t index);
    bool EnableToPush();
    size_t HorizontalSize() const;
    size_t VerticalSize() const;
    std::vector<std::shared_ptr<Column>> &GetColumns();
    const std::vector<std::shared_ptr<Column>> &GetColumns() const;
    const std::shared_ptr<Column> &GetColumn(const size_t index) const;

    Row GetRow(const size_t index) const;
    const std::vector<std::shared_ptr<Column>>
    GetRowLikeColumnVector(const size_t index) const;
    void PushColumnVector(const std::vector<std::shared_ptr<Column>> &row);

    void SetEnabledColumns(std::optional<std::unordered_set<size_t>> &&enabled);
    const std::optional<std::unordered_set<size_t>> &GetEnabledColumns() const;
    std::optional<std::unordered_set<size_t>> &GetEnabledColumns();
    bool IsRowEnabled(size_t index) const;

    void Clear();

    const Scheme &GetScheme() const;

    bool IsEmpty() const;

  private:
    std::vector<std::shared_ptr<Column>> columns_;
    static constexpr size_t kMaxRowsPerBatch = 4096;
    static constexpr size_t kPredictedSize = 300000;

    std::optional<std::unordered_set<size_t>> enabled_;

    Scheme scheme_;
};
