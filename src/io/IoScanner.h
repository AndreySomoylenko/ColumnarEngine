#pragma once

#include "data_structures/Batch.h"
#include "io/BatchScanner.h"
#include "io/ColumnarReader.h"

#include <vector>

class IoScanner : public BatchScanner {
  public:
    IoScanner(const Scheme &scheme, ColumnarReader &reader);
    IoScanner(Scheme &&scheme, ColumnarReader &reader);
    Batch ReadNext() override;
    bool IsEnd() const override;

  private:
    Scheme scheme_;
    std::vector<size_t> columns_to_read_;

    ColumnarReader &reader_;
};
