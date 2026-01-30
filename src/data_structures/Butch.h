#pragma once

#include "Column.h"
#include "Scheme.h"

using Raw = std::vector<std::string>;

class Butch {
public:
    Butch(const Scheme &scheme);
    void AddRaw(const Raw &raw);
    void AddToColumn(const std::string &value, const size_t index);
    bool EnableToPush();
    size_t HorizontalSize() const;
    size_t VerticalSize() const;
    const std::vector<std::shared_ptr<Column>> &GetColumns() const;

    Raw GetRaw(const size_t index);

    void Clear();

private:
    std::vector<std::shared_ptr<Column>> columns_;
    static constexpr size_t kLimitButchSize = 256 * 1024 * 1024;
    size_t cur_size_ = 0;
};
