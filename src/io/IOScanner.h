#pragma once

#include "data_structures/Butch.h"
#include "io/ButchScanner.h"
#include "io/ColumnarReader.h"

class IOScanner : public ButchScanner {
  public:
    IOScanner(const Scheme &scheme, ColumnarReader &reader);
    Butch ReadNext() override;
    bool IsEnd() const override;

  private:
    Scheme scheme_;

    ColumnarReader &reader_;
};