#include "CSVReader.h"
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

CSVReader::CSVReader(const std::string &filename, char sep)
    : sep_(sep)
    , buffer_(1 << 20) {
    is_ = std::ifstream(filename, std::ios::binary);

    is_.rdbuf()->pubsetbuf(buffer_.data(), buffer_.size());
    if (!is_.good()) {
        throw std::invalid_argument("You give me really bad file");
    }
    cur_string_.reserve(256);

    tmp_.reserve(4096);
}

void CSVReader::ReadNext(Raw &result) {
    result.clear();
    cur_string_.clear();

    bool in_quotes = false;

    if (!std::getline(is_, tmp_))
        return;

    do {
        for (size_t i = 0; i < tmp_.size(); ++i) {
            char c = tmp_[i];

            if (in_quotes) {
                if (c == '"') {
                    if (i + 1 < tmp_.size() && tmp_[i + 1] == '"') {
                        cur_string_.push_back('"');
                        ++i;
                    } else {
                        in_quotes = false;
                    }
                } else {
                    cur_string_.push_back(c);
                }
                continue;
            }

            if (c == '"') {
                in_quotes = true;
            } else if (c == sep_) {
                result.emplace_back(std::move(cur_string_));
                cur_string_.clear();
            } else {
                cur_string_.push_back(c);
            }
        }

        if (in_quotes) {
            cur_string_.push_back('\n');
        }

    } while (in_quotes && std::getline(is_, tmp_));

    if (in_quotes)
        throw std::invalid_argument("Bad file");

    result.emplace_back(std::move(cur_string_));
    cur_string_.clear();
}

bool CSVReader::IsEnd() { return is_.eof(); }

CSVReader::~CSVReader() { is_.close(); }