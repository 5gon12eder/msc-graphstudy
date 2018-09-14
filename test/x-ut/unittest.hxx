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

/**
 * @file unittest.hxx
 *
 * @brief
 *     A small and simple unit testing support library.
 *
 * Unlike many other testing libraries, this one is focused on simplicity.  There is no dark macro, template or
 * operating system magic.  Just the minimum set of features in order to effectively write unit tests with confidence
 * that if something is broken, it's your code.
 *
 * Unit tests can be written as ordinary functions (of type `void`), called *test cases*.  If the function returns
 * normally, the test is considered passed.  Four cases of how the function might not return normally are distinguished.
 *
 * - If the test checks a condition and finds it not true, it shall throw an exception of implementation-defined type
 *   `Failure` (that is not derived from `std::exception` so you can `catch` the latter without doubts in your tests)
 *   that aborts the execution of the current test (after running any RAII cleanup, of course) and marks it as *failed*.
 *   This is done by making use of the `MSC_REQUIRE_*` macros inside the test function body.
 *
 * - If the test figures out that it is unable to perform a meaningful check for reasons that are not a bug in the code
 *   to be tested, it shall throw an exception of implementation-defined type `Skip` (that is not derived from
 *   `std::exception` so you can `catch` the latter without doubts in your tests) that aborts the execution of the
 *   current test (after running any RAII cleanup, of course) and marks it as *skipped*.  This is done by making use of
 *   the `MSC_SKIP_*` macros inside the test function body.  Skipped tests don't count towards the total test result.
 *
 * - If the test encounters a fatal error before it can even perform its actual testing or the code under test fails for
 *   unexpected reasons, that is, if an uncaught exception that is neither `Failure` nore `Skip` escapes the test
 *   function, the test is marked as *error*.  For completeness, this library also provides a third
 *   implementation-defined type `Error` (that is also not derived from `std::exception`) that can purposefully be
 *   triggered by using the `MSC_ERROR_*` macros.
 *
 * - If the function exits by any other means such as by doing a `longjmp`, calling `exit` or switching stacks, the
 *   behavior is undefined.
 *
 * In order to actually run the test functions, one of the `MSC_RUN_*` macros can be used.  (Of course, the tests --
 * wich are just ordinary functions, after all -- can also be called directly.)
 *
 * In addition, this library provides the option to register test cases globally which is intended to support the comon
 * case that a test suite is comprised of a relatively small number of tests in a single translation unit.  To register
 * a test, the `MSC_REGISTER_*` macros can be used.
 *
 * However, there is an even more convenient way by using the `AUTO_TEST_CASE` macro which will register the test
 * automatically through some static initialization trick.  (This is actually the dirtiest part of this library but
 * still pretty straight-forward.)
 *
 * Finally, since writing the `main` functions for each unit test file can become tedious, this library can provide a
 * `main` function for you that will by default run all globally registred tests.  Simply define the macro
 * `%MSC_RUN_ALL_UNIT_TESTS_IN_MAIN` *before* you include this file in order to get a `main` provided.
 *
 * @note
 *     When executing the tests, the environment variable `%MSC_TEST_ANSI_TERMINAL` can be set to a non-empty string in
 *     order to produce colorized output using ANSI escape sequences.
 *
 * @todo
 *     The last bullet point is of course unfortunate.  Especially since it is the point of testing not to trust the
 *     code under test.  The seemingly obvious solution would be to `fork()` before running each test.  We shall
 *     investigate this possibility, which would also give us confidence that one test cannot clobber the global state
 *     seen by another, in the future.
 *
 */

#ifndef MSC_UNITTEST_HXX
#define MSC_UNITTEST_HXX

/**
 * @brief
 *     Defines a test case and automatically registers it globally.
 *
 * The `NAME` must be a valid and unique C++ identifier.  Directly following the invocation of this macro must come a
 * function body with the test code.
 *
 *     MSC_AUTO_TEST_CASE(example)
 *     {
 *         using namespace std::string_literals;
 *         MSC_REQUIRE_EQ("42"s, std::to_string(42));
 *     }
 *
 * @param NAME
 *     name for the test case
 *
 */
#define MSC_AUTO_TEST_CASE( NAME )                                                                                      \
    struct NAME                                                                                                         \
    {                                                                                                                   \
        NAME () noexcept = default;                                                                                     \
        void operator()() const;                                                                                        \
    };                                                                                                                  \
    volatile std::size_t _dc7c12e5b72012bb_ ## NAME ## _reg_index = MSC_REGISTER_TEST_CASE( #NAME, NAME {} );           \
    void NAME::operator()() const  // User-privided test body follows here ...

