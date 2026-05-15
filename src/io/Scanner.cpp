#include "Scanner.h"
#include "data_structures/Butch.h"

Scanner::Scanner(const std::vector<size_t> &columns_to_read,
                 ColumnarReader &reader)
    : columns_to_read_(columns_to_read), reader_(reader) {}

Butch Scanner::ReadNext() {
    if (IsEnd()) {
        throw std::out_of_range("No more data to read");
    }

    return reader_.ReadNext(columns_to_read_, cur_index_);
}

bool Scanner::IsEnd() { return reader_.IsEnd(cur_index_); }

void Scanner::Reset() { cur_index_ = 0; }