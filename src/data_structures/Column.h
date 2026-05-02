#pragma once

#include "data_structures/ByteVector.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

enum ColumnTypes { Int64, String, Unknown };

class Column : public std::enable_shared_from_this<Column> {
public:
    virtual size_t Size() = 0;
    virtual void Push(const std::string &s) = 0;
    virtual std::string ToString(const size_t index) = 0;
    virtual void Clear() = 0;

    virtual std::pair<const char *, size_t> ToWrite() = 0;

    virtual ColumnTypes GetColumnType() const = 0;

    virtual const std::vector<size_t> &GetOffsets() const = 0;

    virtual ~Column() = default;

protected:
    ByteVector data_;
};

class Int64Column : public Column {
public:
    Int64Column() = default;
    Int64Column(const size_t sz);

    Int64Column(ByteVector &&data);

    size_t Size() override;
    void Push(const std::string &s) override;
    std::string ToString(const size_t index) override;
    void Clear() override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

    const std::vector<size_t> &GetOffsets() const override;

private:
    static constexpr size_t ElemSize = 8;
};

class StringColumn : public Column {
public:
    StringColumn() = default;
    StringColumn(const size_t sz);

    StringColumn(ByteVector &&data, std::vector<size_t> &&offsets);

    size_t Size() override;
    void Push(const std::string &s) override;
    std::string ToString(const size_t index) override;
    void Clear() override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

    const std::vector<size_t> &GetOffsets() const override;

private:
    std::vector<size_t> offsets_;
};
