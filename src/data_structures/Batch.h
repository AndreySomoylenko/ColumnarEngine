#pragma once

#include "data_structures/Column.h"
#include "data_structures/Containers.h"
#include "data_structures/Scheme.h"

using Row = std::vector<std::string>;

class Batch {
  public:
    explicit Batch(const Scheme &scheme, bool reserve = true);

    explicit Batch(Scheme &&scheme, bool reserve = true);
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
    void PushRowFrom(const Batch &source, size_t row);

    void SetEnabledRaws(EnabledRaws &&enabled);
    const EnabledRaws &GetEnabledRaws() const;
    EnabledRaws &GetEnabledRaws();
    bool IsRowEnabled(size_t index) const;

    void Clear();

    const Scheme &GetScheme() const;

    bool IsEmpty() const;

  private:
    std::vector<std::shared_ptr<Column>> columns_;
    static constexpr size_t kMaxRowsPerBatch = 1 << 14;
    static constexpr size_t kPredictedSize = 300000;

    EnabledRaws enabled_;

    Scheme scheme_;
};
