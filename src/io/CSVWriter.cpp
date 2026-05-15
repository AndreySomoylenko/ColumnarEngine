#include "CSVWriter.h"
#include "utils/StringConverter.h"
#include <fstream>
#include <stdexcept>

CSVWriter::CSVWriter(const std::string &filename, char sep) : sep_(sep) {
    os_.rdbuf()->pubsetbuf(io_buffer_.data(),
                           static_cast<std::streamsize>(io_buffer_.size()));
    os_.open(filename, std::ios::binary | std::ios::out);

    if (!os_.good()) {
        throw std::invalid_argument("I cannot create file!");
    }
}

void CSVWriter::WriteRow(const Row &row) {
    StringConverter converter;

    for (size_t i = 0; i < row.size(); ++i) {
        converter.WriteString(os_, row[i]);

        if (i != row.size() - 1) {
            os_.write(&sep_, 1);
        }
    }

    char n = '\n';

    os_.write(&n, 1);
}

CSVWriter::~CSVWriter() { os_.close(); }
