#! /usr/bin/cmake -P
#! -*- coding:utf-8; mode:cmake; -*-

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

cmake_minimum_required(VERSION 3.9.1)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED TRUE)

project(graphstudy CXX)

set(MSC_CONFIG_DIR "${PROJECT_SOURCE_DIR}/config/" CACHE PATH "Driver configuration directory")
set(MSC_DATA_DIR   "${PROJECT_BINARY_DIR}/data/"   CACHE PATH "Driver persistent storage directory")

set(MSC_PACKAGE_YEAR      "2018"                                          CACHE INTERNAL "Package copyright year(s)")
set(MSC_PACKAGE_AUTHOR    "Karlsruhe Institute of Technology"             CACHE INTERNAL "Package copyright author(s)")
set(MSC_PACKAGE_BUGREPORT "moritz.klammler@alumni.kit.edu"                CACHE INTERNAL "Package bug report address")
set(MSC_PACKAGE_URL       "https://github.com/5gon12eder/msc-graphstudy/" CACHE INTERNAL "Package web site")

include(CTest)

set(EXTRA_CMAKE_CXX_FLAGS CACHE STRING "Additional CMAKE_CXX_FLAGS used by this project but not its dependencies")

# Save these values early so not to pollute the OGDF configuration with any custom flags we need for our code only.
# Unfortunately, the OGDF code is not statically memaccess clean so we have to disable this diagnostic but we do it
# before the user-specified flags to allow them to re-enable that warning if desired.

set(OGDF_CMAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER}")
set(OGDF_CMAKE_CXX_FLAGS "-Wno-error=class-memaccess" "${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DOGDF_DEBUG")

set(TEST_GRAPH_FILE "${PROJECT_SOURCE_DIR}/test/data/cube-graph.xml" CACHE INTERNAL "Test graph file")
set(TEST_LAYOUT_FILE "${PROJECT_SOURCE_DIR}/test/data/cube-layout-normal.xml" CACHE INTERNAL "Test layout file")
set(TEST_RAWDATA_FILE "${PROJECT_SOURCE_DIR}/test/data/events.txt" CACHE INTERNAL "Test raw data file")

add_definitions("-D_POSIX_C_SOURCE=200112L")  # POXIS symbols in GNU C
add_definitions("-D_USE_MATH_DEFINES")        # Math constants in VS

include(CheckSymbolExists)
include(CheckCXXSourceCompiles)

# The vanilla implementations of `CHECK_SYMBOL_EXISTS` and `CHECK_CXX_SOURCE_COMPILES` provided by CMake do not set the
# output variable if the test has a negative result which is most annoying when trying to do arithmetic with the results
# or substitute them into header files.  Therefore, we wrap them and make sure the output variable will always be
# defined either to 0 or to 1.

macro(MSC_CHECK_SYMBOL_EXISTS symbol headers variable)
    check_symbol_exists("${symbol}" "${headers}" "${variable}")
    if(${variable})
        set(${variable} 1)
    else()
        set(${variable} 0)
    endif()
endmacro()

macro(MSC_CHECK_CXX_SOURCE_COMPILES code variable)
    check_cxx_source_compiles("${code}" "${variable}")
    if(${variable})
        set(${variable} 1)
    else()
        set(${variable} 0)
    endif()
endmacro()

