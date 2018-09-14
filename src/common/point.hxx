// -*- coding:utf-8; mode:c++; -*-

// Copyright (C) 2018 Karlsruhe Institute of Technology
// Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.

/**
 * @file point.hxx
 *
 * @brief
 *     <var>N</var>-dimensional points.
 *
 */

#ifndef MSC_POINT_HXX
#define MSC_POINT_HXX

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <tuple>
#include <type_traits>

namespace msc
{
    namespace detail
    {

        template <typename T, std::size_t N>
        class point_base
        {

            static_assert(std::is_floating_point_v<T>, "Points can only have real coordinates");

        public:

            using value_type      = T;
            using size_type       = std::size_t;
            using difference_type = std::ptrdiff_t;
            using reference       = value_type &;
            using const_reference = const value_type &;
            using pointer         = value_type *;
            using const_pointer   = const value_type *;
            using iterator        = pointer;
            using const_iterator  = const_pointer;

            constexpr point_base() noexcept = default;

            template
            <
                typename... Ts,
                typename = std::enable_if_t<(sizeof...(Ts) == N) and (... && std::is_convertible_v<Ts, value_type>)>
            >
            constexpr point_base(const Ts... coords) noexcept : _coords{ coords... }
            {
            }

            // Cannot be defined in-class because we need variadic expansion in the implementation.
            explicit constexpr operator bool() const noexcept;

            constexpr reference operator[](const std::size_t idx)
            {
                assert((idx >= 0) && (idx < N));
                return _coords[idx];
            }

            constexpr const_reference operator[](const std::size_t idx) const
            {
                assert((idx >= 0) && (idx < N));
                return _coords[idx];
            }

            template <std::size_t Idx>
            constexpr auto get() noexcept -> std::enable_if_t<(Idx < N), reference>
            {
                return std::get<Idx>(_coords);
            }

            template <std::size_t Idx>
            constexpr auto get() const noexcept -> std::enable_if_t<(Idx < N), const_reference>
            {
                return std::get<Idx>(_coords);
            }

            constexpr iterator begin() noexcept
            {
                return _coords.data();
            }

            constexpr iterator end() noexcept
            {
                return _coords.data() + _coords.size();
            }

            constexpr const_iterator begin() const noexcept
            {
                return _coords.data();
            }

            constexpr const_iterator end() const noexcept
            {
                return _coords.data() + _coords.size();
            }

            constexpr size_type size() const noexcept
            {
                static_assert(N == _coords.size());
                return N;
            }

            constexpr point_base& operator+=(const point_base& other) noexcept;
            constexpr point_base& operator-=(const point_base& other) noexcept;
            constexpr point_base& operator*=(const value_type alpha) noexcept;
            constexpr point_base& operator/=(const value_type alpha) noexcept;

        private:

            std::array<value_type, N> _coords{};

        };  // class point_base

        template <typename T, std::size_t... Is>
        constexpr bool test([[maybe_unused]] const point_base<T, sizeof...(Is)>& p,
                            std::index_sequence<Is...>) noexcept
        {
            //     ( any element non-zero  )    ( no element NaN          )
            return ( ... || (p[Is] != 0.0) ) && ( ... && (p[Is] == p[Is]) );
        }

        template <typename T, std::size_t N>
        constexpr point_base<T, N>::operator bool() const noexcept
        {
            return test(*this, std::make_index_sequence<N>());
        }

        template <typename T, std::size_t... Is>
        void add_to([[maybe_unused]] point_base<T, sizeof...(Is)>& self,
                    [[maybe_unused]] const point_base<T, sizeof...(Is)>& other,
                    std::index_sequence<Is...>) noexcept
        {
            ( ((void) (self.template get<Is>() += other.template get<Is>())), ... );
        }

        template <typename T, std::size_t... Is>
        void sub_to([[maybe_unused]] point_base<T, sizeof...(Is)>& self,
                    [[maybe_unused]] const point_base<T, sizeof...(Is)>& other,
                    std::index_sequence<Is...>) noexcept
        {
            ( ((void) (self.template get<Is>() -= other.template get<Is>())), ... );
        }

