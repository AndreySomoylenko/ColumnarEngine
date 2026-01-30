#pragma once

#include "data_structures/Butch.h"
#include <fstream>

class ColumnarWriter {
public:
    ColumnarWriter(const std::string &filename);
    void WriteChunk(const Butch &butch);
    void Close(const Scheme &scheme);

private:
    std::vector<uint64_t> chunk_starts;
    std::ofstream os_;
};
