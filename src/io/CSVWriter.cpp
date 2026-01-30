#include "CSVWriter.h"
#include "utils/StringConverter.h"
#include <fstream>
#include <stdexcept>

CSVWriter::CSVWriter(const std::string &filename, char sep)
    : sep_(sep) {
    os_ = std::ofstream(filename);
    if (!os_.good()) {
        throw std::invalid_argument("I cannot create file!");
    }
}

void CSVWriter::WriteRaw(const Raw &raw) {
    StringConverter converter;

    for (size_t i = 0; i < raw.size(); ++i) {
        auto str = raw[i];

        converter.WriteString(os_, str);

        if (i != raw.size() - 1) {
            os_.write(&sep_, 1);
        }
    }

    char n = '\n';

    os_.write(&n, 1);
}

CSVWriter::~CSVWriter() { os_.close(); }
