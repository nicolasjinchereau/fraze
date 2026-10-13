/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <algorithm>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace fraze {

// A std::vector stand-in whose iterators are a pointer and nothing else, so creating or destroying one never
// takes a lock, which a Debug build's std::vector iterators do.
template<class T>
class vector
{
public:
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    class const_iterator
    {
        friend vector;
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        const_iterator() noexcept = default;

        reference operator*() const {
            assert(_ptr);
            return *_ptr;
        }

        pointer operator->() const {
            return _ptr;
        }

        const_iterator& operator++() {
            ++_ptr;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++_ptr;
            return tmp;
        }

        const_iterator& operator--() {
            --_ptr;
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --_ptr;
            return tmp;
        }

        const_iterator& operator+=(difference_type off) {
            _ptr += off;
            return *this;
        }

        const_iterator operator+(difference_type off) const {
            return const_iterator(_ptr + off);
        }

        friend const_iterator operator+(difference_type off, const const_iterator& it) {
            return it + off;
        }

        const_iterator& operator-=(difference_type off) {
            _ptr -= off;
            return *this;
        }

        const_iterator operator-(difference_type off) const {
            return const_iterator(_ptr - off);
        }

        difference_type operator-(const const_iterator& that) const {
            return _ptr - that._ptr;
        }

        reference operator[](difference_type off) const {
            return _ptr[off];
        }

        bool operator==(const const_iterator& that) const {
            return _ptr == that._ptr;
        }

        std::strong_ordering operator<=>(const const_iterator& that) const {
            return _ptr <=> that._ptr;
        }

    protected:
        T* _ptr = nullptr;

        explicit const_iterator(T* ptr) noexcept
            : _ptr(ptr) {}
    };

    class iterator : public const_iterator
    {
        friend vector;
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        iterator() noexcept = default;

        reference operator*() const {
            assert(this->_ptr);
            return *this->_ptr;
        }

        pointer operator->() const {
            return this->_ptr;
        }

        iterator& operator++() {
            ++this->_ptr;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++this->_ptr;
            return tmp;
        }

        iterator& operator--() {
            --this->_ptr;
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --this->_ptr;
            return tmp;
        }

        iterator& operator+=(difference_type off) {
            this->_ptr += off;
            return *this;
        }

        iterator operator+(difference_type off) const {
            return iterator(this->_ptr + off);
        }

        friend iterator operator+(difference_type off, const iterator& it) {
            return it + off;
        }

        iterator& operator-=(difference_type off) {
            this->_ptr -= off;
            return *this;
        }

        // the distance between two iterators, which the overload below would otherwise hide
        using const_iterator::operator-;

        iterator operator-(difference_type off) const {
            return iterator(this->_ptr - off);
        }

        reference operator[](difference_type off) const {
            return this->_ptr[off];
        }

    private:
        explicit iterator(T* ptr) noexcept
            : const_iterator(ptr) {}
    };

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    vector() noexcept = default;

    // Each constructor that allocates delegates to the default one, so the destructor runs if it throws.
    explicit vector(std::size_t count)
        : vector()
    {
        reserve(count);
        _last = std::uninitialized_value_construct_n(_first, count);
    }

    vector(std::size_t count, const T& value)
        : vector()
    {
        reserve(count);
        _last = std::uninitialized_fill_n(_first, count, value);
    }

    template<std::forward_iterator Iter>
    vector(Iter first, Iter last)
        : vector()
    {
        reserve(static_cast<std::size_t>(std::distance(first, last)));
        _last = std::uninitialized_copy(first, last, _first);
    }

    vector(std::initializer_list<T> init)
        : vector(init.begin(), init.end())
    {
    }

    vector(const vector& that)
        : vector(that.begin(), that.end())
    {
    }

    vector(vector&& that) noexcept {
        move_from(that);
    }

    ~vector() {
        cleanup();
    }

    vector& operator=(const vector& that)
    {
        if(this != &that)
            assign(that.begin(), that.end());

        return *this;
    }

    vector& operator=(vector&& that) noexcept
    {
        if(this != &that)
        {
            cleanup();
            move_from(that);
        }

        return *this;
    }

    vector& operator=(std::initializer_list<T> init) {
        assign(init.begin(), init.end());
        return *this;
    }

    template<std::forward_iterator Iter>
    void assign(Iter first, Iter last)
    {
        clear();
        reserve(static_cast<std::size_t>(std::distance(first, last)));
        _last = std::uninitialized_copy(first, last, _first);
    }

    void assign(std::size_t count, const T& value)
    {
        clear();
        reserve(count);
        _last = std::uninitialized_fill_n(_first, count, value);
    }

    void assign(std::initializer_list<T> init) {
        assign(init.begin(), init.end());
    }

    template<class... Args>
    T& emplace_back(Args&&... args)
    {
        if(_last == _end)
        {
            // built before the storage moves, because an argument may refer to an element in it
            T value(std::forward<Args>(args)...);
            reallocate(grown_capacity(size() + 1));
            return append(std::move(value));
        }

        return append(std::forward<Args>(args)...);
    }

    void push_back(const T& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    void pop_back() {
        assert(!empty());
        std::destroy_at(--_last);
    }

    void clear() noexcept {
        std::destroy(_first, _last);
        _last = _first;
    }

    void swap(vector& that) noexcept {
        std::swap(_first, that._first);
        std::swap(_last, that._last);
        std::swap(_end, that._end);
    }

    void resize(std::size_t newSize)
    {
        if(newSize > size())
        {
            if(newSize > capacity())
                reallocate(grown_capacity(newSize));

            _last = std::uninitialized_value_construct_n(_last, newSize - size());
        }
        else
        {
            truncate(newSize);
        }
    }

    void resize(std::size_t newSize, const T& value)
    {
        if(newSize > capacity())
        {
            // copied before the storage moves, because 'value' may refer to an element in it
            T copy(value);
            reallocate(grown_capacity(newSize));
            _last = std::uninitialized_fill_n(_last, newSize - size(), copy);
        }
        else if(newSize > size())
        {
            _last = std::uninitialized_fill_n(_last, newSize - size(), value);
        }
        else
        {
            truncate(newSize);
        }
    }

    void reserve(std::size_t newCapacity)
    {
        assert(newCapacity <= max_size());

        if(newCapacity > capacity())
            reallocate(newCapacity);
    }

    void shrink_to_fit()
    {
        if(empty())
            cleanup();
        else if(_last != _end)
            reallocate(size());
    }

    iterator begin() noexcept {
        return iterator(_first);
    }

    const_iterator begin() const noexcept {
        return const_iterator(_first);
    }

    iterator end() noexcept {
        return iterator(_last);
    }

    const_iterator end() const noexcept {
        return const_iterator(_last);
    }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    template<class U> requires std::constructible_from<T, U>
    iterator insert(const_iterator where, U&& value)
    {
        assert(where._ptr >= _first);
        assert(where._ptr <= _last);

        std::ptrdiff_t offset = where._ptr - _first;

        // built before anything moves, because 'value' may refer to an element of this vector
        T inserted(std::forward<U>(value));

        if(_last == _end)
            reallocate(grown_capacity(size() + 1));

        T* position = _first + offset;

        if(position == _last)
        {
            append(std::move(inserted));
        }
        else
        {
            append(std::move(_last[-1]));
            std::move_backward(position, _last - 2, _last - 1);
            *position = std::move(inserted);
        }

        return iterator(position);
    }

    template<std::forward_iterator Iter>
    iterator insert(const_iterator where, Iter first, Iter last)
    {
        assert(where._ptr >= _first);
        assert(where._ptr <= _last);

        std::ptrdiff_t offset = where._ptr - _first;
        std::size_t oldSize = size();
        std::size_t newSize = oldSize + static_cast<std::size_t>(std::distance(first, last));

        if(newSize > capacity())
            reallocate(grown_capacity(newSize));

        _last = std::uninitialized_copy(first, last, _last);
        std::rotate(_first + offset, _first + oldSize, _last);

        return iterator(_first + offset);
    }

    iterator insert(const_iterator where, std::initializer_list<T> init) {
        return insert(where, init.begin(), init.end());
    }

    iterator erase(const_iterator where)
    {
        assert(where._ptr >= _first);
        assert(where._ptr < _last);

        std::move(where._ptr + 1, _last, where._ptr);
        std::destroy_at(--_last);

        return iterator(where._ptr);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        assert(first._ptr <= last._ptr);
        assert(first._ptr >= _first);
        assert(last._ptr <= _last);

        if(first._ptr != last._ptr)
        {
            T* newLast = std::move(last._ptr, _last, first._ptr);
            std::destroy(newLast, _last);
            _last = newLast;
        }

        return iterator(first._ptr);
    }

    T* data() noexcept {
        return _first;
    }

    const T* data() const noexcept {
        return _first;
    }

    bool empty() const noexcept {
        return _first == _last;
    }

    std::size_t size() const noexcept {
        return static_cast<std::size_t>(_last - _first);
    }

    std::size_t capacity() const noexcept {
        return static_cast<std::size_t>(_end - _first);
    }

    std::size_t max_size() const noexcept {
        return static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(T);
    }

    T& at(std::size_t pos) {
        assert(pos < size());
        return _first[pos];
    }

    const T& at(std::size_t pos) const {
        assert(pos < size());
        return _first[pos];
    }

    T& operator[](std::size_t pos) {
        return at(pos);
    }

    const T& operator[](std::size_t pos) const {
        return at(pos);
    }

    T& front() {
        assert(!empty());
        return *_first;
    }

    const T& front() const {
        assert(!empty());
        return *_first;
    }

    T& back() {
        assert(!empty());
        return _last[-1];
    }

    const T& back() const {
        assert(!empty());
        return _last[-1];
    }

private:
    static T* allocate(std::size_t count) {
        return static_cast<T*>(::operator new(count * sizeof(T), std::align_val_t(alignof(T))));
    }

    static void deallocate(T* p) noexcept {
        ::operator delete(p, std::align_val_t(alignof(T)));
    }

    struct deleter {
        void operator()(T* p) const noexcept {
            deallocate(p);
        }
    };

    std::size_t grown_capacity(std::size_t requiredCapacity) const noexcept {
        std::size_t currentCapacity = capacity();
        return std::max(currentCapacity + currentCapacity / 2, requiredCapacity);
    }

    // '_last' moves only once the element exists, so a constructor that throws leaves the vector as it was.
    template<class... Args>
    T& append(Args&&... args)
    {
        assert(_last != _end);
        T* element = std::construct_at(_last, std::forward<Args>(args)...);
        ++_last;
        return *element;
    }

    void truncate(std::size_t newSize) noexcept
    {
        T* newLast = _first + newSize;
        std::destroy(newLast, _last);
        _last = newLast;
    }

    void cleanup() noexcept
    {
        std::destroy(_first, _last);
        deallocate(_first);

        _first = nullptr;
        _last = nullptr;
        _end = nullptr;
    }

    void reallocate(std::size_t newCapacity)
    {
        assert(newCapacity != 0);
        assert(newCapacity >= size());

        std::unique_ptr<T, deleter> storage(allocate(newCapacity));
        std::size_t count = size();

        if constexpr(std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
            std::uninitialized_move(_first, _last, storage.get());
        else
            std::uninitialized_copy(_first, _last, storage.get());

        std::destroy(_first, _last);
        deallocate(_first);

        _first = storage.release();
        _last = _first + count;
        _end = _first + newCapacity;
    }

    void move_from(vector& that) noexcept
    {
        _first = that._first;
        _last = that._last;
        _end = that._end;

        that._first = nullptr;
        that._last = nullptr;
        that._end = nullptr;
    }

    T* _first = nullptr;
    T* _last = nullptr;
    T* _end = nullptr;
};

} // fraze
