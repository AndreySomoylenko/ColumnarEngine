#pragma once

#include "data_structures/Batch.h"
#include "io/BatchScanner.h"
#include "io/ColumnarReader.h"

class IOScanner : public BatchScanner {
  public:
    IOScanner(const Scheme &scheme, ColumnarReader &reader);
    IOScanner(Scheme &&scheme, ColumnarReader &reader);
    Batch ReadNext() override;
    bool IsEnd() const override;

  private:
    Scheme scheme_;

    ColumnarReader &reader_;
};
