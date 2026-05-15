#pragma once

#include "data_structures/Butch.h"
class ButchScanner {
  public:
    virtual ~ButchScanner() = default;
    virtual Butch ReadNext() = 0;
    virtual bool IsEnd() const = 0;
    void Reset() { cur_index_ = 0; }

  protected:
    size_t cur_index_ = 0;
};
