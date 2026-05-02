#include "ByteVector.h"
#include <algorithm>

ByteVector::ByteVector()
    : capasity_(8 * 1024)
    , size_(0)
    , size_in_bytes_(0)
    , data_(malloc(capasity_)) {}

ByteVector::ByteVector(size_t size, size_t size_in_bytes, void *buf) noexcept
    : capasity_(size_in_bytes)
    , size_(size)
    , size_in_bytes_(size_in_bytes)
    , data_(buf) {}

void ByteVector::ReAllocate(size_t new_cap) {
    void *new_buf = realloc(data_, new_cap);
    capasity_ = new_cap;
    data_ = new_buf;
}

void ByteVector::ReAllocate() { ReAllocate(capasity_ * 2); }

size_t ByteVector::Size() const { return size_; }

size_t ByteVector::SizeInBytes() const { return size_in_bytes_; }

void ByteVector::Reserve(size_t size_in_bytes) {
    capasity_ = size_in_bytes;
    ReAllocate(capasity_);
}

void ByteVector::Reserve(size_t size, size_t element_size) { Reserve(size * element_size); }

void ByteVector::Push(const void *data, size_t size_in_bytes) {
    if (size_in_bytes_ + size_in_bytes > capasity_) {
        ReAllocate(std::max(size_in_bytes + size_in_bytes_, capasity_ * 2));
    }

    memcpy(static_cast<char *>(data_) + size_in_bytes_, static_cast<const char *>(data), size_in_bytes);
    ++size_;
    size_in_bytes_ += size_in_bytes;
}

void ByteVector::Clear() {
    size_ = 0;
    size_in_bytes_ = 0;
}

const char *ByteVector::Data() const { return static_cast<char *>(data_); }

ByteVector::ByteVector(ByteVector &&other)
    : capasity_(other.capasity_)
    , size_(other.size_)
    , size_in_bytes_(other.size_in_bytes_)
    , data_(other.data_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.size_in_bytes_ = 0;
    other.capasity_ = 0;
}

ByteVector &ByteVector::operator=(ByteVector &&other) {
    if (this != &other) {
        free(data_);

        capasity_ = other.capasity_;
        size_ = other.size_;
        size_in_bytes_ = other.size_in_bytes_;
        data_ = other.data_;

        other.data_ = nullptr;
        other.size_ = 0;
        other.size_in_bytes_ = 0;
        other.capasity_ = 0;
    }

    return *this;
}
