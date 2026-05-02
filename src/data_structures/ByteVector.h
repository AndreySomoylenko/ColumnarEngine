#pragma once

#include <cstddef>
#include <cstring>

class ByteVector {
public:
    ByteVector(size_t size, size_t size_in_bytes, void *buf) noexcept;
    ByteVector();

    ByteVector(const ByteVector&) = delete;
    ByteVector& operator=(const ByteVector&) = delete;
    
    ByteVector(ByteVector&&);
    ByteVector& operator=(ByteVector&&);

    size_t Size() const;
    size_t SizeInBytes() const;

    void Reserve(size_t size, size_t element_size);
    void Reserve(size_t size_in_bytes);

    void Push(const void* data, size_t sz) ;

    const char *Data() const;

    void Clear();

private:
    size_t capasity_;

    size_t size_;
    size_t size_in_bytes_;

    void *data_;

    void ReAllocate(size_t new_cap);

    void ReAllocate();

    static constexpr size_t kDefaultSize = 24;
};
