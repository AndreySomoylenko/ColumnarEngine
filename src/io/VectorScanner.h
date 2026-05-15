#pragma once

#include "io/BatchScanner.h"

class VectorScanner : public BatchScanner {
  public:
    VectorScanner(const std::vector<Batch> &batches);
    VectorScanner(std::vector<Batch> &&batches);
    Batch ReadNext() override;
    bool IsEnd() const override;

  private:
    std::vector<Batch> batches_;
};