#pragma once

#include "data_structures/Containers.h"
#include "data_structures/Scheme.h"
#include "utils/HitsColumns.h"

#include <cstddef>

class ProjectionBuilder {
  public:
    explicit ProjectionBuilder(const Scheme &source_scheme);

    size_t Require(hits::HitsColumn column);

    const Scheme &ReadScheme() const;

  private:
    const Scheme &source_scheme_;
    Scheme read_scheme_;
    HashFlatMap<size_t, size_t> source_to_local_;
};
