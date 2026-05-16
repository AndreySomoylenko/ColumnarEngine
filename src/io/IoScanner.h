#pragma once

#include "data_structures/Batch.h"
#include "io/BatchScanner.h"
#include "io/ColumnarReader.h"

class IoScanner : public BatchScanner {
  public:
    IoScanner(const Scheme &scheme, ColumnarReader &reader);
    IoScanner(Scheme &&scheme, ColumnarReader &reader);
    Batch ReadNext() override;
    bool IsEnd() const override;

  private:
    Scheme scheme_;

    ColumnarReader &reader_;
};