/**
 * @brief
 *     Globally registers a test case.
 *
 * This macro expands to an integer that represents the number of currently registred test.
 *
 * @param NAME
 *     string to uniquely identify this test case
 *
 * @param FUNC
 *     callable of type `void()` that should be run as test case
 *
 */
#define MSC_REGISTER_TEST_CASE( NAME, FUNC )                                                                            \
    ::dc7c12e5b72012bbc7e47c3e5436903c::register_test_case( NAME, FUNC )

/**
 * @brief
 *     Runs all globally registred test cases.
 *
 * The `ARGC` and `ARGV` parameters should be the same as for a program's `main` function.  The macro will expand to an
 * integer that is good as exit code for `main`.
 *
 * @param ARGC
 *     number of command-line arguments
 *
 * @param ARGV
 *     vector of command-line arguments
 *
 */
#define MSC_RUN_ALL_UNIT_TESTS( ARGC, ARGV )                                                                            \
    ::dc7c12e5b72012bbc7e47c3e5436903c::run_registred_unit_tests( ARGC, ARGV )

/**
 * @brief
 *     Discards all globally registred test cases and registres and runs the provided functions instead.
 *
 * The `ARGC` and `ARGV` parameters should be the same as for a program's `main` function.  The following parameters
 * should be one or more function pointers of type `void(*)()` that define the anonymous test cases to be run.  The
 * macro will expand to an integer that is good as exit code for `main`.
 *
 * @param ARGC
 *     number of command-line arguments
 *
 * @param ARGV
 *     vector of command-line arguments
 *
 * @param ...
 *     test functions
 *
 */
#define MSC_RUN_UNIT_TESTS( ARGC, ARGV, ... )                                                                           \
    ::dc7c12e5b72012bbc7e47c3e5436903c::run_unit_tests( ARGC, ARGV, __VA_ARGS__ )

/**
 * @brief
 *     Same as `MSC_RUN_UNIT_TESTS` with zero variadic arguments (which is not valid pre-processor syntax).
 *
 * @param ARGC
 *     number of command-line arguments
 *
 * @param ARGV
 *     vector of command-line arguments
 *
 */
#define MSC_RUN_NO_UNIT_TESTS( ARGC, ARGV )                                                                             \
    ::dc7c12e5b72012bbc7e47c3e5436903c::run_unit_tests( ARGC, ARGV )

/**
 * @brief
 *     Tests that an expression throws an exception of a certain type.
 *
 * Executes `EXPR` in a `try` / `catch` block and fail the test unless an exception of type `EXTYPE` was thrown.  If any
 * other exception is thrown, it will escape uncaught.
 *
 *     // Make sure the call to `std::stoi("xlii")` throws an `std::invalid_argument` exception.
 *     MSC_REQUIRE_EXCEPTION(std::invalid_argument, std::stoi("xlii"));
 *
 * @warning
 *     Unlike most of the other `MSC_REQUIRE_*` macros, this one does not expand to an expression.  It can still be used
 *     as a statement, though.
 *
 * @param EXTYPE
 *     type of the expected exception
 *
 * @param EXPR
 *     expression that should throw
 *
 */