        template <typename T, std::size_t... Is>
        void mul_to([[maybe_unused]] point_base<T, sizeof...(Is)>& self,
                    [[maybe_unused]] const T alpha,
                    std::index_sequence<Is...>) noexcept
        {
            ( ((void) (self.template get<Is>() *= alpha)), ... );
        }

        template <typename T, std::size_t... Is>
        void div_to([[maybe_unused]] point_base<T, sizeof...(Is)>& self,
                    [[maybe_unused]] const T alpha,
                    std::index_sequence<Is...>) noexcept
        {
            ( ((void) (self.template get<Is>() /= alpha)), ... );
        }

        template <typename T, std::size_t N>
        constexpr point_base<T, N>& point_base<T, N>::operator+=(const point_base<T, N>& other) noexcept
        {
            add_to(*this, other, std::make_index_sequence<N>());
            return *this;
        }

        template <typename T, std::size_t N>
        constexpr point_base<T, N>& point_base<T, N>::operator-=(const point_base<T, N>& other) noexcept
        {
            sub_to(*this, other, std::make_index_sequence<N>());
            return *this;
        }

        template <typename T, std::size_t N>
        constexpr point_base<T, N>& point_base<T, N>::operator*=(const T alpha) noexcept
        {
            mul_to(*this, alpha, std::make_index_sequence<N>());
            return *this;
        }

        template <typename T, std::size_t N>
        constexpr point_base<T, N>& point_base<T, N>::operator/=(const T alpha) noexcept
        {
            div_to(*this, alpha, std::make_index_sequence<N>());
            return *this;
        }

    }  // namespace detail

    template <typename T, std::size_t N>
    struct point final : detail::point_base<T, N>
    {
        using detail::point_base<T, N>::point_base;
    };

    template <typename T>
    struct point<T, 2> final : detail::point_base<T, 2>
    {
        using detail::point_base<T, 2>::point_base;

        constexpr typename detail::point_base<T, 2>::reference x() noexcept
        {
            return this->template get<0>();
        }

        constexpr typename detail::point_base<T, 2>::const_reference x() const noexcept
        {
            return this->template get<0>();
        }

        constexpr typename detail::point_base<T, 2>::reference y() noexcept
        {
            return this->template get<1>();
        }

        constexpr typename detail::point_base<T, 2>::const_reference y() const noexcept
        {
            return this->template get<1>();
        }

    };  // point2

    template <typename T>
    struct point<T, 3> final : detail::point_base<T, 3>
    {
        using detail::point_base<T, 3>::point_base;

        constexpr typename detail::point_base<T, 3>::reference x() noexcept
        {
            return this->template get<0>();
        }

        constexpr typename detail::point_base<T, 3>::const_reference x() const noexcept
        {
            return this->template get<0>();
        }

        constexpr typename detail::point_base<T, 3>::reference y() noexcept
        {
            return this->template get<1>();
        }

        constexpr typename detail::point_base<T, 3>::const_reference y() const noexcept
        {
            return this->template get<1>();
        }

        constexpr typename detail::point_base<T, 3>::reference z() noexcept
        {
            return this->template get<2>();
        }

        constexpr typename detail::point_base<T, 3>::const_reference z() const noexcept
        {
            return this->template get<2>();
        }

    };  // point3

    using point2d = point<double, 2>;
    using point3d = point<double, 3>;

    template <std::size_t Idx, typename T, std::size_t N>
    constexpr auto get(point<T, N>& p) noexcept -> decltype(p.template get<Idx>())
    {
        return p.template get<Idx>();
    }

    template <std::size_t Idx, typename T, std::size_t N>
    constexpr auto get(const point<T, N>& p) noexcept -> decltype(p.template get<Idx>())
    {
        return p.template get<Idx>();
    }

    namespace detail
    {

