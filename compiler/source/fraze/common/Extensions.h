/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace fraze {

struct StringViewHash
{
    using is_transparent = void;
    
    template<typename T>
    size_t operator()(const T& key) const {
        return std::hash<std::string_view>{}(std::string_view(key));
    }
};

struct StringViewEqual
{
    using is_transparent = void;
    
    template<typename LHS, typename RHS>
    bool operator()(const LHS& lhs, const RHS& rhs) const {
        return std::string_view(lhs) == std::string_view(rhs);
    }
};

template<class T>
using string_view_map = std::unordered_map<std::string, T, StringViewHash, StringViewEqual>;

using string_view_set = std::unordered_set<std::string, StringViewHash, StringViewEqual>;

template<typename T, typename... Ts>
constexpr bool is_any_of_v = (std::is_same_v<T, Ts> || ...);

template<size_t i, class... Args>
using get_type = std::tuple_element_t<i, std::tuple<Args...>>;

template<typename T>
struct is_c_string
    : std::bool_constant<
        (std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>) ||
        (std::is_array_v<T> && std::is_same_v<std::remove_cv_t<std::remove_extent_t<T>>, char>)
    >{};

template<typename T>
inline constexpr bool is_c_string_v = is_c_string<T>::value;

template<class T>
concept StringViewSource = std::is_constructible_v<std::string_view, T>;

template<class>
constexpr bool is_span_v = false;

template<class T, size_t Extent>
constexpr bool is_span_v<std::span<T, Extent>> = true;

inline std::string_view trim_left(std::string_view sv) {
    size_t start = sv.find_first_not_of(" \t\n\r\f\v");
    return (start == std::string_view::npos) ? std::string_view{} : sv.substr(start);
}

inline std::string_view trim_right(std::string_view sv) {
    size_t end = sv.find_last_not_of(" \t\n\r\f\v");
    return (end == std::string_view::npos) ? std::string_view{} : sv.substr(0, end + 1);
}

inline std::string_view trim(std::string_view sv) {
    return trim_left(trim_right(sv));
}

template<class From, class To>
using copy_const_t = std::conditional_t<
    std::is_const_v<From>,
    std::conditional_t<
        std::is_pointer_v<To>,
        std::add_pointer_t<std::add_const_t<std::remove_pointer_t<To>>>,
        std::add_const_t<To>
    >,
    To
>;

template<std::ranges::input_range R>
class CountableView : public std::ranges::view_interface<CountableView<R>>
{
    R base;

public:
    explicit CountableView(R base) : base(std::move(base)) {}

    auto begin() {
        return std::ranges::begin(base);
    }

    auto end() {
        return std::ranges::end(base);
    }

    std::size_t count() {
        auto ct = std::ranges::distance(base);
        return static_cast<size_t>(ct);
    }
};

struct CountableAdaptor
{
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        return CountableView{ std::forward<R>(r) };
    }

    // enable pipe support
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const CountableAdaptor& self) {
        return self(std::forward<R>(r));
    }
};

inline constexpr CountableAdaptor countable{};


template <std::ranges::input_range R1, std::ranges::input_range R2>
class concat_view : public std::ranges::view_interface<concat_view<R1, R2>>
{
    R1 r1_;
    R2 r2_;

    class iterator
    {
        using I1 = std::ranges::iterator_t<R1>;
        using S1 = std::ranges::sentinel_t<R1>;
        using I2 = std::ranges::iterator_t<R2>;
        using S2 = std::ranges::sentinel_t<R2>;

        I1 it1_;
        S1 end1_;
        I2 it2_;
        S2 end2_;
        bool in_first_;

    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::common_type_t<
            std::ranges::range_value_t<R1>,
            std::ranges::range_value_t<R2>>;

        iterator(I1 it1, S1 end1, I2 it2, S2 end2, bool in_first)
            : it1_(it1), end1_(end1), it2_(it2), end2_(end2), in_first_(in_first)
        {
            if(in_first_ && it1_ == end1_) in_first_ = false;
        }

        decltype(auto) operator*() const
        {
            return in_first_ ? *it1_ : *it2_;
        }

        iterator& operator++()
        {
            if(in_first_)
            {
                ++it1_;
                if(it1_ == end1_) in_first_ = false;
            }
            else
            {
                ++it2_;
            }
            return *this;
        }

        void operator++(int) { ++*this; }

        friend bool operator==(const iterator& it, std::default_sentinel_t)
        {
            return !it.in_first_ && it.it2_ == it.end2_;
        }
    };

public:
    concat_view() = default;
    concat_view(R1 r1, R2 r2) : r1_(std::move(r1)), r2_(std::move(r2)) {}

    auto begin()
    {
        return iterator(
            std::ranges::begin(r1_), std::ranges::end(r1_),
            std::ranges::begin(r2_), std::ranges::end(r2_),
            true);
    }

    auto end() { return std::default_sentinel; }
};

template <class R1, class R2>
concat_view(R1, R2) -> concat_view<std::views::all_t<R1>, std::views::all_t<R2>>;

struct ConcatFunc
{
    template <std::ranges::input_range R1, std::ranges::input_range R2>
    auto operator()(R1&& r1, R2&& r2) const
    {
        return concat_view<std::views::all_t<R1>, std::views::all_t<R2>>(
            std::views::all(std::forward<R1>(r1)),
            std::views::all(std::forward<R2>(r2)));
    }
};

inline constexpr ConcatFunc concat{};

} // fraze
