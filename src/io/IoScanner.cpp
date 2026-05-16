#include "io/IoScanner.h"
#include "data_structures/Batch.h"

IoScanner::IoScanner(const Scheme &scheme, ColumnarReader &reader)
    : scheme_(scheme), reader_(reader) {}

IoScanner::IoScanner(Scheme &&scheme, ColumnarReader &reader)
    : scheme_(std::move(scheme)), reader_(reader) {}

Batch IoScanner::ReadNext() {
    if (IsEnd()) {
        throw std::out_of_range("No more data to read");
    }

    return reader_.ReadNext(scheme_, cur_index_);
}

bool IoScanner::IsEnd() const { return reader_.IsEnd(cur_index_); }