        template <typename T, std::size_t... Is>
        constexpr bool
        cmp([[maybe_unused]] const point<T, sizeof...(Is)>& p1,
            [[maybe_unused]] const point<T, sizeof...(Is)>& p2,
            std::index_sequence<Is...>) noexcept
        {
            return ( ... && (get<Is>(p1) == get<Is>(p2)) );
        }

        template <typename T, std::size_t... Is>
        constexpr point<T, sizeof...(Is)>
        add([[maybe_unused]] const point<T, sizeof...(Is)>& p1,
            [[maybe_unused]] const point<T, sizeof...(Is)>& p2,
            std::index_sequence<Is...>) noexcept
        {
            return { (get<Is>(p1) + get<Is>(p2)) ... };
        }

        template <typename T, std::size_t... Is>
        constexpr point<T, sizeof...(Is)>
        sub([[maybe_unused]] const point<T, sizeof...(Is)>& p1,
            [[maybe_unused]] const point<T, sizeof...(Is)>& p2,
            std::index_sequence<Is...>) noexcept
        {
            return { (get<Is>(p1) - get<Is>(p2)) ... };
        }

        template <typename T, std::size_t... Is>
        constexpr point<T, sizeof...(Is)>
        mul([[maybe_unused]] const point<T, sizeof...(Is)>& p,
            [[maybe_unused]] const T alpha,
            std::index_sequence<Is...>) noexcept
        {
            return { (get<Is>(p) * alpha) ... };
        }

        template <typename T, std::size_t... Is>
        constexpr point<T, sizeof...(Is)>
        div([[maybe_unused]] const point<T, sizeof...(Is)>& p,
            [[maybe_unused]] const T alpha,
            std::index_sequence<Is...>) noexcept
        {
            return { (get<Is>(p) / alpha) ... };
        }

        template <typename T, std::size_t... Is>
        constexpr point<T, sizeof...(Is)>
        neg([[maybe_unused]] const point<T, sizeof...(Is)>& p,
            std::index_sequence<Is...>) noexcept
        {
            return { -get<Is>(p) ... };
        }

        template <typename T, std::size_t... Is>
        constexpr T
        dot([[maybe_unused]] const point<T, sizeof...(Is)>& p1,
            [[maybe_unused]] const point<T, sizeof...(Is)>& p2,
            std::index_sequence<Is...>) noexcept
        {
            return ( T{0} + ... + (get<Is>(p1) * get<Is>(p2)) );
        }

        template <typename T, std::size_t... Is>
        constexpr point<T, sizeof...(Is)>
        make_from_val([[maybe_unused]] const T value, std::index_sequence<Is...>) noexcept
        {
            return { (((void) Is), value) ... };
        }

        template <typename T, std::size_t... Is>
        constexpr point<T, sizeof...(Is)>
        make_from_idx([[maybe_unused]] const std::size_t idx, std::index_sequence<Is...>) noexcept
        {
            return { ((Is == idx) ? T{1} : T{0}) ... };
        }

        template <typename T, typename FuncT, std::size_t... Is>
        constexpr point<T, sizeof...(Is)>
        make_from_gen([[maybe_unused]] FuncT&& gen, std::index_sequence<Is...>) noexcept
        {
            return { (((void) Is), gen()) ... };
        }

    }  // namespace detail

