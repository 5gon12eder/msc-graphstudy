// -*- coding:utf-8; mode:c++; -*-

// Copyright (C) 2016 Moritz Klammler <moritz.klammler@student.kit.edu>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "unittest.hxx"

#include <iostream>
#include <random>
#include <string>

namespace /*anonymous*/
{

    double random_real()
    {
        static auto rndeng = std::default_random_engine {std::random_device{}()};
        auto rnddst = std::uniform_real_distribution{};
        return rnddst(rndeng);
    }

    struct my_error{};

    void function_that_might_throw(const bool doit)
    {
        if (doit) {
            throw my_error{};
        }
    }

    MSC_AUTO_TEST_CASE(require_exception)
    {
        MSC_REQUIRE_EXCEPTION(my_error, function_that_might_throw(true));
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_EXCEPTION(my_error, function_that_might_throw(false)));
    }

    MSC_AUTO_TEST_CASE(require)
    {
        MSC_REQUIRE(true);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE(false));
    }

    MSC_AUTO_TEST_CASE(require_eq)
    {
        using namespace std::string_literals;
        MSC_REQUIRE_EQ("alpha"s, "alpha"s);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_EQ("alpha"s, "beta"s));
        const auto x = random_real();
        const auto delta = 1.0E-10;
        MSC_REQUIRE_EQ(x, x);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_EQ(x, x + delta));
    }

    MSC_AUTO_TEST_CASE(require_ne)
    {
        using namespace std::string_literals;
        MSC_REQUIRE_NE("alpha"s, "beta"s);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_NE("alpha"s, "alpha"s));
        const auto x = random_real();
        const auto delta = 1.0E-10;
        MSC_REQUIRE_NE(x + delta, x - delta);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_NE(x, x));
    }

    MSC_AUTO_TEST_CASE(require_lt)
    {
        using namespace std::string_literals;
        MSC_REQUIRE_LT("abc"s, "abd"s);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_LT("abc"s, "abc"s));
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_LT("abc"s, "abb"s));
        const auto x = random_real();
        const auto delta = 1.0E-10;
        MSC_REQUIRE_LT(x - delta, x + delta);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_LT(x, x));
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_LT(x, x - delta));
    }

    MSC_AUTO_TEST_CASE(require_le)
    {
        using namespace std::string_literals;
        MSC_REQUIRE_LE("abc"s, "abd"s);
        MSC_REQUIRE_LE("abc"s, "abc"s);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_LE("abc"s, "abb"s));
        const auto x = random_real();
        const auto delta = 1.0E-10;
        MSC_REQUIRE_LE(x, x);
        MSC_REQUIRE_LE(x - delta, x + delta);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_LE(x, x - delta));
    }

    MSC_AUTO_TEST_CASE(require_gt)
    {
        using namespace std::string_literals;
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_GT("abc"s, "abd"s));
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_GT("abc"s, "abc"s));
        MSC_REQUIRE_GT("abc"s, "abb"s);
        const auto x = random_real();
        const auto delta = 1.0E-10;
        MSC_REQUIRE_GT(x + delta, x - delta);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_GT(x, x));
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_GT(x, x + delta));
    }

    MSC_AUTO_TEST_CASE(require_ge)
    {
        using namespace std::string_literals;
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_GE("abc"s, "abd"s));
        MSC_REQUIRE_GE("abc"s, "abc"s);
        MSC_REQUIRE_GE("abc"s, "abb"s);
        const auto x = random_real();
        const auto delta = 1.0E-10;
        MSC_REQUIRE_GE(x + delta, x - delta);
        MSC_REQUIRE_GE(x, x);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_GE(x, x + delta));
    }

    MSC_AUTO_TEST_CASE(require_close)
    {
        MSC_REQUIRE_CLOSE(0.0, 1.0, 1.0);
        MSC_REQUIRE_CLOSE(1.0E-5, 1.0, 1.0 + 1.0E-6);
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_CLOSE(1.0E-6, 1.0, 1.0 + 1.0E-5));
    }

    MSC_AUTO_TEST_CASE(require_match)
    {
        MSC_REQUIRE_MATCH("abc", "abc");
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_MATCH("abc", "abcd"));
        MSC_REQUIRE_FAILURE(MSC_REQUIRE_MATCH("abc", "aabc"));
        MSC_REQUIRE_MATCH(".*", "random garbage");
        MSC_REQUIRE_MATCH("\\d{8}(-\\d{4}){3}-\\d{12}", "46373666-3860-4966-9333-435330520335");
    }

}

int main(const int argc, const char *const *const argv)
{
    std::cerr << "Please ignore the spurious \"error\" messages printed by this test." << std::endl;
    return MSC_RUN_ALL_UNIT_TESTS(argc, argv);
}
