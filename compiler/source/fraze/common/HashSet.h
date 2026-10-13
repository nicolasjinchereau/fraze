/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <fraze/common/DynamicArray.h>

namespace fraze {

// A hash set that keeps its elements in one array and finds them by linear probing. Inserting or erasing an
// element invalidates every iterator, and an iterator holds no debug bookkeeping, so using one never takes a lock.
template<class T, class Hash = std::hash<T>>
class hash_set
{
    static_assert(std::is_nothrow_move_constructible_v<T>,
        "hash_set<T> requires T to be nothrow move constructible");

public:
    using value_type = T;
    using size_type = std::size_t;

    class const_iterator
    {
        friend hash_set;

        const hash_set* _set = nullptr;
        std::size_t _index = 0;

        const_iterator(const hash_set* set, std::size_t index) noexcept
            : _set(set), _index(index) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        const_iterator() noexcept = default;

        reference operator*() const noexcept {
            assert(_set && _index < _set->capacity() && _set->_occupied[_index]);
            return _set->_slots[_index];
        }

        pointer operator->() const noexcept {
            return &**this;
        }

        const_iterator& operator++() noexcept {
            assert(_set && _index < _set->capacity());
            _index = _set->next_occupied(_index + 1);
            return *this;
        }

        const_iterator operator++(int) noexcept {
            const_iterator result = *this;
            ++*this;
            return result;
        }

        bool operator==(const const_iterator& that) const noexcept {
            return _set == that._set && _index == that._index;
        }
    };

    using iterator = const_iterator;

    hash_set() noexcept = default;

    // Delegates to the default constructor, so the destructor runs if an insert throws.
    hash_set(const hash_set& that)
        : hash_set()
    {
        reserve(that.size());

        for(const T& value : that)
            insert(value);
    }

    hash_set(hash_set&& that) noexcept {
        swap(that);
    }

    ~hash_set() {
        clear();
        deallocate(_slots);
    }

    hash_set& operator=(const hash_set& that)
    {
        if(this != &that)
        {
            hash_set copy(that);
            swap(copy);
        }

        return *this;
    }

    hash_set& operator=(hash_set&& that) noexcept
    {
        if(this != &that)
        {
            hash_set moved(std::move(that));
            swap(moved);
        }

        return *this;
    }

    void swap(hash_set& that) noexcept {
        std::swap(_slots, that._slots);
        _occupied.swap(that._occupied);
        std::swap(_size, that._size);
    }

    const_iterator begin() const noexcept {
        return const_iterator(this, next_occupied(0));
    }

    const_iterator end() const noexcept {
        return const_iterator(this, capacity());
    }

    bool empty() const noexcept {
        return _size == 0;
    }

    std::size_t size() const noexcept {
        return _size;
    }

    // The number of slots, which is zero or a power of two. The set grows before every slot is in use.
    std::size_t capacity() const noexcept {
        return _occupied.size();
    }

    std::pair<iterator, bool> insert(const T& value) {
        return insert_value(value);
    }

    std::pair<iterator, bool> insert(T&& value) {
        return insert_value(std::move(value));
    }

    std::size_t erase(const T& value)
    {
        if(empty())
            return 0;

        std::size_t hole = find_slot(value);

        if(!_occupied[hole])
            return 0;

        std::destroy_at(_slots + hole);

        // A lookup stops at the first empty slot, so no element may be left with one between its ideal slot and
        // itself. Each element after the hole that probing would have put in the hole moves into it.
        for(std::size_t index = next_index(hole); _occupied[index]; index = next_index(index))
        {
            std::size_t ideal = ideal_index(_slots[index]);

            if(probe_distance(ideal, hole) < probe_distance(ideal, index))
            {
                std::construct_at(_slots + hole, std::move(_slots[index]));
                std::destroy_at(_slots + index);
                hole = index;
            }
        }

        _occupied[hole] = false;
        --_size;

        return 1;
    }

    const_iterator find(const T& value) const
    {
        if(empty())
            return end();

        std::size_t index = find_slot(value);
        return _occupied[index] ? const_iterator(this, index) : end();
    }

