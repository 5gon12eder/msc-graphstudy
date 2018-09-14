#! /usr/bin/cmake -P
#! -*- coding:utf-8; mode:cmake; -*-

# Copyright (C) 2018 Moritz Klammler <moritz.klammler@student.kit.edu>
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

add_subdirectory(fonts)
add_subdirectory(pics)

set(typo_toolchain_env)
if(NOT "LUALATEX-NOTFOUND" STREQUAL "${LUALATEX_EXECUTABLE}")
    list(APPEND typo_toolchain_env "LUALATEX=${LUALATEX_EXECUTABLE}")
endif()
if(NOT "BIBTEX-NOTFOUND" STREQUAL "${BIBTEX_EXECUTABLE}")
    list(APPEND typo_toolchain_env "BIBTEX=${BIBTEX_EXECUTABLE}")
endif()
if(NOT "BIBER-NOTFOUND" STREQUAL "${BIBER_EXECUTABLE}")
    list(APPEND typo_toolchain_env "BIBER=${BIBER_EXECUTABLE}")
endif()
if(NOT "MAKEINDEX-NOTFOUND" STREQUAL "${MAKEINDEX_EXECUTABLE}")
    list(APPEND typo_toolchain_env "MAKEINDEX=${MAKEINDEX_EXECUTABLE}")
endif()

add_custom_target(
    report-clean
    COMMAND "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/utils/tex-cleanup.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

set(report_dependencies
    graphstudy.tex  # This must be the first item on the list
    abbreviations.tex
    eval-cross-valid.tex
    eval-puncture.tex
    fonts/Roboto-Bold.ttf
    fonts/Roboto-Regular.ttf
    graphstudy.cls
    literature.bib
    logos/algo.pdf
    logos/kit.pdf
    notation.tex
    preamble.tex
    titlepage.tex
)

foreach(item ${report_picfiles})
    list(APPEND report_dependencies "pics/${item}")
endforeach(item)
foreach(item ${report_picfiles_dynamic})
    set_source_files_properties("pics/${item}" PROPERTIES GENERATED TRUE)
endforeach(item)

set_source_files_properties("fonts/Roboto-Regular.ttf" "fonts/Roboto-Bold.ttf" PROPERTIES GENERATED TRUE)

add_custom_command(
    OUTPUT graphstudy.pdf
    COMMAND "${CMAKE_COMMAND}" -E env ${typo_toolchain_env}
            "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/typeset.py"
            "--srcdir=${CMAKE_CURRENT_SOURCE_DIR}"
            "--bindir=${CMAKE_CURRENT_BINARY_DIR}"
            "--tex=LUALATEX"
            "--bib=BIBER"
            "--idx=MAKEINDEX"
            "--jobname=graphstudy"
            "--max-print-line=1024"
            -- ${report_dependencies}
    VERBATIM
    DEPENDS ${report_dependencies}
)

add_custom_target(report DEPENDS graphstudy.pdf)
add_dependencies(report report-fonts report-pics)

add_custom_command(
    OUTPUT eval-cross-valid.tex
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/format-confusion.py"
            -o "${CMAKE_CURRENT_BINARY_DIR}/eval-cross-valid.tex"
            -t NN_FORWARD
            "${PROJECT_BINARY_DIR}/eval/cross-valid.json"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS eval-cross-valid.stamp
            "${PROJECT_SOURCE_DIR}/utils/format-confusion.py"
)

add_custom_command(
    OUTPUT eval-puncture.tex
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/format-puncture.py"
            -d "${PROJECT_BINARY_DIR}/eval/"
            -o "${CMAKE_CURRENT_BINARY_DIR}/eval-puncture.tex"
            -p "${PROJECT_BINARY_DIR}/eval/puncture-properties.txt"
            -t NN_FORWARD --sort-excl
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS eval-puncture.stamp
            "${PROJECT_BINARY_DIR}/eval/puncture-properties.txt"
            "${PROJECT_SOURCE_DIR}/utils/format-puncture.py"
)

add_custom_command(
    OUTPUT eval-cross-valid.stamp
    COMMAND "${CMAKE_COMMAND}" -E touch eval-cross-valid.stamp
    VERBATIM
    COMMENT "Faking stamp file '${CMAKE_CURRENT_BINARY_DIR}/eval-cross-valid.stamp'"
)

add_custom_command(
    OUTPUT eval-puncture.stamp
    COMMAND "${CMAKE_COMMAND}" -E touch eval-puncture.stamp
    VERBATIM
    COMMENT "Faking stamp file '${CMAKE_CURRENT_BINARY_DIR}/eval-puncture.stamp'"
)
