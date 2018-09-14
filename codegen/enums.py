#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2018 Karlsruhe Institute of Technology
# Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
#
# This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

from sys import hexversion as PYTHON_HEXVERSION
if PYTHON_HEXVERSION < 0x30602f0:
    raise Exception("Your Python interpreter is too old; Python 3.6 or newer is required for this program to run")

import argparse
import contextlib
import json
import os
import sys
import textwrap

NOEDIT_BANNER = '''
/* ------------------------------------------------------------------------------------------------------------------ */
/*                                THIS IS A GENERATED FILE -- DO NOT EDIT IT MANUALLY!                                */
/* ------------------------------------------------------------------------------------------------------------------ */
'''

def main():
    ap = argparse.ArgumentParser(description="Creates boilerplate C++ code for enumerations.")
    ap.add_argument('infile', metavar='FILE', help="read Json definitions from FILE")
    ns = ap.parse_args()
    log("Reading definition file {!r} ...".format(ns.infile))
    with open(ns.infile, 'r') as istr:
        lines = istr.readlines()
    # Skip comments at top of file (JSON sucks)
    while lines:
        line = lines[0].strip()
        if not line: del lines[0]
        elif line.startswith('//'): del lines[0]
        elif line.startswith('/*') and line.endswith('*/'): del lines[0]
        else: break
    definitions = json.loads(''.join(lines))
    maindir = os.path.join('src', 'common', 'enums')
    testdir = os.path.join('test', 'unit', 'enums')
    log("Creating directory {!r} ...".format(maindir))
    os.makedirs(maindir, exist_ok=True)
    log("Creating directory {!r} ...".format(testdir))
    os.makedirs(testdir, exist_ok=True)
    for (name, definition) in definitions.items():
        headerfile = os.path.join(maindir, name + '.hxx')
        sourcefile = os.path.join(maindir, name + '.cxx')
        testfile = os.path.join(testdir, name + '_test.txx')
        log("Writing header file {!r} ...".format(headerfile))
        with open(headerfile, 'w') as ostr:
            with contextlib.redirect_stdout(ostr):
                print_header(name, definition)
        log("Writing source file {!r} ...".format(sourcefile))
        with open(sourcefile, 'w') as ostr:
            with contextlib.redirect_stdout(ostr):
                print_source(name, definition)
        log("Writing unit test file {!r} ...".format(testfile))
        with open(testfile, 'w') as ostr:
            with contextlib.redirect_stdout(ostr):
                print_test(name, definition)

def print_header(name, definition):
    guardname = 'MSC_' + name.upper() + '_HXX'
    docstring_text_wrapper = make_text_wrapper('     * ')
    docstring_item_wrapper = make_text_wrapper('         * ')
    print('/* -*- coding:utf-8; mode:c++; -*- */')
    print(NOEDIT_BANNER)
    print('/**')
    print(' * @file {:s}'.format(name + '.hxx'))
    print(' *')
    print(' * @brief')
    print(' *     ' + definition['help'])
    print(' *')
    print(' */')
    print()
    print('#ifndef {:s}'.format(guardname))
    print('#define {:s}'.format(guardname))
    print()
    for header in {'array', 'string_view'}:
        print('#include <{:s}>'.format(header))
    print()
    print('namespace msc')
    print('{')
    print()
    print('    /** @brief {:s}  */'.format(definition['help']))
    print('    enum class {:s}'.format(name))
    print('    {')
    maxname = max(map(len, definition['values'].keys()))
    for (i, key) in enumerate(sorted(definition['values'].keys())):
        helptext = definition['values'][key]
        if helptext is not None:
            print('        {:{width}s} = {:3d},  ///< {:s}'.format(key, 1 + i, helptext, width=maxname))
        else:
            print('        {:{width}s} = {:3d},'.format(key, 1 + i, width=maxname))
    print('    };')
    print()
    print('    /**')
    print('     * @brief')
    print('     *     Returns a constant reference to a statically allocated array of all declared enumerators.')
    print('     *')
    print('     * @returns')
    print('     *     array with all declared enumerators')
    print('     *')
    print('     */')
    print('    const std::array<{0:s}, {1:d}>& all_{0:s}() noexcept;'.format(name, len(definition['values'])))
    print()
    print('    /**')
    print('     * @brief')
    print('     *     Returns the canonical name of an enumerator.')
    print('     *')
    print(docstring_text_wrapper.fill(
        "The returned name will consist of one or more lower-case words joined together with a dash, for example"
        + " `beta` or `alpha-omega`.  The character array the returned `std::string_view` refers to is statically"
        + " allocated and guaranteed to be NUL terminated."
    ))
    print('     *')
    print('     * @param item')
    print('     *     enumerator to get the name for')
    print('     *')
    print('     * @returns')
    print('     *     canonical enumerator name')
    print('     *')
    print('     * @throws std::invalid_argument')
    print('     *     if the constant is not a declared enumerator')
    print('     *')
    print('     */')
    print('    std::string_view name({:s} item);'.format(name))
    print()
    print('    /**')
    print('     * @brief')
    print('     *     Returns the enumerator for a name.')
    print('     *')
    print(docstring_text_wrapper.fill(
        "The conversion is case-insensitive and ignores leading and trailing white-space. "
        + " In addition to converting names, if `name` parses as a decimal integer (no leading signum allowed) then"
        + " the enumerator with that numerical value is returned, if it is declared."
    ))
    print('     *')
    print('     * @param name')
    print('     *     name to get the enumeration constant for')
    print('     *')
    print('     * @returns')
    print('     *     enumeration constant')
    print('     *')
    print('     * @throws std::invalid_argument')
    print('     *     if the name does not refer to a declared enumerator')
    print('     *')
    print('     */')
    print('    {0:s} value_of_{0:s}(std::string_view name);'.format(name))
    print()
    print('}  // namespace msc')
    print()
    print('#endif  // !defined({:s})'.format(guardname))

