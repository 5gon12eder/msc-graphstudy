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
 * @file json.hxx
 *
 * @brief
 *     Utilities for JSON output.
 *
 */

#ifndef MSC_JSON_HXX
#define MSC_JSON_HXX

#include <cstddef>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace msc
{
    /**
     * @brief
     *     Represents a `null` value.
     *
     */
    struct json_null final
    {
        /** @brief Tag to identify this as a primitive JSON type. */
        using primitive_json_tag_type = void;

        /** @brief Constructs a `null` value. */
        json_null() noexcept = default;

        /** @brief Constructs a `null` value. */
        json_null(std::nullptr_t) noexcept { /* empty */ }
    };

    /**
     * @brief
     *     Wrapper for a text value.
     *
     */
    struct json_text final
    {
        /** @brief Tag to identify this as a primitive JSON type. */
        using primitive_json_tag_type = void;

        /**
         * @brief
         *     Constructs an empty JSON string.
         *
         */
        json_text() noexcept = default;

        /**
         * @brief
         *     Constructs a JSON string with the given value.
         *
         * @param text
         *     text to copy into the JSON string
         *
         */
        json_text(const std::string_view text) : value{text.data(), text.size()}
        {
        }

        /**
         * @brief
         *     Constructs a JSON string with the given value.
         *
         * @param text
         *     text to copy into the JSON string
         *
         */
        json_text(const char *const text) : value{text}
        {
        }

        /**
         * @brief
         *     Constructs a JSON string with the given value.
         *
         * @param text
         *     text to copy into the JSON string
         *
         */
        json_text(const std::string& text) : value{text}
        {
        }

        /**
         * @brief
         *     Constructs a JSON string with the given value.
         *
         * @param text
         *     text to move into the JSON string
         *
         */
        json_text(std::string&& text) : value{std::move(text)}
        {
        }

        /** @brief The wrapped value. */
        std::string value{};

    };  // struct json_text

    /**
     * @brief
     *     Wrapper for a logical value.
     *
     */
    struct json_bool final
    {
        /** @brief Tag to identify this as a primitive JSON type. */
        using primitive_json_tag_type = void;

        /** @brief The value held by this object. */
        bool value{};
    };

    /**
     * @brief
     *     Wrapper for a real value.
     *
     */
    struct json_real final
    {
        /** @brief Tag to identify this as a primitive JSON type. */
        using primitive_json_tag_type = void;

        /** @brief The value held by this object. */
        double value{};
    };

    /**
     * @brief
     *     Wrapper for a `size_t` value.
     *
     * The JSON format does not support integer types but using this value in C++ is useful and ensures nicer output (no
     * trailing zeros).
     *
     */
    struct json_size final
    {
        /** @brief Tag to identify this as a primitive JSON type. */
        using primitive_json_tag_type = void;

        /** @brief The value held by this object. */
        std::size_t value{};
    };

    /**
     * @brief
     *     Wrapper for a `ptrdiff_t` value.
     *
     * The JSON format does not support integer types but using this value in C++ is useful and ensures nicer output (no
     * trailing zeros).
     *
     */
    struct json_diff final
    {
        /** @brief Tag to identify this as a primitive JSON type. */
        using primitive_json_tag_type = void;

        /** @brief The value held by this object. */
        std::ptrdiff_t value{};
    };

    struct json_array;
    struct json_object;

    /** @brief Polymorphic type holding any JSON value (including `null`). */
    using json_any = std::variant<
        json_null, json_text, json_bool, json_real, json_size, json_diff, json_array, json_object
    >;

    /**
     * @brief
     *     Ordered array of zero or more arbitrary JSON values.
     *
     */
    struct json_array final : std::vector<json_any>
    {
        using std::vector<json_any>::vector;
    };

    /**
     * @brief
     *     Unordered associative array of zero or more arbitrary JSON values.
     *
     */
    struct json_object final : std::map<std::string, json_any, std::less<>>
    {
        using std::map<std::string, json_any, std::less<>>::map;

        /**
         * @brief
         *     Additional convenience overload that allows indexing a `json_object` with a `std::string_view` directly.
         *
         * This function creates a temporary `std::string` object so it is not "transparent".
         *
         * @param key
         *     member name to look up and insert (default-construction) if not already present
         *
         * @returns
         *     reference to the (possibly just inserted) member associated with the given name
         *
         */
        json_any& operator[](const std::string_view key)
        {
            // The funky qualification is necessary to avoid an infinite recursion.
            using namespace std;
            return this->map<string, json_any, less<>>::operator[](std::string{key.data(), key.size()});
        }

        /**
         * @brief
         *     Inserts all members of the given JSON object into this one, possibly overwriting alerady existing
         *     members.
         *
         * @param other
         *     JSON object to copy the members from
         *
         */
        void update(const json_object& other);

        /**
         * @brief
         *     Inserts all members of the given JSON object into this one, possibly overwriting alerady existing
         *     members.
         *
         * @param other
         *     JSON object to copy the members from
         *
         */
        void update(json_object&& other);

    };  // struct json_object

    /**
     * @brief
     *     Creates a JSON object of type `JsonT` if `optval` holds a value or else `json_null`.
     *
     * @tparam JsonT
     *     type of the JSON object to create if a value is available
     *
     * @tparam T
     *     type of the value to construct a JSON object from
     *
     * @param optval
     *     optional value to construct the JSON object from
     *
     * @returns
     *     JSON object that either holds `JsonT{*optval}` or `null`
     *
     */
    template <typename JsonT, typename T>
    json_any make_json(const std::optional<T>& optval)
    {
        if (!optval) {
            return json_null{};
        } else {
            return JsonT{*optval};
        }
    }

    /**
     * @brief
     *     Creates a JSON object of type `json_text` if `text` is non-empty or else `json_null`.
     *
     * @param text
     *     possibly empty string to store in a JSON object
     *
     * @returns
     *     JSON object that either holds `json_text{text}` or `null`
     *
     */
    inline json_any make_json(const std::string_view text)
    {
        if (text.empty()) {
            return json_null{};
        } else {
            return json_text{text};
        }
    }

    /**
     * @brief
     *     Streams out a `null` value as JSON.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_null& obj);

    /**
     * @brief
     *     Streams out a text value as JSON.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_text& obj);

    /**
     * @brief
     *     Streams out a logical value as JSON.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_bool& obj);

    /**
     * @brief
     *     Streams out a real value as JSON.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_real& obj);

    /**
     * @brief
     *     Streams out a size value as JSON.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_size& obj);

    /**
     * @brief
     *     Streams out a difference value as JSON.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_diff& obj);

    /**
     * @brief
     *     Streams out a JSON array.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_array& obj);

    /**
     * @brief
     *     Streams out a JSON object.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_object& obj);

    /**
     * @brief
     *     Streams out any JSON value.
     *
     * @param ostr
     *     stream to write to
     *
     * @param obj
     *     value to write
     *
     * @returns
     *     a reference to the stream
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const json_any& obj);

}  // namespace msc

#endif  // !defined(MSC_JSON_HXX)
