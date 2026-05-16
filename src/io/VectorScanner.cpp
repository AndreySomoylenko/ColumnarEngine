#include "io/VectorScanner.h"

VectorScanner::VectorScanner(const std::vector<Batch> &batches)
    : batches_(batches) {}

VectorScanner::VectorScanner(std::vector<Batch> &&batches)
    : batches_(std::move(batches)) {}

Batch VectorScanner::ReadNext() {
    if (IsEnd()) {
        throw std::out_of_range("No more batches to read");
    }
    return batches_[cur_index_++];
}

bool VectorScanner::IsEnd() const { return cur_index_ >= batches_.size(); }
