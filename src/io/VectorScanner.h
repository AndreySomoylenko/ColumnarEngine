#pragma once

#include "io/ButchScanner.h"

class VectorScanner : public ButchScanner {
  public:
    VectorScanner(const std::vector<Butch> &butches);
    VectorScanner(std::vector<Butch> &&butches);
    Butch ReadNext() override;
    bool IsEnd() const override;

  private:
    std::vector<Butch> butches_;
};