msc_check_symbol_exists(TIOCGWINSZ     "sys/ioctl.h"         HAVE_LINUX_TIOCGWINSZ    )
msc_check_symbol_exists(M_E            "cmath"               HAVE_MATH_E              )
msc_check_symbol_exists(M_PI           "cmath"               HAVE_MATH_PI             )
msc_check_symbol_exists(M_SQRT2        "cmath"               HAVE_MATH_SQRT2          )
msc_check_symbol_exists(close          "unistd.h"            HAVE_POSIX_CLOSE         )
msc_check_symbol_exists(dup            "unistd.h"            HAVE_POSIX_DUP           )
msc_check_symbol_exists(dup2           "unistd.h"            HAVE_POSIX_DUP2          )
msc_check_symbol_exists(fileno         "stdio.h"             HAVE_POSIX_FILENO        )
msc_check_symbol_exists(getenv         "stdlib.h"            HAVE_POSIX_GETENV        )
msc_check_symbol_exists(getrlimit      "sys/resource.h"      HAVE_POSIX_GETRLIMIT     )
msc_check_symbol_exists(ioctl          "stropts.h"           HAVE_POSIX_IOCTL         )
msc_check_symbol_exists(isatty         "unistd.h"            HAVE_POSIX_ISATTY        )
msc_check_symbol_exists(open           "sys/stat.h;fcntl.h"  HAVE_POSIX_OPEN          )
msc_check_symbol_exists(O_APPEND       "fcntl.h"             HAVE_POSIX_O_APPEND      )
msc_check_symbol_exists(O_CLOEXEC      "fcntl.h"             HAVE_POSIX_O_CLOEXEC     )
msc_check_symbol_exists(O_CREAT        "fcntl.h"             HAVE_POSIX_O_CREAT       )
msc_check_symbol_exists(O_DIRECTORY    "fcntl.h"             HAVE_POSIX_O_DIRECTORY   )
msc_check_symbol_exists(O_EXCL         "fcntl.h"             HAVE_POSIX_O_EXCL        )
msc_check_symbol_exists(O_NOFOLLOW     "fcntl.h"             HAVE_POSIX_O_NOFOLLOW    )
msc_check_symbol_exists(O_PATH         "fcntl.h"             HAVE_POSIX_O_PATH        )
msc_check_symbol_exists(O_RDONLY       "fcntl.h"             HAVE_POSIX_O_RDONLY      )
msc_check_symbol_exists(O_RDWR         "fcntl.h"             HAVE_POSIX_O_RDWR        )
msc_check_symbol_exists(O_TRUNC        "fcntl.h"             HAVE_POSIX_O_TRUNC       )
msc_check_symbol_exists(O_WRONLY       "fcntl.h"             HAVE_POSIX_O_WRONLY      )
msc_check_symbol_exists(setenv         "stdlib.h"            HAVE_POSIX_SETENV        )
msc_check_symbol_exists(setrlimit      "sys/resource.h"      HAVE_POSIX_SETRLIMIT     )
msc_check_symbol_exists(STDERR_FILENO  "unistd.h"            HAVE_POSIX_STDERR_FILENO )
msc_check_symbol_exists(STDIN_FILENO   "unistd.h"            HAVE_POSIX_STDIN_FILENO  )
msc_check_symbol_exists(STDOUT_FILENO  "unistd.h"            HAVE_POSIX_STDOUT_FILENO )
msc_check_symbol_exists(unsetenv       "stdlib.h"            HAVE_POSIX_UNSETENV      )
msc_check_symbol_exists(write          "unistd.h"            HAVE_POSIX_WRITE         )

function(msc_conjunction outvar)
    set(temp 1)
    foreach(arg ${ARGN})
        if(NOT "${arg}")
            set(temp 0)
            break()
        endif()
    endforeach(arg)
    set("${outvar}" ${temp} PARENT_SCOPE)
endfunction()

msc_conjunction(HAVE_MATH_CONSTANTS ${HAVE_MATH_PI} ${HAVE_MATH_E} ${HAVE_MATH_SQRT2})
msc_conjunction(
    HAVE_POSIX_OPEN_FLAGS
    ${HAVE_POSIX_O_PATH}
    ${HAVE_POSIX_O_RDONLY}
    ${HAVE_POSIX_O_WRONLY}
    ${HAVE_POSIX_O_RDWR}
    ${HAVE_POSIX_O_APPEND}
    ${HAVE_POSIX_O_CLOEXEC}
    ${HAVE_POSIX_O_CREAT}
    ${HAVE_POSIX_O_DIRECTORY}
    ${HAVE_POSIX_O_EXCL}
    ${HAVE_POSIX_O_NOFOLLOW}
    ${HAVE_POSIX_O_TRUNC}
)

msc_check_cxx_source_compiles(
    "#include <cstdlib>\nextern \"C\" char **environ;\nint main() { return environ == nullptr; }\n"
    HAVE_POSIX_ENVIRON
)

msc_check_cxx_source_compiles(
    "#include <sys/ioctl.h>\nint main() { return sizeof(winsize) == 0; }\n"
    HAVE_LINUX_WINSIZE
)

configure_file("${PROJECT_SOURCE_DIR}/config.h.in"     "${PROJECT_BINARY_DIR}/config.h"     @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/doxygen.conf.in" "${PROJECT_BINARY_DIR}/doxygen.conf" @ONLY)

include_directories("${PROJECT_BINARY_DIR}")

add_definitions("-DHAVE_CONFIG_H")
add_definitions("-DMSC_FILESYSTEM_IMPL=MSC_FILESYSTEM_IMPL_BOOST")

include(ogdf.cmake)

find_package(Boost 1.66 REQUIRED COMPONENTS filesystem iostreams program_options system)

find_package(Threads)

# TODO: Once we have CMake 3.12, we can get rid of this nonsense and use FindPython3 instead.
# https://cmake.org/cmake/help/latest/module/FindPython3.html
find_package(PythonInterp)
if(${PYTHONINTERP_FOUND})
    if("${PYTHON_VERSION_STRING}" VERSION_LESS "3.6.2")
        message(FATAL_ERROR "Python interpreter is too old (Python 3.6.2 or newer is needed)")
    endif()
    set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}" CACHE INTERNAL "Python3 Interpreter Executable")
else()
    message(FATAL_ERROR "No suitable Python 3 interpreter found")
endif()

