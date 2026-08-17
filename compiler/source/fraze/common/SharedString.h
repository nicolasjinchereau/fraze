/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cassert>
#include <format>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <fraze/common/Extensions.h>

namespace fraze {

class SharedStringPool
{
public:
    static SharedStringPool& Get() {
        static SharedStringPool instance;
        return instance;
    }

    std::string_view Acquire(std::string_view key)
    {
        std::unique_lock lk(_mtx);
        auto it = _set.find(key);
        if (it != _set.end()) {
            return *it;
        }
        auto [ins, ok] = _set.emplace(key);
        (void)ok;
        return *ins;
    }

    std::string_view Acquire(std::string&& key)
    {
        std::unique_lock lk(_mtx);
        auto it = _set.find(key);
        if (it != _set.end()) {
            return *it;
        }
        auto [ins, ok] = _set.emplace(std::move(key));
        (void)ok;
        return *ins;
    }

    const std::string& GetString(std::string_view key) const {
        std::shared_lock lk(_mtx);
        auto it = _set.find(key);
        assert(it != _set.end());
        return *it;
    }

private:
    string_view_set _set;
    mutable std::shared_mutex _mtx;
};

class shared_string
{
    std::string_view _view{};
public:
    shared_string() noexcept = default;

    explicit shared_string(std::string_view sv) {
        _view = SharedStringPool::Get().Acquire(sv);
    }

    explicit shared_string(const char* s)
        : shared_string(std::string_view{s ? s : ""})
    {
    }

    explicit shared_string(std::string&& s) {
        _view = SharedStringPool::Get().Acquire(std::move(s));
    }

    shared_string(const shared_string& other)
        : _view(other._view)
    {
    }

    shared_string(shared_string&& other) noexcept
        : _view(other._view)
    {
        other._view = {};
    }

    shared_string& operator=(const shared_string& other)
    {
        if (this != &other)
        {
            _view = other._view;
        }
        return *this;
    }

    shared_string& operator=(shared_string&& other) noexcept
    {
        if (this != &other)
        {
            _view = other._view;
            other._view = {};
        }
        return *this;
    }

    ~shared_string() = default;

    operator std::string_view() const {
        return _view;
    }

    std::string_view view() const {
        return _view;
    }

    const char* c_str() const noexcept {
        return _view.data();
    }

    const std::string& str() const {
        return SharedStringPool::Get().GetString(_view);
    }

    const char* data() const noexcept {
        return _view.data();
    }

    std::size_t size() const noexcept {
        return _view.size();
    }

    bool empty() const noexcept {
        return _view.empty();
    }

    explicit operator bool() const noexcept {
        return !_view.empty();
    }

    friend bool operator==(StringViewSource auto const& lhs, StringViewSource auto const& rhs) noexcept {
        return std::string_view(lhs) == std::string_view(rhs);
    }

    friend bool operator!=(StringViewSource auto const& lhs, StringViewSource auto const& rhs) noexcept {
        return std::string_view(lhs) != std::string_view(rhs);
    }

    friend bool operator<(StringViewSource auto const& lhs, StringViewSource auto const& rhs) noexcept {
        return std::string_view(lhs) < std::string_view(rhs);
    }

    void swap(shared_string& other) noexcept {
        std::swap(_view, other._view);
    }

    const char& operator[](const size_t pos) const noexcept {
        return _view[pos];
    }

    size_t find(std::string_view sv, size_t pos = 0) const noexcept {
        return _view.find(sv, pos);
    }

    size_t find(char c, size_t pos = 0) const noexcept {
        return _view.find(c, pos);
    }

    size_t rfind(std::string_view sv, size_t pos = std::string_view::npos) const noexcept {
        return _view.rfind(sv, pos);
    }

    size_t rfind(char c, size_t pos = std::string_view::npos) const noexcept {
        return _view.rfind(c, pos);
    }

    size_t find_first_of(std::string_view sv, size_t pos = 0) const noexcept {
        return _view.find_first_of(sv, pos);
    }

    size_t find_first_of(char c, size_t pos = 0) const noexcept {
        return _view.find_first_of(c, pos);
    }

    size_t find_last_of(std::string_view sv, size_t pos = std::string_view::npos) const noexcept {
        return _view.find_last_of(sv, pos);
    }

    size_t find_last_of(char c, size_t pos = std::string_view::npos) const noexcept {
        return _view.find_last_of(c, pos);
    }

    size_t find_first_not_of(std::string_view sv, size_t pos = 0) const noexcept {
        return _view.find_first_not_of(sv, pos);
    }

    size_t find_first_not_of(char c, size_t pos = 0) const noexcept {
        return _view.find_first_not_of(c, pos);
    }

    size_t find_last_not_of(std::string_view sv, size_t pos = std::string_view::npos) const noexcept {
        return _view.find_last_not_of(sv, pos);
    }

    size_t find_last_not_of(char c, size_t pos = std::string_view::npos) const noexcept {
        return _view.find_last_not_of(c, pos);
    }

    bool starts_with(std::string_view sv) const noexcept {
        return _view.starts_with(sv);
    }

    bool starts_with(char c) const noexcept {
        return _view.starts_with(c);
    }

    bool ends_with(std::string_view sv) const noexcept {
        return _view.ends_with(sv);
    }

    bool ends_with(char c) const noexcept {
        return _view.ends_with(c);
    }

    friend std::ostream& operator<<(std::ostream& os, const shared_string& str) {
        return os << str._view;
    }

    friend std::string operator+(const shared_string& lhs, const std::string_view& rhs) {
        std::string ret;
        ret.reserve(lhs._view.size() + rhs.size());
        ret += lhs._view;
        ret += rhs;
        return ret;
    }

    friend std::string operator+(const std::string_view& lhs, const shared_string& rhs) {
        std::string ret;
        ret.reserve(lhs.size() + rhs._view.size());
        ret += lhs;
        ret += rhs._view;
        return ret;
    }

    friend std::string operator+(const shared_string& lhs, const shared_string& rhs) {
        std::string ret;
        ret.reserve(lhs._view.size() + rhs._view.size());
        ret += lhs._view;
        ret += rhs._view;
        return ret;
    }
};

} // fraze

namespace std
{

template<>
struct formatter<fraze::shared_string, char> : formatter<std::string_view, char>
{
    template<class FormatContext>
    auto format(const fraze::shared_string& s, FormatContext& ctx) const {
        return formatter<std::string_view, char>::format(s.view(), ctx);
    }
};

} // std
