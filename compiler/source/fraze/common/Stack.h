/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Platform.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <new>
#include <stack>
#include <type_traits>
#include <utility>
#include <vector>

namespace fraze {

template<typename T>
class stack
{
public:
    stack()
        : _data(static_cast<T*>(::operator new[](0, std::align_val_t{64})))
        , _top(_data - 1)
        , _cap(_data)
    {}

    explicit stack(std::size_t capacity)
        : _data(static_cast<T*>(::operator new[](capacity * sizeof(T), std::align_val_t{64})))
        , _top(_data - 1)
        , _cap(_data + capacity)
    {}

    ~stack() {
        ::operator delete[](_data, std::align_val_t{64});
    }

    stack(const stack& other)
        : _data(static_cast<T*>(::operator new[](other.capacity() * sizeof(T), std::align_val_t{64})))
        , _cap(_data + other.capacity())
    {
        size_t size = other.size() * sizeof(T);
        memcpy(_data, other._data, size);
        _top = _data + other.size() - 1;
    }

    stack& operator=(const stack& other)
    {
        stack(other).swap(*this);
    }

    stack(stack&& other)
    {
        other.swap(*this);
        other.clear();
    }

    stack& operator=(stack&& other)
    {
        other.swap(*this);
        other.clear();
        return *this;
    }

    void swap(stack& other) noexcept
    {
        std::swap(_data, other._data);
        std::swap(_top, other._top);
        std::swap(_cap, other._cap);
    }

    bool operator==(const stack& other) const
    {
        if(size() != other.size())
            return false;
        
        for(size_t i = 0; i != size(); ++i)
        {
            if(this->operator[](i) != other[i])
                return false;
        }

        return true;
    }

    bool operator!=(const stack& other) const
    {
        return !(*this == other);
    }

    FRAZE_INLINE std::size_t capacity() const noexcept {
        return std::size_t(_cap - _data);
    }

    FRAZE_INLINE std::size_t size() const noexcept {
        return std::size_t(_top - _data + 1);
    }

    FRAZE_INLINE bool empty() const noexcept {
        return _top < _data;
    }

    FRAZE_INLINE void clear() noexcept {
        _top = _data - 1;
    }

    FRAZE_INLINE void reserve(std::size_t new_cap) {
        if (capacity() >= new_cap) return;
        ::operator delete[](_data, std::align_val_t{64});
        _data = static_cast<T*>(::operator new[](new_cap * sizeof(T), std::align_val_t{64}));
        _top  = _data - 1;
        _cap  = _data + new_cap;
    }

    FRAZE_INLINE void resize(std::size_t new_size) {
        assert(new_size <= capacity() && "resize exceeds capacity");
        _top = (_data - 1) + new_size;
    }

    FRAZE_INLINE T* data() noexcept {
        return _data;
    }

    FRAZE_INLINE const T* data() const noexcept {
        return _data;
    }

    FRAZE_INLINE T& operator[](std::size_t i) {
        assert(i < size());
        return _data[i];
    }

    FRAZE_INLINE const T& operator[](std::size_t i) const {
        assert(i < size());
        return _data[i];
    }

    FRAZE_INLINE T& top() {
        assert(!empty());
        return *_top;
    }

    FRAZE_INLINE const T& top() const {
        assert(!empty());
        return *_top;
    }

    FRAZE_INLINE T*& top_ptr() {
        assert(capacity() != 0);
        return _top;
    }

    FRAZE_INLINE T* begin() {
        return _data;
    }

    FRAZE_INLINE const T* begin() const {
        return _data;
    }

    FRAZE_INLINE T* end() {
        return _top + 1;
    }

    FRAZE_INLINE const T* end() const {
        return _top + 1;
    }

    FRAZE_INLINE void push(const T& val) {
        assert(size() < capacity() && "stack overflow");
        *(++_top) = val;
    }

    FRAZE_INLINE void push(T&& val) {
        assert(size() < capacity() && "stack overflow");
        *(++_top) = std::move(val);
    }

    FRAZE_INLINE void push(const T* RESTRICT src, std::size_t count)
    {
        assert(size() + count <= capacity() && "stack overflow");
        
        T* dest = _top + 1;
        
        for (std::size_t i = 0; i < count; ++i)
            dest[i] = src[i];

        _top = dest + (count - 1);
    }

    FRAZE_INLINE T pull() {
        assert(!empty() && "stack underflow");
        T val = *_top;
        --_top;
        return val;
    }

    FRAZE_INLINE void pop() {
        assert(!empty() && "stack underflow");
        --_top;
    }

    FRAZE_INLINE void grow(std::size_t count) {
        assert(size() + count <= capacity() && "stack overflow");
        _top += count;
    }

    FRAZE_INLINE void shrink(std::size_t count) {
        assert(count <= size() && "stack underflow");
        _top -= count;
    }
private:
    T* RESTRICT _data;   // cache-line aligned base of storage
    T* RESTRICT _top;    // points at last element, or (_data - 1) when empty
    T* RESTRICT _cap;    // one past end of storage
};


template<class T>
struct stack_facade
{
    T*& RESTRICT _top;

    FRAZE_INLINE stack_facade(/* T* data, */ T*& top)
        : _top(top){}

    FRAZE_INLINE T& top() {
        return *_top;
    }

    FRAZE_INLINE const T& top() const {
        return *_top;
    }

    FRAZE_INLINE T* top_ptr() {
        return _top;
    }

    FRAZE_INLINE const T* top_ptr() const {
        return _top;
    }

    FRAZE_INLINE T* end() {
        return _top + 1;
    }

    FRAZE_INLINE const T* end() const {
        return _top + 1;
    }

    FRAZE_INLINE void push(const T& val) {
        *(++_top) = val;
    }

    FRAZE_INLINE void push(T&& val) {
        *(++_top) = std::move(val);
    }

    FRAZE_INLINE void push(const T* RESTRICT src, std::size_t count) noexcept
    {
        T* dest = _top + 1;

        for (std::size_t i = 0; i < count; ++i)
            dest[i] = src[i];

        _top = dest + (count - 1);
    }

    FRAZE_INLINE T pull() {
        T val = *_top;
        --_top;
        return val;
    }

    FRAZE_INLINE void pop() noexcept {
        --_top;
    }

    FRAZE_INLINE void grow(std::size_t count) noexcept {
        _top += count;
    }

    FRAZE_INLINE void shrink(std::size_t count) noexcept {
        _top -= count;
    }
};

} // fraze
