#pragma once

#include "data_structures/Batch.h"
#include "data_structures/Scheme.h"
#include "io/BatchScanner.h"
#include "io/ColumnarReader.h"
#include "io/CsvWriter.h"
#include "operations/Operations.h"
#include <memory>
#include <string>
#include <vector>

class Pipeline {
  public:
    Pipeline(std::vector<std::unique_ptr<Operation>> &&operations,
             ColumnarReader &reader, const Scheme &scheme,
             const std::string &result_filename = "result.csv");
    Pipeline(std::vector<std::unique_ptr<Operation>> &&operations,
             ColumnarReader &reader, Scheme &&scheme,
             const std::string &result_filename = "result.csv");
    void Execute();

  private:
    void Write(const Batch &batch);

    std::vector<std::unique_ptr<Operation>> operations_;
    CsvWriter writer_;
    std::unique_ptr<BatchScanner> scanner_;
};
