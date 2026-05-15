#include "IOScanner.h"
#include "data_structures/Batch.h"

IOScanner::IOScanner(const Scheme &scheme, ColumnarReader &reader)
    : scheme_(scheme), reader_(reader) {}

IOScanner::IOScanner(Scheme &&scheme, ColumnarReader &reader)
    : scheme_(std::move(scheme)), reader_(reader) {}

Batch IOScanner::ReadNext() {
    if (IsEnd()) {
        throw std::out_of_range("No more data to read");
    }

    return reader_.ReadNext(scheme_, cur_index_);
}

bool IOScanner::IsEnd() const { return reader_.IsEnd(cur_index_); }
