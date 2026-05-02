#include "CSVReader.h"

#include <fstream>
#include <stdexcept>
#include <string>

CSVReader::CSVReader(const std::string &filename, char sep)
    : sep_(sep) {
    is_.rdbuf()->pubsetbuf(io_buffer_.data(), static_cast<std::streamsize>(io_buffer_.size()));
    is_.open(filename, std::ios::binary);

    if (!is_) {
        throw std::invalid_argument("You give me really bad file");
    }

    tmp_.reserve(1 << 20);
    cur_string_.reserve(1 << 16);
}

void CSVReader::ReadNext(Raw &result) {
    result.clear();

    if (!std::getline(is_, tmp_)) {
        return;
    }

    if (tmp_.empty()) {
        return;
    }

    bool in_quotes = false;
    size_t start = 0;

    while (true) {
        for (size_t i = 0; i < tmp_.size(); ++i) {
            char c = tmp_[i];

            if (in_quotes) {
                if (c == '"') {
                    if (i + 1 < tmp_.size() && tmp_[i + 1] == '"') {
                        cur_string_.append(tmp_, start, i + 1 - start);
                        ++i;
                        start = i + 1;
                    } else {
                        cur_string_.append(tmp_, start, i - start);
                        in_quotes = false;
                        start = i + 1;
                    }
                }
                continue;
            }

            if (c == '"') {
                cur_string_.append(tmp_, start, i - start);
                in_quotes = true;
                start = i + 1;
                continue;
            }

            if (c == sep_) {
                cur_string_.append(tmp_, start, i - start);
                result.emplace_back(std::move(cur_string_));
                cur_string_.clear();
                start = i + 1;
            }
        }

        cur_string_.append(tmp_, start, tmp_.size() - start);

        if (!in_quotes) {
            break;
        }

        cur_string_.push_back('\n');

        if (!std::getline(is_, tmp_)) {
            throw std::invalid_argument("Bad file: unclosed quote");
        }

        if (!tmp_.empty() && tmp_.back() == '\r') {
            tmp_.pop_back();
        }

        start = 0;
    }

    result.emplace_back(std::move(cur_string_));
    cur_string_.clear();
}

bool CSVReader::IsEnd() {
    return !is_ || is_.peek() == EOF;
}

CSVReader::~CSVReader() {
    is_.close();
}