#define MSC_REQUIRE_EXCEPTION( EXTYPE, EXPR )                                                                           \
    do {                                                                                                                \
        auto _dc7c12e5b72012bb_thrown = false;                                                                          \
        auto _dc7c12e5b72012bb_info = ::dc7c12e5b72012bbc7e47c3e5436903c::failed_t{};                                   \
        try {                                                                                                           \
            EXPR;                                                                                                       \
            _dc7c12e5b72012bb_info.assign( __FILE__, __LINE__, "Exception not thrown: " #EXTYPE );                      \
            _dc7c12e5b72012bb_info.amend( #EXPR );                                                                      \
        } catch (const EXTYPE&) {                                                                                       \
            _dc7c12e5b72012bb_thrown = true;                                                                            \
        }                                                                                                               \
        if (!_dc7c12e5b72012bb_thrown) {                                                                                \
            throw _dc7c12e5b72012bb_info;                                                                               \
        }                                                                                                               \
    } while (false)

/**
 * @brief
 *     Tests that an expression throws an exception.
 *
 * `MSC_REQUIRE_THROWS(EXPR)` is the same as `MSC_REQUIRE_EXCEPTION(std::exception, EXPR)`.
 *
 * @warning
 *     The same concerns as for `MSC_REQUIRE_EXCEPTION` apply.
 *
 * @param EXPR
 *     expression that should throw
 *
 */
#define MSC_REQUIRE_THROWS( EXPR )                                                                                      \
    MSC_REQUIRE_EXCEPTION( ::std::exception, EXPR )

/**
 * @brief
 *     Inverts the test of another test.
 *
 * This macro is only really useful to test the test library itself.
 *
 * `MSC_REQUIRE_FAILURE(MSC_REQUIRE_EQ(42, foo()))` will fail unless `foo() != 42` which were of course better tested
 * using the straight-forward `MSC_REQUIRE_NE(42, foo())`.
 *
 * @warning
 *     The same concerns as for `MSC_REQUIRE_EXCEPTION` apply.
 *
 * @param EXPR
 *     expression that should be another assertion
 *
 */
#define MSC_REQUIRE_FAILURE( EXPR )                                                                                     \
    MSC_REQUIRE_EXCEPTION( ::dc7c12e5b72012bbc7e47c3e5436903c::failed_t, EXPR )

/**
 * @brief
 *     Tests that an expression is true.
 *
 * @param EXPR
 *     expression that should evaluate to `true`
 *
 */
#define MSC_REQUIRE( EXPR )                                                                                             \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require( (EXPR), __FILE__, __LINE__, #EXPR )

/**
 * @brief
 *     Tests that two expressions compare equal (`LHS == RHS`).
 *
 * @param LHS
 *     left-hand side expression of the comparison
 *
 * @param RHS
 *     right-hand side expression of the comparison
 *
 */
#define MSC_REQUIRE_EQ( LHS, RHS )                                                                                      \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_relop(                                                            \
        std::equal_to<>{}, (LHS), (RHS), __FILE__, __LINE__, #LHS, #RHS                                                 \
    )

/**
 * @brief
 *     Tests that two expressions compare unequal (`LHS != RHS`).
 *
 * @param LHS
 *     left-hand side expression of the comparison
 *
 * @param RHS
 *     right-hand side expression of the comparison
 *
 */
#define MSC_REQUIRE_NE( LHS, RHS )                                                                                      \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_relop(                                                            \
        std::not_equal_to<>{}, (LHS), (RHS), __FILE__, __LINE__, #LHS, #RHS                                             \
    )

/**
 * @brief
 *     Tests that one expression compares less than another (`LHS < RHS`).
 *
 * @param LHS
 *     left-hand side expression of the comparison
 *
 * @param RHS
 *     right-hand side expression of the comparison
 *
 */
#define MSC_REQUIRE_LT( LHS, RHS )                                                                                      \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_relop(                                                            \
        std::less<>{}, (LHS), (RHS), __FILE__, __LINE__, #LHS, #RHS                                                     \
    )

/**
 * @brief
 *     Tests that one expression compares less than or equal to another (`LHS <= RHS`).
 *
 * @param LHS
 *     left-hand side expression of the comparison
 *
 * @param RHS
 *     right-hand side expression of the comparison
 *
 */
#define MSC_REQUIRE_LE( LHS, RHS )                                                                                      \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_relop(                                                            \
        std::less_equal<>{}, (LHS), (RHS), __FILE__, __LINE__, #LHS, #RHS                                               \
    )

/**
 * @brief
 *     Tests that one expression compares greater than or equal to another (`LHS >= RHS`).
 *
 * @param LHS
 *     left-hand side expression of the comparison
 *
 * @param RHS
 *     right-hand side expression of the comparison
 *
 */
#define MSC_REQUIRE_GE( LHS, RHS )                                                                                      \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_relop(                                                            \
        std::greater_equal<>{}, (LHS), (RHS), __FILE__, __LINE__, #LHS, #RHS                                            \
    )

/**
 * @brief
 *     Tests that one expression compares greater than another (`LHS > RHS`).
 *
 * @param LHS
 *     left-hand side expression of the comparison
 *
 * @param RHS
 *     right-hand side expression of the comparison
 *
 */
#define MSC_REQUIRE_GT( LHS, RHS )                                                                                      \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_relop(                                                            \
        std::greater<>{}, (LHS), (RHS), __FILE__, __LINE__, #LHS, #RHS                                                  \
    )

