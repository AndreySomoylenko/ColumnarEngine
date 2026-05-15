#pragma once

#include "data_structures/ByteVector.h"
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

enum ColumnTypes {
    Int16,
    Int32,
    Int64,
    Int128,
    String,
    Unknown,
    Timestamp,
    Date,
    Double
};

std::string GetNameByType(ColumnTypes type);

struct ColumnValueView {
    const char *data;
    size_t size;
};

class Column : public std::enable_shared_from_this<Column> {
  public:
    virtual size_t Size() const = 0;
    virtual void Push(const std::string &s) = 0;
    virtual void Push(const char *data, size_t sz) = 0;
    virtual std::string ToString(const size_t index) = 0;
    virtual ColumnValueView Get(size_t index) const = 0;
    virtual void Clear() = 0;

    virtual std::pair<const char *, size_t> ToWrite() = 0;

    virtual ColumnTypes GetColumnType() const = 0;

    virtual const std::vector<size_t> &GetOffsets() const;

    virtual ~Column() = default;

    bool operator<(const Column &other) const;
    bool operator==(const Column &other) const;
    bool operator!=(const Column &other) const;
    bool operator>(const Column &other) const;
    bool operator<=(const Column &other) const;
    bool operator>=(const Column &other) const;

  protected:
    ByteVector data_;
};

class Int16Column : public Column {
  public:
    Int16Column() = default;
    Int16Column(const size_t sz);
    Int16Column(const char *data, size_t sz, size_t count);
    Int16Column(ByteVector &&data);

    size_t Size() const override;
    void Push(const std::string &s) override;
    void Push(const char *data, size_t sz) override;
    std::string ToString(const size_t index) override;
    void Clear() override;
    ColumnValueView Get(size_t index) const override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

  private:
    static constexpr size_t ElemSize = 2;
};

class Int32Column : public Column {
  public:
    Int32Column() = default;
    Int32Column(const size_t sz);
    Int32Column(const char *data, size_t sz, size_t count);
    Int32Column(ByteVector &&data);

    size_t Size() const override;
    void Push(const std::string &s) override;
    void Push(const char *data, size_t sz) override;
    std::string ToString(const size_t index) override;
    void Clear() override;
    ColumnValueView Get(size_t index) const override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

  private:
    static constexpr size_t ElemSize = 4;
};

class Int64Column : public Column {
  public:
    Int64Column() = default;
    Int64Column(const size_t sz);
    Int64Column(const char *data, size_t sz, size_t count);

    Int64Column(ByteVector &&data);

    size_t Size() const override;
    void Push(const std::string &s) override;
    void Push(const char *data, size_t sz) override;
    std::string ToString(const size_t index) override;
    void Clear() override;
    ColumnValueView Get(size_t index) const override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

  private:
    static constexpr size_t ElemSize = 8;
};

class Int128Column : public Column {
  public:
    Int128Column() = default;
    Int128Column(const size_t sz);
    Int128Column(const char *data, size_t sz, size_t count);
    Int128Column(ByteVector &&data);

    size_t Size() const override;
    void Push(const std::string &s) override;
    void Push(const char *data, size_t sz) override;
    std::string ToString(const size_t index) override;
    void Clear() override;
    ColumnValueView Get(size_t index) const override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

  private:
    static constexpr size_t ElemSize = 16;
};

class DoubleColumn : public Column {
  public:
    DoubleColumn() = default;
    DoubleColumn(const size_t sz);
    DoubleColumn(const char *data, size_t sz, size_t count);
    DoubleColumn(ByteVector &&data);

    size_t Size() const override;
    void Push(const std::string &s) override;
    void Push(const char *data, size_t sz) override;
    std::string ToString(const size_t index) override;
    void Clear() override;
    ColumnValueView Get(size_t index) const override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

  private:
    static constexpr size_t ElemSize = sizeof(double);
};

class StringColumn : public Column {
  public:
    StringColumn() = default;
    StringColumn(const size_t sz);
    StringColumn(const char *data, size_t sz, size_t count);

    StringColumn(ByteVector &&data, std::vector<size_t> &&offsets);

    size_t Size() const override;
    void Push(const std::string &s) override;
    void Push(const char *data, size_t sz) override;
    std::string ToString(const size_t index) override;
    ColumnValueView Get(size_t index) const override;
    void Clear() override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

    const std::vector<size_t> &GetOffsets() const override;

  private:
    std::vector<size_t> offsets_;
};

class TimeColumn : public Column {
  public:
    TimeColumn() = default;
    explicit TimeColumn(bool is_date);
    TimeColumn(const size_t sz, bool is_date = false);
    TimeColumn(const char *data, size_t sz, size_t count,
               bool is_date = false);

    TimeColumn(ByteVector &&data, bool is_date = false);
    size_t Size() const override;
    void Push(const std::string &s) override;
    void Push(const char *data, size_t sz) override;
    std::string ToString(const size_t index) override;
    ColumnValueView Get(size_t index) const override;
    void Clear() override;

    std::pair<const char *, size_t> ToWrite() override;

    ColumnTypes GetColumnType() const override;

  private:
    bool is_date_ = false;
    static constexpr size_t ElemSize =
        sizeof(std::chrono::system_clock::time_point);
};
