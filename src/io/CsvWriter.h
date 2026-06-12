#pragma once

#include <fstream>
#include <string>
#include <vector>

using Row = std::vector<std::string>;

class CsvWriter {
  public:
    explicit CsvWriter(const std::string &filename, char sep = ',');

    void WriteRow(const Row &row);
    void Flush();

    ~CsvWriter();

  private:
    std::ofstream os_;
    char sep_;
};
