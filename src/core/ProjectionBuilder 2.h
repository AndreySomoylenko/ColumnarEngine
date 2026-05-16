#pragma once

#include "data_structures/Scheme.h"
#include "utils/HitsColumns.h"

#include <cstddef>
#include <unordered_map>

class ProjectionBuilder {
  public:
    explicit ProjectionBuilder(const Scheme &source_scheme);

    size_t Require(hits::HitsColumn column);

    const Scheme &ReadScheme() const;

  private:
    const Scheme &source_scheme_;
    Scheme read_scheme_;
    std::unordered_map<size_t, size_t> source_to_local_;
};
