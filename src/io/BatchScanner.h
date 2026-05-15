#pragma once

#include "data_structures/Batch.h"
class BatchScanner {
  public:
    virtual ~BatchScanner() = default;
    virtual Batch ReadNext() = 0;
    virtual bool IsEnd() const = 0;
    void Reset() { cur_index_ = 0; }

  protected:
    size_t cur_index_ = 0;
};