def print_source(name, definition):
    arraytype = 'std::array<{:s}, {:d}>'.format(name, len(definition['values']))
    print('/* -*- coding:utf-8; mode:c++; -*- */')
    print(NOEDIT_BANNER)
    print('#ifdef HAVE_CONFIG_H')
    print('#  include <config.h>')
    print('#endif')
    print()
    print('#include "enums/{:s}.hxx"'.format(name))
    print()
    for header in {'string'}:
        print('#include <{:s}>'.format(header))
    print()
    for header in {'useful.hxx'}:
        print('#include "{:s}"'.format(header))
    print()
    print('namespace msc')
    print('{')
    print()
    print('    namespace /*anonymous*/')
    print('    {')
    print()
    print('        constexpr {:s} all_values = '.format(arraytype) + '{{')
    for key in sorted(definition['values'].keys()):
        print('            {:s}::{:s},'.format(name, key))
    print('        }};')
    print()
    print('    }  // namespace /*anonymous*/')
    print()
    print('    const {:s}& all_{:s}() noexcept'.format(arraytype, name))
    print('    {')
    print('        return all_values;')
    print('    }')
    print()
    print('    std::string_view name(const {:s} item)'.format(name))
    print('    {')
    print('        switch (item) {')
    for key in sorted(definition['values'].keys()):
        print('            case {:s}::{:s}:'.format(name, key))
        print('                return "{:s}";'.format(dashy(key)))
    print('        }')
    print('        reject_invalid_enumeration(item, "msc::{:s}");'.format(name))
    print('    }')
    print()
    print('    {0:s} value_of_{0:s}(std::string_view name)'.format(name))
    print('    {')
    print('        const auto canonical = normalize_constant_name(name);');
    print('        if (const auto raw = parse_decimal_number(canonical)) {')
    print('            if ((*raw <= 0) || (*raw > {:d})) {{'.format(len(definition['values'])))
    print('                reject_invalid_enumeration(*raw, "msc::{:s}");'.format(name))
    print('            }')
    print('            return static_cast<{:s}>(*raw);'.format(name))
    for key in sorted(definition['values'].keys()):
        print('        }} else if (canonical == "{:s}") {{'.format(dashy(key)))
        print('            return {:s}::{:s};'.format(name, key))
    print('        } else {')
    print('            reject_invalid_enumeration(name, "msc::{:s}");'.format(name))
    print('        }')
    print('    }')
    print()
    print('}  // namespace msc')

