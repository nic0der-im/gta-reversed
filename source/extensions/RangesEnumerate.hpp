#pragma once

/*
 * Native build replacement for `std::views::enumerate` (C++23), which libc++ doesn't
 * implement yet. It's pulled into `rngv` by `Base.h`, so the rest of the code base keeps
 * using `rngv::enumerate` no matter the build.
 *
 * Unlike the standard one, elements are exposed as `std::tuple<index, reference>`, which
 * is enough for every call site here (structured bindings and `std::get`).
 */

#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>

namespace rngv {

using namespace std::views;

namespace detail {

template<std::ranges::view V>
class enumerate_view : public std::ranges::view_interface<enumerate_view<V>> {
public:
    class iterator {
    public:
        using iterator_concept = std::input_iterator_tag;
        using difference_type  = std::ptrdiff_t;
        using value_type       = std::tuple<std::size_t, std::ranges::range_value_t<V>>;
        using reference        = std::tuple<std::size_t, std::ranges::range_reference_t<V>>;

        iterator() = default;

        iterator(std::ranges::iterator_t<V> it, std::size_t idx) :
            m_It{ std::move(it) },
            m_Index{ idx }
        {
        }

        reference operator*() const { return { m_Index, *m_It }; }

        iterator& operator++() {
            ++m_It;
            ++m_Index;
            return *this;
        }

        void operator++(int) { ++*this; }

        bool operator==(const iterator& other) const { return m_It == other.m_It; }

    private:
        std::ranges::iterator_t<V> m_It{};
        std::size_t                m_Index{};
    };

    explicit enumerate_view(V base) : m_Base{ std::move(base) } { }

    auto begin() { return iterator{ std::ranges::begin(m_Base), 0 }; }
    auto end()   { return iterator{ std::ranges::end(m_Base), 0 }; }

    auto begin() const requires std::ranges::input_range<const V> { return iterator{ std::ranges::begin(m_Base), 0 }; }
    auto end()   const requires std::ranges::input_range<const V> { return iterator{ std::ranges::end(m_Base), 0 }; }

private:
    V m_Base;
};

struct enumerate_fn {
    template<std::ranges::viewable_range R>
    auto operator()(R&& r) const {
        return enumerate_view<std::views::all_t<R>>{ std::views::all(std::forward<R>(r)) };
    }

    template<std::ranges::viewable_range R>
    friend auto operator|(R&& r, enumerate_fn) {
        return enumerate_fn{}(std::forward<R>(r));
    }
};

} // namespace detail

inline constexpr detail::enumerate_fn enumerate{};

} // namespace rngv
