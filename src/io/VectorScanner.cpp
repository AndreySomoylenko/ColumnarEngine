#include "VectorScanner.h"

VectorScanner::VectorScanner(const std::vector<Butch> &butches)
    : butches_(butches) {}

VectorScanner::VectorScanner(std::vector<Butch> &&butches)
    : butches_(std::move(butches)) {}

Butch VectorScanner::ReadNext() {
    if (IsEnd()) {
        throw std::out_of_range("No more butches to read");
    }
    return butches_[cur_index_++];
}

bool VectorScanner::IsEnd() const { return cur_index_ >= butches_.size(); }