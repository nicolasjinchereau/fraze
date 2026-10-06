/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace fraze {

class intrusive_list_node
{
    template<class T>
    friend class intrusive_list;

    intrusive_list_node* _next{};
    intrusive_list_node* _prev{};

public:
    intrusive_list_node() noexcept = default;
    ~intrusive_list_node() {
        assert(!_next && !_prev);
    }

    intrusive_list_node(const intrusive_list_node&) = delete;
    intrusive_list_node(intrusive_list_node&&) = delete;
    intrusive_list_node& operator=(const intrusive_list_node&) = delete;
    intrusive_list_node& operator=(intrusive_list_node&&) = delete;
};

// A circular list which can contain externally allocated nodes.
template<class T>
class intrusive_list
{
    static_assert(std::is_base_of_v<intrusive_list_node, T>,
        "intrusive_list<T> requires T to derive from intrusive_list_node");

    intrusive_list_node _head;
    std::size_t _count = 0;

    void init_head() noexcept
    {
        _head._next = &_head;
        _head._prev = &_head;
    }

    void adopt(intrusive_list& that) noexcept
    {
        assert(empty());

        if(that.empty())
        {
            init_head();
            return;
        }

        _head._next = that._head._next;
        _head._prev = that._head._prev;
        _head._next->_prev = &_head;
        _head._prev->_next = &_head;
        _count = that._count;

        that.init_head();
        that._count = 0;
    }

    void link(intrusive_list_node* before, T* node) noexcept
    {
        assert(before);
        assert(node);

        intrusive_list_node* inserted = node;
        assert(!inserted->_prev && !inserted->_next);

        intrusive_list_node* after = before->_next;
        before->_next = inserted;
        inserted->_prev = before;
        inserted->_next = after;
        after->_prev = inserted;
        ++_count;
    }

    void unlink(intrusive_list_node* node) noexcept
    {
        assert(node);
        assert(node != &_head);
        assert(node->_prev && node->_next);

        node->_prev->_next = node->_next;
        node->_next->_prev = node->_prev;
        node->_next = nullptr;
        node->_prev = nullptr;
        --_count;
    }

public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;

    template<class U>
    class basic_iterator
    {
        friend class intrusive_list;

        template<class V>
        friend class basic_iterator;

        intrusive_list_node* _node{};

        explicit basic_iterator(intrusive_list_node* node) noexcept
            : _node(node){}

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::remove_const_t<U>;
        using difference_type = std::ptrdiff_t;
        using pointer = U*;
        using reference = U&;

        basic_iterator() noexcept = default;

        template<class V> requires std::is_same_v<U, const V>
        basic_iterator(const basic_iterator<V>& that) noexcept
            : _node(that._node)
        {}

        reference operator*() const noexcept {
            return *static_cast<U*>(_node);
        }

        pointer operator->() const noexcept {
            return static_cast<U*>(_node);
        }

        pointer get() const noexcept {
            return static_cast<U*>(_node);
        }

        basic_iterator& operator++() noexcept {
            _node = _node->_next;
            return *this;
        }

        basic_iterator operator++(int) noexcept {
            basic_iterator result = *this;
            _node = _node->_next;
            return result;
        }

        basic_iterator& operator--() noexcept {
            _node = _node->_prev;
            return *this;
        }

        basic_iterator operator--(int) noexcept {
            basic_iterator result = *this;
            _node = _node->_prev;
            return result;
        }

        template<class V>
        bool operator==(const basic_iterator<V>& that) const noexcept {
            return _node == that._node;
        }
    };

    using iterator = basic_iterator<T>;
    using const_iterator = basic_iterator<const T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    intrusive_list() noexcept {
        init_head();
    }

    // Unlinks all nodes without destroying them and clears _head's pointers to avoid the assertion in its destructor.
    ~intrusive_list()
    {
        clear();
        _head._prev = nullptr;
        _head._next = nullptr;
    }

    intrusive_list(const intrusive_list&) = delete;
    intrusive_list& operator=(const intrusive_list&) = delete;

    intrusive_list(intrusive_list&& that) noexcept
        : intrusive_list()
    {
        adopt(that);
    }

    intrusive_list& operator=(intrusive_list&& that) noexcept
    {
        if(this != &that)
        {
            clear();
            adopt(that);
        }

        return *this;
    }

    void push_front(T* node) noexcept {
        link(&_head, node);
    }

    void push_back(T* node) noexcept {
        link(_head._prev, node);
    }

    void insert(const_iterator where, T* node) noexcept {
        assert(where._node);
        link(where._node->_prev, node);
    }

    void pop_front() noexcept {
        assert(!empty());
        unlink(_head._next);
    }

    void pop_back() noexcept {
        assert(!empty());
        unlink(_head._prev);
    }

    iterator erase(const_iterator where) noexcept
    {
        assert(where._node);
        intrusive_list_node* after = where._node->_next;
        unlink(where._node);
        return iterator(after);
    }

    void remove(T* node) noexcept {
        unlink(node);
    }

    // For use after a node has been moved in memory by something like std::realloc.
    void relink_neighbors(T* node) noexcept
    {
        assert(node);

        intrusive_list_node* moved = node;
        assert(moved->_prev && moved->_next);

        moved->_prev->_next = moved;
        moved->_next->_prev = moved;
    }

    void clear() noexcept
    {
        while(!empty())
            pop_front();
    }

    bool contains(const T* node) const noexcept
    {
        for(const intrusive_list_node* n = _head._next; n != &_head; n = n->_next)
        {
            if(n == static_cast<const intrusive_list_node*>(node))
                return true;
        }

        return false;
    }

    reference front() noexcept {
        assert(!empty());
        return *static_cast<T*>(_head._next);
    }

    const_reference front() const noexcept {
        assert(!empty());
        return *static_cast<const T*>(_head._next);
    }

    reference back() noexcept {
        assert(!empty());
        return *static_cast<T*>(_head._prev);
    }

    const_reference back() const noexcept {
        assert(!empty());
        return *static_cast<const T*>(_head._prev);
    }

    size_type size() const noexcept {
        return _count;
    }

    bool empty() const noexcept {
        return _head._next == &_head;
    }

    iterator begin() noexcept {
        return iterator(_head._next);
    }

    const_iterator begin() const noexcept {
        return const_iterator(const_cast<intrusive_list_node*>(_head._next));
    }

    iterator end() noexcept {
        return iterator(&_head);
    }

    const_iterator end() const noexcept {
        return const_iterator(const_cast<intrusive_list_node*>(&_head));
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
};

} // fraze
