#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept
        : buffer_(std::exchange(other.buffer_, nullptr)),
        capacity_(std::exchange(other.capacity_, 0))
    {}

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != rhs) {
            buffer_ = std::move(rhs.buffer_);
            //capacity_ = std::move(rhs.capacity_);
            capacity_ = rhs.capacity_;

            rhs.buffer_ = 0;
            rhs.capacity_ = 0;
        }

        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_)),
        size_(std::exchange(other.size_, 0))
    {}

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    using iterator = T*;
    using const_iterator = const T*;

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);
        MoveOrCopy(new_data);
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        else {
            if (new_size > Capacity()) {
                size_t new_size_capacity = std::max(Capacity() * 2, new_size);
                Reserve(new_size_capacity);
            }
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        int position = pos - begin();

        if (size_ < Capacity()) {
            if (pos != end()) {
                T element(std::forward<Args>(args)...);
                new (end()) T(std::forward<T>(data_[size_ - 1]));
                std::move_backward(begin() + position, end() - 1, end());
                *(begin() + position) = std::forward<T>(element);
            }
            else {
                new (end()) T(std::forward<Args>(args)...);
            }
        }
        else {
            //Resize(0 ? 1 : size_ * 2);
            size_t new_size = size_ == 0 ? 1 : size_ * 2;

            RawMemory<T> tmp(new_size);
            new(tmp.GetAddress() + position) T(std::forward<Args>(args)...);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), position, tmp.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + position, size_ - position, tmp.GetAddress() + position + 1);
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), position, tmp.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + position, size_ - position, tmp.GetAddress() + position + 1);
            }

            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(tmp);
        }

        ++size_;

        return begin() + position;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        return *Emplace(data_.GetAddress(), args...);
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    void PushBack(const T& value) {
    if (size_ >= Capacity()) {
            //Reserve(size_ == 0 ? 1 : size_ * 2);

            size_t tmp_size = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> tmp(tmp_size);

            new (tmp.GetAddress() + size_) T(value);

            MoveOrCopy(tmp);
        }
        else {
            new (data_.GetAddress() + size_) T(value);
        }
        ++size_;
//        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        if (size_ >= Capacity()) {
            size_t tmp_size = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> tmp(tmp_size);

            new (tmp.GetAddress() + size_) T(std::move(value));

            MoveOrCopy(tmp);
        }

        else {
            new (data_.GetAddress() + size_) T(std::move(value));
        }
        ++size_;
//        EmplaceBack(std::move(value));
    }

    void PopBack() noexcept {
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
    }

    iterator Erase(const_iterator pos) {
        int position = pos - begin();
        std::move(begin() + position + 1, end(), begin() + position);
        std::destroy_at(end() - 1);
        size_ -= 1;

        return begin() + position;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ <= data_.Capacity()) {
                if (rhs.size_ < size_) {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                    size_ = rhs.size_;
                }
                else {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                    size_ = rhs.size_;
                }
            }
            else {
                Vector tmp(rhs);
                Swap(tmp);
            }
        }

        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        Swap(rhs);

        return *this;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    bool Empty() {
        if (size_ > 0) {
            return false;
        }

        return true;
    }
private:
    RawMemory<T> data_;
    size_t size_ = 0;

    void MoveOrCopy(RawMemory<T>& tmp) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, tmp.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, tmp.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);

        data_.Swap(tmp);
    }

};