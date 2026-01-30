#include "StringConverter.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

std::string StringConverter::ReadNextString(std::ifstream &is, const std::vector<int> &separators) {
    bool is_big = false;

    if (is.peek() == '"') {
        is_big = true;
        is.get();
    }

    std::string result;

    result.reserve(40);

    while (is_big || !count(separators.begin(), separators.end(), is.peek())) {

        if (is.peek() == EOF) {
            throw std::runtime_error("WTF");
        }
        char x = is.get();

        if (x == '"') {
            if (!is_big) {
                throw std::invalid_argument("WTFF");
            }
            if (!(is.peek() == '"')) {
                break;
            }
            is.get();
        }

        result += x;
    }

    return result;
}

void StringConverter::WriteString(std::ofstream &os, const std::string &to_write) {
    std::string tmp = TransformStringToFilestring(to_write);
    os.write(tmp.data(), tmp.size());
}

std::string StringConverter::TransformStringToFilestring(const std::string &cur) {
    std::string result = "\"";
    result.reserve(2 * cur.size());
    for (auto &x : cur) {
        if (x == '"') {
            result += '"';
        }
        result += x;
    }

    result += '"';

    return result;
}