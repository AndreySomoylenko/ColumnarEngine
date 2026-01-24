#include "CSVReader.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

CSVReader::CSVReader(const std::string &filename, char sep)
    : sep_(sep) {
    is_ = std::ifstream(filename);
    if (!is_.good()) {
        throw std::invalid_argument("You give me really bad file");
    }
}

Raw CSVReader::ReadNext() {
    Raw result;
    std::string raw;
    bool is_string = false;
    std::string cur;

    while ((is_string || is_.peek() != '\n') && is_.peek() != EOF) {
        char x = static_cast<char>(is_.get());

        if (x == '"') {
            if (is_string) {
                if (is_.peek() == '"') {
                    cur += x;
                    is_.get();
                } else {
                    if (is_.peek() != '\n' && is_.peek() != EOF && is_.peek() != sep_) {
                        std::cout << char(is_.peek()) << std::endl;
                        throw std::runtime_error("Invalid CSV");
                    }

                    is_string = false;
                }
            } else {
                is_string = true;
            }

            continue;
        }

        if (x == sep_) {
            if (is_string) {
                cur += x;
            } else {
                result.emplace_back(cur);
                cur = "";
            }
        } else {
            cur += x;
        }
    }

    is_.get();

    result.emplace_back(cur);

    if (is_string) {
        throw std::runtime_error("Very bad input");
    }

    return result;
}

bool CSVReader::IsEnd() { return is_.peek() == EOF; }

CSVReader::~CSVReader() { is_.close(); }