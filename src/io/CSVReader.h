#include <fstream>
#include <string>
#include <vector>

using Raw = std::vector<std::string>;

class CSVReader {
public:
    CSVReader(const std::string &filename, char sep = ';');

    Raw ReadNext();
    bool IsEnd();

    ~CSVReader();

private:
    std::ifstream is_;
    char sep_;
};