def print_test(name, definition):
    print('/* -*- coding:utf-8; mode:c++; -*- */')
    print(NOEDIT_BANNER)
    print('// This file is supposed to be `#include`d into the actual unit test TU rather than compiled directly.')
    print()
    for header in {'algorithm', 'functional', 'set', 'utility'}:
        print('#include <{:s}>'.format(header))
    print()
    print('namespace /*anonymous*/')
    print('{')
    print()
    print('    MSC_AUTO_TEST_CASE(all_sorted_unique_and_not_zero)')
    print('    {')
    print('        const auto zero = msc::' + name + '{};')
    print('        const auto themall = msc::all_{:s}();'.format(name))
    print('        MSC_REQUIRE(std::adjacent_find(std::begin(themall), std::end(themall), std::greater_equal<>{})',
                   '== std::end(themall));')
    print('        MSC_REQUIRE(std::find(std::begin(themall), std::end(themall), zero) == std::end(themall));')
    print('    }')
    print()
    print('    MSC_AUTO_TEST_CASE(name_returns_declared_name_but_with_dashes)')
    print('    {')
    print('        using namespace std::string_view_literals;')
    for key in sorted(definition['values'].keys()):
        print('        MSC_REQUIRE_EQ("{:s}"sv, name(msc::{:s}::{:s}));'.format(dashy(key), name, key))
    print('    }')
    print()
    print('    MSC_AUTO_TEST_CASE(names_are_unique)')
    print('    {')
    print('        auto thenames = std::set<std::string_view>{};')
    print('        for (const auto item : msc::all_{:s}()) {{'.format(name))
    print('            thenames.insert(name(item));')
    print('        }')
    print('        MSC_REQUIRE_EQ(msc::all_{:s}().size(), thenames.size());'.format(name))
    print('    }')
    print()
    print('    MSC_AUTO_TEST_CASE(name_throws_exception_for_undeclared_enumerators)')
    print('    {')
    print('        const auto zero = msc::' + name + '{};')
    print('        MSC_REQUIRE_EXCEPTION(std::invalid_argument, name(zero));')
    print('    }')
    print()
    print('    MSC_AUTO_TEST_CASE(value_of_returns_expected_constant_is_case_insensitive_and_ignores_space)')
    print('    {')
    for key in sorted(definition['values'].keys()):
        ec = 'msc::{:s}::{:s}'.format(name, key)
        canonical = dashy(key)
        for text in [canonical, canonical.upper(), ' ' + canonical.title() + ' ']:
            print('        MSC_REQUIRE_EQ({:s}, msc::value_of_{:s}("{:s}"));'.format(ec, name, text))
    print('    }')
    print()
    print('    MSC_AUTO_TEST_CASE(value_of_decimal_integer_returns_constant_and_ignores_space)')
    print('    {')
    for (i, key) in enumerate(sorted(definition['values'].keys())):
        ec = 'msc::{:s}::{:s}'.format(name, key)
        print('        MSC_REQUIRE_EQ({:s}, msc::value_of_{:s}("  {:d}  \\t"));'.format(ec, name, i + 1))
    print('    }')
    print()
    print('    MSC_AUTO_TEST_CASE(value_of_throws_exception_for_invalid_input)')
    print('    {')
    for bad in ['', r'\t\v\n\r', 'Cat', 'dog-food', '-42', '+1', '0', '00', str(len(definition['values']) + 1)]:
        print('        MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::value_of_{:s}("{:s}"));'.format(name, bad))
    print('    }')
    print()
    print('    MSC_AUTO_TEST_CASE(value_of_textual_round_trip)')
    print('    {')
    print('        for (const auto item : msc::all_{:s}()) {{'.format(name))
    print('            MSC_REQUIRE_EQ(item, msc::value_of_{:s}(name(item)));'.format(name))
    print('        }')
    print('    }')
    print()
    print('    MSC_AUTO_TEST_CASE(value_of_numeric_round_trip)')
    print('    {')
    print('        for (const auto item : msc::all_{:s}()) {{'.format(name))
    print('            const auto raw = static_cast<int>(item);')
    print('            MSC_REQUIRE_EQ(item, msc::value_of_{:s}(std::to_string(raw)));'.format(name))
    print('        }')
    print('    }')
    print()
    print('}  // namespace /*anonymous*/')

def dashy(name):
    return name.replace('_', '-')

def make_text_wrapper(indent):
    return textwrap.TextWrapper(
        width=120,
        initial_indent=indent,
        subsequent_indent=indent,
        fix_sentence_endings=True,
        break_long_words=False,
        break_on_hyphens=False,
    )

def log(message):
    print(message, file=sys.stderr)

if __name__ == '__main__':
    main()
