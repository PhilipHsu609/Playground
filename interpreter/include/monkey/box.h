#pragma once

#include <memory>

template <typename T>
class Box {
  public:
    Box(T &&obj) // NOLINT(google-explicit-constructor)
                 // for easier construction of Box<T>
        : ptr_(std::make_unique<T>(std::move(obj))) {}
    Box(const T &obj) // NOLINT(google-explicit-constructor)
                      // for easier construction of Box<T>
        : ptr_(std::make_unique<T>(obj)) {}

    Box(const Box &other) : Box(*other.ptr_) {}
    Box &operator=(const Box &other) {
        if (this == &other) {
            return *this;
        }
        *ptr_ = *other.ptr_;
        return *this;
    }

    Box(Box &&) noexcept = default;
    Box &operator=(Box &&) noexcept = default;

    ~Box() = default;

    auto &&operator*(this auto &&self) { return *self.ptr_; }
    auto operator->(this auto &&self) { return self.ptr_.get(); }

  private:
    std::unique_ptr<T> ptr_;
};