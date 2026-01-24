#include "CSVWriter.h"
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
    for (size_t i = 0; i < raw.size(); ++i) {
        auto str = raw[i];

        os_ << '"';
        for (auto &x : str) {
            if (x == '"') {
                os_ << x;
            }
            os_ << x;
        }
        os_ << '"';

        if (i != raw.size() - 1) {
            os_ << sep_;
        }
    }

    os_ << '\n';

    os_.flush();
}

CSVWriter::~CSVWriter() { os_.close(); }
