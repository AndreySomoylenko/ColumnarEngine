#include "CSVReader.h"
#include "utils/StringConverter.h"
#include <cstdio>
#include <fstream>
#include <stdexcept>

CSVReader::CSVReader(const std::string &filename, char sep)
    : sep_(sep) {
    is_ = std::ifstream(filename);
    if (!is_.good()) {
        throw std::invalid_argument("You give me really bad file");
    }
}

Raw CSVReader::ReadNext() {
    StringConverter converter;

    Raw result;

    while (is_.peek() != '\n' && is_.peek() != EOF) {
        result.emplace_back(converter.ReadNextString(is_, {sep_, '\n', EOF}));
        if (is_.peek() == '\n' || is_.peek() == EOF) {
            break;
        }

        is_.get();
    }

    is_.get();

    return result;
}

bool CSVReader::IsEnd() { return is_.peek() == EOF; }

CSVReader::~CSVReader() { is_.close(); }