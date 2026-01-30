#pragma once

#include <memory>
#include <string>
#include <vector>

enum ColumnTypes { Int64, String, Unknown };

class Column : public std::enable_shared_from_this<Column> {
public:
    virtual size_t Size() = 0;
    virtual char *Data() = 0;
    virtual size_t Push(const std::string &s) = 0;
    virtual std::string ToString(const size_t index) = 0;
    virtual void Clear() = 0;

    virtual std::pair<char *, size_t> ToWrite(const size_t index) = 0;

    virtual ~Column() = default;
};

class Int64Column : public Column {
public:
    size_t Size() override;
    char *Data() override;
    size_t Push(const std::string &s) override;
    std::string ToString(const size_t index) override;
    void Clear() override;

    std::pair<char *, size_t> ToWrite(const size_t index) override;

private:
    std::vector<int64_t> data_;
};

class StringColumn : public Column {
public:
    size_t Size() override;
    char *Data() override;
    size_t Push(const std::string &s) override;
    std::string ToString(const size_t index) override;
    void Clear() override;

    std::pair<char *, size_t> ToWrite(const size_t index) override;

private:
    std::vector<std::string> data_;
    std::string cache_;
};