    bool contains(const T& value) const {
        return find(value) != end();
    }

    void clear() noexcept
    {
        for(std::size_t index = 0; index != capacity(); ++index)
        {
            if(_occupied[index])
            {
                std::destroy_at(_slots + index);
                _occupied[index] = false;
            }
        }

        _size = 0;
    }

    void reserve(std::size_t count)
    {
        std::size_t requiredCapacity = min_capacity;

        while(max_load(requiredCapacity) < count)
            requiredCapacity *= 2;

        if(requiredCapacity > capacity())
            rehash(requiredCapacity);
    }

private:
    static constexpr std::size_t min_capacity = 8;

    // Linear probing slows down sharply as the array fills, so a quarter of it stays empty.
    static constexpr std::size_t max_load(std::size_t capacity) noexcept {
        return capacity - capacity / 4;
    }

    static T* allocate(std::size_t count) {
        return static_cast<T*>(::operator new(count * sizeof(T), std::align_val_t(alignof(T))));
    }

    static void deallocate(T* p) noexcept {
        ::operator delete(p, std::align_val_t(alignof(T)));
    }

    // Where probing for 'value' starts. The multiplication spreads the hash's bits upward, and the shift keeps
    // as many of the top ones as the capacity needs, so a hash that only varies in a few bits still spreads out.
    std::size_t ideal_index(const T& value) const noexcept
    {
        assert(capacity() >= min_capacity);

        std::uint64_t spread = static_cast<std::uint64_t>(Hash()(value)) * 0x9E3779B97F4A7C15ull;
        return static_cast<std::size_t>(spread >> std::countl_zero(static_cast<std::uint64_t>(capacity() - 1)));
    }

    std::size_t next_index(std::size_t index) const noexcept {
        return (index + 1) & (capacity() - 1);
    }

    // The number of steps probing takes from one slot to another.
    std::size_t probe_distance(std::size_t from, std::size_t to) const noexcept {
        return (to - from) & (capacity() - 1);
    }

    std::size_t next_occupied(std::size_t index) const noexcept
    {
        while(index != capacity() && !_occupied[index])
            ++index;

        return index;
    }

    // The slot that holds 'value', or the empty one it belongs in.
    std::size_t find_slot(const T& value) const noexcept
    {
        std::size_t index = ideal_index(value);

        while(_occupied[index] && !(_slots[index] == value))
            index = next_index(index);

        return index;
    }

    template<class U>
    std::size_t emplace_at(std::size_t index, U&& value)
    {
        assert(!_occupied[index]);

        std::construct_at(_slots + index, std::forward<U>(value));
        _occupied[index] = true;
        ++_size;

        return index;
    }

    template<class U>
    std::pair<iterator, bool> insert_value(U&& value)
    {
        if(capacity() != 0)
        {
            std::size_t index = find_slot(value);

            if(_occupied[index])
                return { const_iterator(this, index), false };

            if(_size < max_load(capacity()))
                return { const_iterator(this, emplace_at(index, std::forward<U>(value))), true };
        }

        // 'value' wasn't found, so it isn't an element and stays where it is while the elements move.
        rehash(capacity() != 0 ? capacity() * 2 : min_capacity);

        return { const_iterator(this, emplace_at(find_slot(value), std::forward<U>(value))), true };
    }

    void rehash(std::size_t newCapacity)
    {
        assert(std::has_single_bit(newCapacity));
        assert(_size <= max_load(newCapacity));

        hash_set rehashed;
        rehashed._slots = allocate(newCapacity);
        rehashed._occupied = dynamic_array<std::uint8_t>(newCapacity, false);

        for(std::size_t index = 0; index != capacity(); ++index)
        {
            if(_occupied[index])
                rehashed.emplace_at(rehashed.find_slot(_slots[index]), std::move(_slots[index]));
        }

        // 'rehashed' takes the old array, and destroys the moved-from elements left in it.
        swap(rehashed);
    }

    T* _slots = nullptr;
    dynamic_array<std::uint8_t> _occupied;
    std::size_t _size = 0;
};

} // fraze
