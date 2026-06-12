#pragma once

#include "io/BatchScanner.h"

class VectorScanner : public BatchScanner {
  public:
    explicit VectorScanner(const std::vector<Batch> &batches);
    explicit VectorScanner(std::vector<Batch> &&batches);
    Batch ReadNext() override;
    bool IsEnd() const override;

  private:
    std::vector<Batch> batches_;
};
