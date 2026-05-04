#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "FieldTag.hpp"

namespace fast_little_market {

namespace fix {

enum class AllocError { OutOfMemory, Underflow };

class Buffer {
 private:
  std::byte* buffer_;
  std::byte* offset_;
  size_t bytes_allocated_ = 0;

 public:
  const size_t capacity_;

  explicit Buffer(size_t capacity)
      : buffer_(static_cast<std::byte*>(std::malloc(capacity))),
        offset_(buffer_),
        capacity_(capacity) {
    if (!buffer_) throw std::bad_alloc{};
  }

  ~Buffer() { std::free(buffer_); }

  Buffer(const Buffer&) = delete;  // Avoid double free

  Buffer& operator=(const Buffer&) = delete;

  Buffer(Buffer&& other) noexcept
      : buffer_(other.buffer_),
        offset_(other.offset_),
        bytes_allocated_(other.bytes_allocated_),
        capacity_(other.capacity_) {
    other.buffer_ = nullptr;
    other.offset_ = nullptr;
  }

  void* allocate(size_t n) {
    if (offset_ + n > buffer_ + capacity_) throw std::bad_alloc{};

    void* addr = offset_;
    offset_ += n;
    bytes_allocated_ += n;
    return addr;
  }

  void deallocate(size_t n) noexcept {
    if (n > bytes_allocated_) return;
    bytes_allocated_ -= n;
    offset_ -= n;
  }

  void reset() noexcept {
    offset_ = buffer_;
    bytes_allocated_ = 0;
  }

  size_t allocated() const noexcept { return bytes_allocated_; }
  size_t remaining() const noexcept { return capacity_ - bytes_allocated_; }
};

template <typename T>
class Allocator {
 private:
  Buffer& buffer_;

 public:
  using value_type = T;
  explicit Allocator(Buffer& buffer) : buffer_(buffer) {}

  Allocator(const Allocator<T>& other) : buffer_(other.buffer_) {}

  ~Allocator() = default;

  Buffer& buffer() const { return buffer_; }

  T* allocate(size_t n) {
    return static_cast<T*>(buffer_.allocate(n * sizeof(T)));
  }

  void deallocate(T* /*p*/, size_t n) noexcept {
    buffer_.deallocate(n * sizeof(T));
  }
};

}  // namespace fix

}  // namespace fast_little_market