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
 * @file useful.hxx
 *
 * @brief
 *     Grab bag of tiny useful utilities.
 *
 */

#ifndef MSC_USEFUL_HXX
#define MSC_USEFUL_HXX

#include <algorithm>
#include <cassert>
#include <exception>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

/**
 * @def MSC_NOT_REACHED
 *
 * @brief
 *     Use this macro to annotate places that the control flow can never reach in a bug-free program.
 *
 */
#ifdef __GNUC__
#  define MSC_NOT_REACHED()  __builtin_trap()
#else
#  define MSC_NOT_REACHED()  ::std::terminate()
#endif

namespace msc
{

    /**
     * @brief
     *     Returns an iterator to the next item in a sequence, wrapping over to the first at the end.
     *
     * The behavior is undefined if `iter == last`.
     *
     * @tparam IterT
     *     iterator type
     *
     * @param iter
     *     iterator pointing to the current element
     *
     * @param first
     *     iterator pointing at the first element
     *
     * @param last
     *     iterator pointing after the last element
     *
     * @returns
     *     iterator pointing at the next element
     *
     */
    template <typename IterT>
    IterT cyclic_next(const IterT iter, const IterT first, const IterT last)
    {
        assert(iter != last);
        const auto next = std::next(iter);
        return (next != last) ? next : first;
    }

    /**
     * @brief
     *     Returns the element at the specified index of a container if it exists.
     *
     * @tparam ContainerT
     *     container type (random access)
     *
     * @tparam T
     *     element type (can be specified differently if implicit conversion is desired)
     *
     * @param container
     *     container to get element from
     *
     * @param index
     *     index of element to get
     *
     * @returns
     *     element at index or nothing
     *
     */
    template <typename ContainerT, typename T = typename ContainerT::value_type>
    std::optional<T> get_item(const ContainerT& container, const std::size_t index)
    {
        if (container.size() > index) {
            return container[index];
        } else {
            return std::nullopt;
        }
    }

    /**
     * @brief
     *     Given a non-empty list of equal values, returns the first value.
     *
     * The behavior is undefined if the sequence is empty or contains values that do not compare equal to its first
     * element.
     *
     * @tparam T
     *     type of the items
     *
     * @param items
     *     non-empty sequence of equal values
     *
     * @returns
     *     `items.front()`
     *
     */
    template <typename T>
    constexpr T get_same(const std::initializer_list<T> items)
    {
        assert(items.size() > 0);
        const auto answer = *std::begin(items);
        assert(std::all_of(std::begin(items), std::end(items), [answer](auto&& x){ return x == answer; }));
        return answer;
    }

    /**
     * @brief
     *     If the `src` contains a value (of type `SourceT`), it is `static_cast` to `TargetT` and returned as a new
     *     optional value; otherwise, an empty optional is returned.
     *
     * @tparam TargetT
     *     type of the optional value to cast from
     *
     * @tparam SourceT
     *     type of the optional value to cast to
     *
     * @param src
     *     optional value to cast
     *
     * @returns
     *     casted optional value
     *
     */
    template <typename TargetT, typename SourceT>
    std::optional<TargetT> optional_cast(std::optional<SourceT> src)
        noexcept(std::is_nothrow_move_constructible_v<TargetT>)
    {
        if (src) {
            return static_cast<TargetT>(std::move(*src));
        } else {
            return std::nullopt;
        }
    }

    /**
     * @brief
     *     Constructs a pair of shared pointers from a pair of unique pointers via the respective conversion
     *     constructor.
     *
     * This function cannot be `noexcept` because in the unlikely event that the memory allocation for the shared
     * pointer control block should fail, a `std::bad_alloc` will be thrown.
     *
     * @tparam T1
     *     type of the first managed object
     *
     * @tparam T2
     *     type of the second managed object
     *
     * @param duo
     *     pair of unique objects
     *
     * @returns
     *     pair of shared objects
     *
     */
    template <typename T1, typename T2>
    std::pair<std::shared_ptr<T1>, std::shared_ptr<T2>>
    share_pair(std::pair<std::unique_ptr<T1>, std::unique_ptr<T2>> duo)
    {
        return {std::move(duo.first), std::move(duo.second)};
    }

    /**
     * @brief
     *     Normalizes a textual constant name (as it might appear in JSON) in order to prepare it for being converted to
     *     a C++ enumerator constant.
     *
     * Normalization consists of trimming white-space, converting all characters to lower-case and replacing underscores
     * with dashes.
     *
     * @param name
     *     constant name to normalize
     *
     * @returns
     *     normalized constant name
     *
     */
    std::string normalize_constant_name(const std::string_view name);

    /**
     * @brief
     *     Reports an error due to an invalid enumerator constant.
     *
     * @param value
     *     offending numeric value of the enumeration constant
     *
     * @param name
     *     informal name for the enumeration type
     *
     * @throws std::invalid_argument
     *     that's its purpose
     *
     */
    [[noreturn]] void reject_invalid_enumeration(int value, std::string_view name);

    /**
     * @brief
     *     Convenience wrapper for `reject_invalid_enumeration` when the constant is already of the enumeration's type.
     *
     * @tparam EnumT
     *     type of the enum
     *
     * @param value
     *     offending enumeration constant
     *
     * @param name
     *     informal name for the enumeration type
     *
     * @throws std::invalid_argument
     *     that's its purpose
     *
     */
    template <typename EnumT>
    [[noreturn]] std::enable_if_t<std::is_enum_v<EnumT>>
    reject_invalid_enumeration(const EnumT value, const std::string_view name)
    {
        using raw_type = std::underlying_type_t<EnumT>;
        const auto raw = static_cast<raw_type>(value);
        reject_invalid_enumeration(raw, name);
    }

    /**
     * @brief
     *     Reports an error due to an invalid enumerator name.
     *
     * @param value
     *     offending enumeration name
     *
     * @param name
     *     informal name for the enumeration type
     *
     * @throws std::invalid_argument
     *     that's its purpose
     *
     */
    [[noreturn]] void reject_invalid_enumeration(std::string_view value, std::string_view name);

    /**
     * @brief
     *     Tests whether a string parses as decimal number and if so returns its value.
     *
     * This function will return nothing if the number looks like a decimal integer (matches `\d+`) but exceeds
     * `INT_MAX`.
     *
     * @param text
     *     text to check
     *
     * @returns
     *     just the numeric value or else nothing
     *
     */
    std::optional<int> parse_decimal_number(std::string_view text) noexcept;

    /**
     * @brief
     *     Squares a number.
     *
     * @tparam
     *     arithmetic type of the argument and result
     *
     * @param x
     *     value to square
     *
     * @returns
     *     squared value
     *
     */
    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, T> square(const T x) noexcept
    {
        return x * x;
    }

}  // namespace msc

#endif  // !defined(MSC_USEFUL_HXX)