    template <typename T, std::size_t N>
    constexpr bool operator==(const point<T, N>& p1, const point<T, N>& p2) noexcept
    {
        return detail::cmp(p1, p2, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr bool operator!=(const point<T, N>& p1, const point<T, N>& p2) noexcept
    {
        return !(p1 == p2);
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> operator+(const point<T, N>& p1, const point<T, N>& p2) noexcept
    {
        return detail::add(p1, p2, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> operator-(const point<T, N>& p1, const point<T, N>& p2) noexcept
    {
        return detail::sub(p1, p2, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> operator*(const T alpha, const point<T, N>& p) noexcept
    {
        return detail::mul(p, alpha, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> operator*(const point<T, N>& p, const T alpha) noexcept
    {
        return detail::mul(p, alpha, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> operator/(const point<T, N>& p, const T alpha) noexcept
    {
        return detail::div(p, alpha, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> operator-(const point<T, N>& p) noexcept
    {
        return detail::neg(p, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr T dot(const point<T, N>& p1, const point<T, N>& p2) noexcept
    {
        return detail::dot(p1, p2, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr T normsq(const point<T, N>& p) noexcept
    {
        return dot(p, p);
    }

    template <typename T, std::size_t N>
    T abs(const point<T, N>& p) noexcept
    {
        return std::sqrt(dot(p, p));
    }

    template <typename T, std::size_t N>
    T distance(const point<T, N>& p1, const point<T, N>& p2) noexcept
    {
        return abs(p1 - p2);
    }

    template <typename T>
    constexpr point<T, 3> cross(const point<T, 3>& p1, const point<T, 3>& p2) noexcept
    {
        const auto x = p1.y() * p2.z() - p1.z() * p2.y();
        const auto y = p1.z() * p2.x() - p1.x() * p2.z();
        const auto z = p1.x() * p2.y() - p1.y() * p2.x();
        return {x, y, z};
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> make_point(const T value = T{}) noexcept
    {
        return detail::make_from_val<T>(value, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> make_unit_point(const std::size_t idx)
    {
        assert(idx < N);
        return detail::make_from_idx<T>(idx, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N, typename EngineT, typename DistrT>
    constexpr point<T, N> make_random_point(EngineT& rndeng, DistrT& rnddst) noexcept(noexcept(rnddst(rndeng)))
    {
        return detail::make_from_gen<T>(
            [&rndeng, &rnddst](){ return rnddst(rndeng); },
            std::make_index_sequence<N>()
        );
    }

    template <typename T, std::size_t N>
    constexpr point<T, N> make_invalid_point() noexcept
    {
        const auto value = std::numeric_limits<T>::quiet_NaN();
        return detail::make_from_val<T>(value, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    point<T, N> normalized(const point<T, N>& p)
    {
        assert(p);
        return p / abs(p);
    }

    template <typename T, std::size_t N>
    std::ostream& operator<<(std::ostream& ostr, const point<T, N>& p)
    {
        ostr << "(";
        for (std::size_t i = 0; i < N; ++i) {
            if (i > 0) { ostr << ", "; }
            ostr << p[i];
        }
        ostr << ")";
        return ostr;
    }

    template <typename T, std::size_t N>
    std::istream& operator>>(std::istream& istr, point<T, N>& p)
    {
        char temp;
        if (!(istr >> std::ws) || !(istr >> temp) || (temp != '(')) {
            goto failure;
        }
        for (std::size_t i = 0; i < N; ++i) {
            if ((i > 0) && (!(istr >> std::ws) || !(istr >> temp) || (temp != ','))) {
                goto failure;
            }
            if (!(istr >> std::ws) || !(istr >> p[i])) {
                goto failure;
            }
        }
        if (!(istr >> std::ws) || !(istr >> temp) || (temp != ')') || !(istr >> std::ws)) {
            goto failure;
        }
        return istr;
    failure:
        istr.setstate(std::ios_base::failbit);
        p = make_invalid_point<T, N>();
        return istr;
    }

    template <typename T>
    struct point_order
    {

        template <std::size_t N>
        constexpr bool operator()(const point<T, N>& p1, const point<T, N>& p2) const noexcept
        {
            for (const auto diff : p1 - p2) {
                if (diff < 0.0) return true;
                if (diff > 0.0) return false;
            }
            return false;
        }

    };

}  // namespace msc

namespace std
{

    template <typename T, std::size_t N>
    struct tuple_size<msc::point<T, N>> : std::integral_constant<std::size_t, N> { };

    template <std::size_t Idx, typename T, std::size_t N>
    struct tuple_element<Idx, msc::point<T, N>> { using type = typename msc::point<T, N>::reference; };

    template <std::size_t Idx, typename T, std::size_t N>
    struct tuple_element<Idx, const msc::point<T, N>> { using type = typename msc::point<T, N>::const_reference; };

}  // namespace std

#endif  // !defined(MSC_POINT_HXX)
