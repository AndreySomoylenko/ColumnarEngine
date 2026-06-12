#include "core/ProjectionBuilder.h"

#include "data_structures/Column.h"

#include <stdexcept>
#include <string>

ProjectionBuilder::ProjectionBuilder(const Scheme &source_scheme)
    : source_scheme_(source_scheme) {}

size_t ProjectionBuilder::Require(hits::HitsColumn column) {
    const size_t source_index = hits::Col(column);
    if (source_index >= source_scheme_.GetSchemeNames().size()) {
        throw std::out_of_range("Incorrect source column index");
    }

    if (auto it = source_to_local_.find(source_index);
        it != source_to_local_.end()) {
        return it->second;
    }

    const size_t local_index = read_scheme_.GetSchemeNames().size();
    const ColumnTypes type = source_scheme_.GetSchemeTypes()[source_index];
    read_scheme_.Add({std::string(hits::Name(column)), GetNameByType(type)});
    source_to_local_[source_index] = local_index;

    return local_index;
}

const Scheme &ProjectionBuilder::ReadScheme() const { return read_scheme_; }
