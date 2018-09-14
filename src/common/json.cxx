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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "json.hxx"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>

namespace msc
{

    namespace /*anonymous*/
    {

        std::string hex_escape(const int c)
        {
            using namespace std::string_literals;
            using namespace std::string_view_literals;
            static const auto hexdigits = "0123456789abcdef"sv;
            const auto hi = (static_cast<unsigned>(c) >> 4) & 0xfu;
            const auto lo = (static_cast<unsigned>(c) >> 0) & 0xfu;
            auto result = std::string{"\\x"};
            result.push_back(hexdigits.at(hi));
            result.push_back(hexdigits.at(lo));
            return result;
        }

        std::ostream& write_real_finite(std::ostream& ostr, const double value)
        {
            constexpr auto digits = std::numeric_limits<double>::max_digits10;
            char buffer[digits + 10];
            const auto count = std::snprintf(buffer, sizeof(buffer), "%.*E", digits, value);
            if ((count < 0) || (static_cast<std::size_t>(count) > sizeof(buffer))) {
                throw std::runtime_error{"Cannot represent floating-point value as JSON"};
            }
            ostr.write(buffer, count);
            return ostr;
        }

        class wrapped_string_view final
        {
        public:

            wrapped_string_view(const std::string_view text) : _text{text} { /* empty */ }

            friend std::ostream& operator<<(std::ostream& ostr, const wrapped_string_view& wsv)
            {
                const auto first = wsv._text.data();
                const auto last = first + wsv._text.size();
                ostr << '"';
                for (auto it = first; it != last; ++it) {
                    const auto c = static_cast<int>(static_cast<unsigned char>(*it));
                    switch (c) {
                    case ' ':  ostr << ' ';    break;
                    case '\"': ostr << "\\\""; break;
                    case '\\': ostr << "\\\\"; break;
                    case '\a': ostr << "\\a";  break;
                    case '\b': ostr << "\\b";  break;
                    case '\f': ostr << "\\f";  break;
                    case '\n': ostr << "\\n";  break;
                    case '\r': ostr << "\\r";  break;
                    case '\t': ostr << "\\t";  break;
                    case '\v': ostr << "\\v";  break;
                    default:
                        if (std::isalnum(c) || std::ispunct(c)) {
                            ostr.put(c);
                        } else {
                            ostr << hex_escape(c);
                        }
                    }
                }
                ostr << '"';
                return ostr;
            }

        private:

            std::string_view _text{};

        };  // class wrapped_string_view

    }  // namespace /*anonymous*/

    std::ostream& operator<<(std::ostream& ostr, [[maybe_unused]] const json_null& obj)
    {
        return ostr << "null";
    }

    std::ostream& operator<<(std::ostream& ostr, const json_text& obj)
    {
        return ostr << wrapped_string_view{obj.value};
    }

    std::ostream& operator<<(std::ostream& ostr, const json_bool& obj)
    {
        return ostr << (obj.value ? "true" : "false");
    }

    std::ostream& operator<<(std::ostream& ostr, const json_real& obj)
    {
        switch (std::fpclassify(obj.value)) {
        case FP_INFINITE:
            return ostr << ((obj.value < 0.0) ? "-Infinity" : "Infinity");
        case FP_NAN:
            return ostr << "NaN";
        case FP_NORMAL:
        case FP_SUBNORMAL:
        case FP_ZERO:
            return write_real_finite(ostr, obj.value);
        default:
            throw std::invalid_argument{"Cannot represent real value of unknown floating-point category as JSON"};
        }
    }

    std::ostream& operator<<(std::ostream& ostr, const json_size& obj)
    {
        return ostr << obj.value;
    }

    std::ostream& operator<<(std::ostream& ostr, const json_diff& obj)
    {
        return ostr << obj.value;
    }

    std::ostream& operator<<(std::ostream& ostr, const json_array& obj)
    {
        ostr << "[";
        for (std::size_t i = 0; i < obj.size(); ++i) {
            if (i > 0) {
                ostr << ", ";
            }
            ostr << obj[i];
        }
        ostr << "]";
        return ostr;
    }

    std::ostream& operator<<(std::ostream& ostr, const json_object& obj)
    {
        ostr << "{";
        for (auto it = std::cbegin(obj); it != std::cend(obj); ++it) {
            if (it != std::cbegin(obj)) {
                ostr << ", ";
            }
            ostr << wrapped_string_view{it->first} << ": " << it->second;
        }
        ostr << "}";
        return ostr;
    }

    std::ostream& operator<<(std::ostream& ostr, const json_any& obj)
    {
        const auto visitor = [op = std::addressof(ostr)](auto&& val)->std::ostream&{ return ((*op) << val); };
        return std::visit(visitor, obj);
    }

    void json_object::update(const json_object& other)
    {
        for (const auto& [key, value] : other) {
            this->insert_or_assign(key, value);
        }
    }

    void json_object::update(json_object&& other)
    {
        // https://codereview.stackexchange.com/a/181167/67456
        this->swap(other);
        this->merge(other);
        other.clear();
    }

}  // namespace msc
