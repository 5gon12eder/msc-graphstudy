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
    slides-gd18-clean
    COMMAND "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/utils/tex-cleanup.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

set(slides_gd18_dependencies
    graphstudy.tex  # This must be the first item on the list
    preamble.tex
    literature.bib
    # Evaluation results
    eval-competing-metrics.tex
    eval-cross-valid.tex
    eval-puncture.tex
    nn-info.tex
    disco.tex
)

foreach(item ${slides_gd18_picfiles})
    list(APPEND slides_gd18_dependencies "pics/${item}")
endforeach(item)
foreach(item ${slides_gd18_picfiles_dynamic})
    set_source_files_properties("pics/${item}" PROPERTIES GENERATED TRUE)
endforeach(item)

if(EXISTS "${MSC_KIT_BEAMER_ZIP}")
    add_custom_command(
        OUTPUT kit-beamer.stamp
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${MSC_KIT_BEAMER_ZIP}" --format=zip
        COMMAND "${CMAKE_COMMAND}" -E touch kit-beamer.stamp
        VERBATIM
        DEPENDS "${MSC_KIT_BEAMER_ZIP}"
    )
else()
    add_custom_command(
        OUTPUT kit-beamer.stamp
        COMMAND "${CMAKE_COMMAND}" -E touch kit-beamer.stamp
        VERBATIM
    )
endif()

add_custom_command(
    OUTPUT graphstudy.pdf
    COMMAND "${CMAKE_COMMAND}" -E env ${typo_toolchain_env}
            "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/typeset.py"
            "--srcdir=${CMAKE_CURRENT_SOURCE_DIR}"
            "--bindir=${CMAKE_CURRENT_BINARY_DIR}"
            "--tex=LUALATEX"
            "--bib=BIBER"
            "--jobname=graphstudy"
            "--max-print-line=1024"
            -- ${slides_gd18_dependencies}
    VERBATIM
    DEPENDS ${slides_gd18_dependencies} kit-beamer.stamp
)

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
    OUTPUT eval-competing-metrics.tex
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/format-competing-metrics.py"
            -p "${PROJECT_BINARY_DIR}/eval/xeval-*.json"
            -o "${CMAKE_CURRENT_BINARY_DIR}/eval-competing-metrics.tex"
            -r "${CMAKE_CURRENT_SOURCE_DIR}/rename-tests.txt"
            -t NN_FORWARD -t STRESS_FIT_SCALE -t HUANG
            "${PROJECT_BINARY_DIR}/eval/cross-valid.json"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS eval-cross-valid.stamp rename-tests.txt
            "${PROJECT_SOURCE_DIR}/utils/format-competing-metrics.py"
)

add_custom_command(
    OUTPUT eval-puncture.tex
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/format-puncture.py"
            -d "${PROJECT_BINARY_DIR}/eval/"
            -o "${CMAKE_CURRENT_BINARY_DIR}/eval-puncture.tex"
            -r "${CMAKE_CURRENT_SOURCE_DIR}/rename-properties.txt"
            -p "${PROJECT_BINARY_DIR}/eval/puncture-properties.txt"
            -t NN_FORWARD --sort-incl
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS eval-puncture.stamp rename-properties.txt
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

add_custom_command(
    OUTPUT nn-info.tex
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/format-nn-info.py"
            -o nn-info.tex --corpus=nn-dump/corpus.json
            --shared-info=nn-dump/shared.txt --total-info=nn-dump/total.txt
    VERBATIM
    DEPENDS nn-dump.stamp
            "${PROJECT_SOURCE_DIR}/utils/format-nn-info.py"
)

add_custom_command(
    OUTPUT nn-dump.stamp
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/nn-dump/"
    COMMAND "${Python3_EXECUTABLE}" -m driver.model --model --corpus
            "--debugdir=${CMAKE_CURRENT_BINARY_DIR}/nn-dump/"
            "--configdir=${MSC_CONFIG_DIR}" "--datadir=${MSC_DATA_DIR}" "--bindir=${PROJECT_BINARY_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_CURRENT_BINARY_DIR}/nn-dump.stamp"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS omnibus
)

add_custom_command(
    OUTPUT disco.tex
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/format-disco.py"
            -n NN_FORWARD=discmod -n STRESS_FIT_SCALE=stress -n HUANG=comb
            -o "${CMAKE_CURRENT_BINARY_DIR}/disco.tex" "${CMAKE_CURRENT_BINARY_DIR}/disco.txt"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS disco.txt "${PROJECT_SOURCE_DIR}/utils/format-disco.py"
)

add_custom_command(
    OUTPUT disco.txt
    COMMAND "${Python3_EXECUTABLE}" -m driver.compare
            "--configdir=${MSC_CONFIG_DIR}" "--datadir=${MSC_DATA_DIR}" "--bindir=${PROJECT_BINARY_DIR}"
            --fingerprint
            -m NN_FORWARD -m STRESS_FIT_SCALE -m HUANG
            -o "${CMAKE_CURRENT_BINARY_DIR}/disco.txt"
            "@${CMAKE_CURRENT_SOURCE_DIR}/disco-couples.txt"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS omnibus disco-couples.txt
)

add_custom_target(slides-gd18 DEPENDS graphstudy.pdf)
add_dependencies(slides-gd18 slides-gd18-pics)
