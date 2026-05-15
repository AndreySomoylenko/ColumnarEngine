#pragma once

#include <fstream>
#include <string>
#include <vector>

using Row = std::vector<std::string>;

class CSVReader {
  public:
    CSVReader(const std::string &filename, char sep = ',');

    void ReadNext(Row &row);
    bool IsEnd();

    ~CSVReader();

  private:
    std::ifstream is_;
    char sep_;

    std::string cur_string_;

    std::string tmp_;

};