/**
 * @brief
 *     Tests that one boolean expression implies another.
 *
 * Since many people seem to struggle with implications, here is a table of eternal truth:
 *
 * <table>
 *   <tr><th>`LHS`</th><th>`RHS`</th><th>result</th></tr>
 *   <tr><td>0</td><td>0</td><td></td>passes</tr>
 *   <tr><td>0</td><td>1</td><td></td>passes</tr>
 *   <tr><td>1</td><td>0</td><td></td>fails</tr>
 *   <tr><td>1</td><td>1</td><td></td>passes</tr>
 * </table>
 *
 * @param LHS
 *     left-hand side expression of the implication
 *
 * @param RHS
 *     right-hand side expression of the implication
 *
 */
#define MSC_REQUIRE_IMPLIES( LHS, RHS )                                                                                 \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_implies( (LHS), (RHS), __FILE__, __LINE__, #LHS, #RHS )

/**
 * @brief
 *     Tests that one floating-point expression is sufficiently close to another.
 *
 * This test will fail if the absolute difference between the values the two expressions evaluate to is greater than the
 * given tolerance.  If either expression evaluates to a non-finite value, the test will always fail.
 *
 * @param TOL
 *     tolerance (non-negative floating-point value)
 *
 * @param X
 *     expected value (floating-point expression)
 *
 * @param Y
 *     actual value (floating-point expression)
 *
 */
#define MSC_REQUIRE_CLOSE( TOL, X, Y )                                                                                  \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_close( (TOL), (X), (Y), __FILE__, __LINE__, #X, #Y )

/**
 * @brief
 *     Tests that an expression (which should evaluate to a character sequence) matches a given regular expression.
 *
 * @param PATTERN
 *     regular expression to be matched (string)
 *
 * @param TEXT
 *     text that should match the regular expression
 *
 */
#define MSC_REQUIRE_MATCH( PATTERN, TEXT )                                                                              \
    ::dc7c12e5b72012bbc7e47c3e5436903c::check_require_match(                                                            \
        (PATTERN), (TEXT), __FILE__, __LINE__, #PATTERN, #TEXT                                                          \
    )

/**
 * @brief
 *     Unconditionally skips the test with the provided message.
 *
 * @param MSG
 *     explanatin why the test was skipped
 *
 */
#define MSC_SKIP( MSG )                                                                                                 \
    ::dc7c12e5b72012bbc7e47c3e5436903c::skip_msg( __FILE__, __LINE__, (MSG) )

/**
 * @brief
 *     Conditionally skips the test.
 *
 * @param COND
 *     expression that should evaluate to a truth value indicating whether the test shall be skipped
 *
 */
#define MSC_SKIP_IF( COND )                                                                                             \
    (                                                                                                                   \
        static_cast<bool>( COND )                                                                                       \
            ? ::dc7c12e5b72012bbc7e47c3e5436903c::skip_msg( __FILE__, __LINE__, "Condition true: " #COND )              \
            : void()                                                                                                    \
    )

/**
 * @brief
 *     Conditionally skips the test.
 *
 * @param COND
 *     expression that should evaluate to a truth value indicating whether the test shall be exercised
 *
 */
#define MSC_SKIP_UNLESS( COND )                                                                                         \
    (                                                                                                                   \
        static_cast<bool>( COND )                                                                                       \
            ? void()                                                                                                    \
            : ::dc7c12e5b72012bbc7e47c3e5436903c::skip_msg( __FILE__, __LINE__, "Condition false: " #COND )             \
    )

/**
 * @brief
 *     Unconditionally fails the test with the provided message.
 *
 * @param MSG
 *     explanatin why the test failed
 *
 */
#define MSC_FAIL( MSG )                                                                                                 \
    ::dc7c12e5b72012bbc7e47c3e5436903c::fail_msg( __FILE__, __LINE__, (MSG) )

/**
 * @brief
 *     Unconditionally errors the test with the provided message.
 *
 * @param MSG
 *     explanatin why the test had an error
 *
 */
#define MSC_ERROR( MSG )                                                                                                \
    ::dc7c12e5b72012bbc7e47c3e5436903c::err_msg( __FILE__, __LINE__, (MSG) )

//! @cond FALSE
#ifndef MSC_PARSED_BY_DOXYGEN
#  define MSC_INCLUDED_FROM_UNITTEST_HXX
#  include "unittest.txx"
#  undef MSC_INCLUDED_FROM_UNITTEST_HXX
#endif

#ifdef MSC_RUN_ALL_UNIT_TESTS_IN_MAIN
int main(int argc, char** argv)
{
    return MSC_RUN_ALL_UNIT_TESTS(argc, argv);
}
#endif
//! @endcond

#endif  // !defined(MSC_UNITTEST_HXX)
