#include "StringConverter.h"
#include <fstream>
#include <string>

void StringConverter::WriteString(std::ofstream &os,
                                  const std::string &to_write) {
    std::string tmp = TransformStringToFilestring(to_write);
    os.write(tmp.data(), tmp.size());
}

std::string
StringConverter::TransformStringToFilestring(const std::string &cur) {
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