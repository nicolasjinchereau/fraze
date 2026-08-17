/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

namespace fraze {

template <class T, std::size_t alignment = alignof(T)>
class dynamic_array
{
    static_assert(std::is_trivially_copyable_v<T>,
        "dynamic_array<T> requires T to be trivially copyable");

public:
    using value_type = T;
    using size_type = std::size_t;

    dynamic_array() noexcept = default;

    explicit dynamic_array(std::size_t size)
    {
        allocate(size);
    }

    dynamic_array(const dynamic_array& other)
    {
        allocate(other._size);

        if(_data)
        {
            std::memcpy(_data, other._data, _size * sizeof(T));
        }
    }

    dynamic_array(dynamic_array&& other) noexcept
        : _data(other._data)
        , _size(other._size)
    {
        other._data = nullptr;
        other._size = 0;
    }

    dynamic_array& operator=(const dynamic_array& other)
    {
        if(this == &other)
            return *this;

        dynamic_array tmp(other);
        swap(tmp);
        return *this;
    }

    dynamic_array& operator=(dynamic_array&& other) noexcept
    {
        if(this == &other)
            return *this;

        deallocate();

        _data = other._data;
        _size = other._size;

        other._data = nullptr;
        other._size = 0;

        return *this;
    }

    ~dynamic_array() {
        deallocate();
    }

    void swap(dynamic_array& other) noexcept {
        std::swap(_data, other._data);
        std::swap(_size, other._size);
    }

    T* data() noexcept {
        return _data;
    }

    const T* data() const noexcept {
        return _data;
    }

    std::size_t size() const noexcept {
        return _size;
    }
    
    bool empty() const noexcept {
        return _size == 0;
    }

    T* begin() noexcept {
        return _data;
    }

    const T* begin() const noexcept {
        return _data;
    }

    T* end() noexcept {
        return _data + _size;
    }

    const T* end() const noexcept {
        return _data + _size;
    }

    const T* cbegin() const noexcept {
        return _data;
    }

    const T* cend() const noexcept {
        return _data + _size;
    }

    T& operator[](std::size_t i) noexcept {
        return _data[i];
    }

    const T& operator[](std::size_t i) const noexcept {
        return _data[i];
    }

private:
    void allocate(size_t size)
    {
        assert(_data == nullptr);
        assert(_size == 0);

        if(size != 0)
        {
            _data = static_cast<T*>(::operator new(size * sizeof(T), std::align_val_t(alignment)));
            _size = size;
        }
    }

    void deallocate() noexcept
    {
        if(_data)
        {
            ::operator delete(_data, std::align_val_t(alignment));
            _data = nullptr;
            _size = 0;
        }
    }

    T* _data = nullptr;
    std::size_t _size = 0;
};

} // fraze