# Tools needed for typesetting the written thesis:
find_program(AWK_EXECUTABLE       gawk      DOC "GNU AWK executable (https://www.gnu.org/software/gawk/)")
find_program(BIBER_EXECUTABLE     biber     DOC "Biber executable (https://www.ctan.org/pkg/biber/)")
find_program(BIBTEX_EXECUTABLE    bibtex    DOC "BibTeX executable (https://www.ctan.org/pkg/bibtex/)")
find_program(GNUPLOT_EXECUTABLE   gnuplot   DOC "Gnuplot executable (http://gnuplot.info/)")
find_program(LUALATEX_EXECUTABLE  lualatex  DOC "LuaLaTeX executable (https://www.ctan.org/pkg/luatex/)")
find_program(MAKEINDEX_EXECUTABLE makeindex DOC "Makeindex executable (https://www.ctan.org/pkg/makeindex/)")
find_program(PDFTOPS_EXECUTABLE   pdftops   DOC "PDF to PostScript converter executable compatible to 'pdftops'")
find_program(ZIP_EXECUTABLE       zip       DOC "Info-ZIP compatible ZIP archiver executable (http://www.info-zip.org)")

add_subdirectory(utils)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(eval)
add_subdirectory(report)
add_subdirectory(paper)
add_subdirectory(slides_2018-04-24_kit)
add_subdirectory(slides_2018-09-26_gd18)

include(driver-tests.cmake)

find_package(Doxygen)

if(DOXYGEN_FOUND)
    add_custom_target(
        apidoc
        COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_BINARY_DIR}/doxygen.conf"
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
        DEPENDS common-codegen
        COMMENT "Generating API documentation with Doxygen ..."
        VERBATIM
    )
endif(DOXYGEN_FOUND)

add_custom_target(
    benchmark
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/benchmarks/macro-driver.py"
            "--manifest=${PROJECT_BINARY_DIR}/test/perf/macro/Manifest.json"
            "--history=${PROJECT_BINARY_DIR}/benchmark-history-macro.db"
            "--warmup=1"
            "--quantile=0.50"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/benchmarks/micro-driver.py"
            "--manifest=${PROJECT_BINARY_DIR}/test/perf/micro/Manifest.json"
            "--history=${PROJECT_BINARY_DIR}/benchmark-history-micro.db"
            "--warmup=1"
            "--quantile=0.50"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    VERBATIM
    DEPENDS omnibus
)

add_custom_target(
    deploy
    COMMAND "${Python3_EXECUTABLE}" -m driver.deploy --known-bad --all
            "--configdir=${MSC_CONFIG_DIR}" "--datadir=${MSC_DATA_DIR}" "--bindir=${PROJECT_BINARY_DIR}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS omnibus
)

add_custom_target(
    integrity
    COMMAND "${Python3_EXECUTABLE}" -m driver.integrity "--journal=${MSC_DATA_DIR}/integrity.log"
            "--configdir=${MSC_CONFIG_DIR}" "--datadir=${MSC_DATA_DIR}" "--bindir=${PROJECT_BINARY_DIR}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS omnibus
)

add_custom_target(
    integrity-fix
    COMMAND "${Python3_EXECUTABLE}" -m driver.integrity "--journal=${MSC_DATA_DIR}/integrity.log" --repair=10
            "--configdir=${MSC_CONFIG_DIR}" "--datadir=${MSC_DATA_DIR}" "--bindir=${PROJECT_BINARY_DIR}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS omnibus
)

add_custom_target(
    httpd
    COMMAND "${Python3_EXECUTABLE}" -m driver.httpd --fork "--pid-file=${PROJECT_BINARY_DIR}/.httpd.pid"
            "--configdir=${MSC_CONFIG_DIR}" "--datadir=${MSC_DATA_DIR}" "--bindir=${PROJECT_BINARY_DIR}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS omnibus
)

add_custom_target(
    omnibus
    COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    VERBATIM
)

add_custom_target(
    maintainer-everything
    COMMAND "${CMAKE_COMMAND}" --build .
    COMMAND "${CMAKE_COMMAND}" --build . --target apidoc
    COMMAND "${CMAKE_COMMAND}" -E env --unset=CTEST_PARALLEL_LEVEL
            "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}"
            "${CMAKE_CTEST_COMMAND}" -T Test
    COMMAND "${CMAKE_COMMAND}" --build . --target benchmark
    COMMAND "${CMAKE_COMMAND}" --build . --target deploy
    COMMAND "${CMAKE_COMMAND}" --build . --target integrity
    COMMAND "${CMAKE_COMMAND}" --build . --target eval
    COMMAND "${CMAKE_COMMAND}" --build . --target report
    COMMAND "${CMAKE_COMMAND}" --build . --target paper
    COMMAND "${CMAKE_COMMAND}" --build . --target paper-clean
    COMMAND "${CMAKE_COMMAND}" --build . --target paper-pubar
    COMMAND "${CMAKE_COMMAND}" --build . --target slides-kit
    COMMAND "${CMAKE_COMMAND}" --build . --target slides-gd18
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    VERBATIM
)
