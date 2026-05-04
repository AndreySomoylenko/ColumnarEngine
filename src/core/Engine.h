#pragma once

#include "data_structures/Scheme.h"
#include "io/ColumnarReader.h"

class Engine {
  public:
    Engine(const Filename &data, const Filename &scheme,
           const Filename &columnar = "test.hub");
    Engine(const Filename &columnar);

    void TakeAll(const Filename &result_name);

  private:
    ColumnarReader reader_;
};