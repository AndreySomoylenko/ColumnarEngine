#pragma once

#include <fstream>
#include <string>
#include <vector>

using Row = std::vector<std::string>;

class CSVWriter {
  public:
    CSVWriter(const std::string &filename, char sep = ',');

    void WriteRow(const Row &row);
    void Flush();

    ~CSVWriter();

  private:
    std::ofstream os_;
    char sep_;
};
