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

add_custom_target(
    paper-clean
    COMMAND "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/utils/tex-cleanup.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

set(paper_dependencies
    # main (must be the first item)
    graphstudy.tex
    # sections
    sec-intro.tex
    sec-relwork.tex
    sec-syndromes.tex
    sec-featex.tex
    sec-model.tex
    sec-data.tex
    sec-eval.tex
    sec-conc.tex
    appendix.tex
    # data
    eval-competing-metrics.tex
    eval-cross-valid.tex
    eval-huang-weights.tex
    eval-puncture.tex
    nn-info.tex
    # resources
    literature-bibtex.bib
    # style
    preamble.tex
    llncs.cls
    splncs04.bst
    tango-colors.sty
)

foreach(item ${paper_picfiles})
    list(APPEND paper_dependencies "pics/${item}")
endforeach(item)
foreach(item ${paper_picfiles_dynamic})
    set_source_files_properties("pics/${item}" PROPERTIES GENERATED TRUE)
endforeach(item)

set(pubfiles graphstudy-gd18.pdf graphstudy-arxiv.pdf graphstudy-gd18.tex graphstudy-arxiv.tex ${paper_dependencies})

add_custom_command(
    OUTPUT llncs.cls splncs04.bst
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/download.py"
            "--checksum=sha256:f0127c19b212cdd76e3f3d5bb363f0e22c58cba6fe600a81c05b101035f4fe5c"
            "--extract=llncs.cls"
            "--extract=splncs04.bst"
            "ftp://ftp.springernature.com/cs-proceeding/llncs/llncs2e.zip"
    VERBATIM
)

foreach(what gd18 arxiv)
    add_custom_command(
        OUTPUT graphstudy-${what}.pdf
        COMMAND "${CMAKE_COMMAND}" -E env ${typo_toolchain_env}
                "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/typeset.py"
                "--srcdir=${CMAKE_CURRENT_SOURCE_DIR}"
                "--bindir=${CMAKE_CURRENT_BINARY_DIR}"
                "--tex=LUALATEX"
                "--bib=BIBTEX"
                "--jobname=wip-${what}"
                "--max-print-line=1024"
                -- "graphstudy-${what}.tex" ${paper_dependencies} preamble-local.tex
        COMMAND "${CMAKE_COMMAND}" -E copy "wip-${what}.pdf" "graphstudy-${what}.pdf"
        VERBATIM
        DEPENDS graphstudy-${what}.tex ${paper_dependencies} preamble-local.tex cache.stamp
    )
endforeach(what)

add_custom_command(
    OUTPUT literature-bibtex.bib
    COMMAND "${CMAKE_COMMAND}" -E remove -f literature-bibtex.bib
    COMMAND "${AWK_EXECUTABLE}" -v "IGNORECASE=1" -v "OUTFILE=literature-bibtex.bib"
            -f "${CMAKE_CURRENT_SOURCE_DIR}/transmogrify.awk" -- "${CMAKE_CURRENT_SOURCE_DIR}/literature.bib"
    VERBATIM
    DEPENDS literature.bib transmogrify.awk
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
    OUTPUT nn-info.tex
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/format-nn-info.py"
            -o nn-info.tex --corpus=nn-dump/corpus.json
            --shared-info=nn-dump/shared.txt --total-info=nn-dump/total.txt
    VERBATIM
    DEPENDS nn-dump.stamp
            "${PROJECT_SOURCE_DIR}/utils/format-nn-info.py"
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
            -t STRESS_FIT_SCALE -t HUANG
            "${PROJECT_BINARY_DIR}/eval/cross-valid.json"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS eval-cross-valid.stamp rename-tests.txt
            "${PROJECT_SOURCE_DIR}/utils/format-competing-metrics.py"
)

add_custom_command(
    OUTPUT eval-huang-weights.tex
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/format-huang-weights.py"
            -o "${CMAKE_CURRENT_BINARY_DIR}/eval-huang-weights.tex"
            -r "${CMAKE_CURRENT_SOURCE_DIR}/rename-metrics.txt"
            "${PROJECT_BINARY_DIR}/eval/huang-weights.json"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS eval-cross-valid.stamp rename-metrics.txt
            "${PROJECT_SOURCE_DIR}/utils/format-huang-weights.py"
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
    OUTPUT preamble-local.tex
    COMMAND "${CMAKE_COMMAND}" -E touch preamble-local.tex
    COMMENT "Creating stub file '${CMAKE_CURRENT_BINARY_DIR}/preamble-local.tex' (free feel to edit)"
    VERBATIM
)

add_custom_command(
    OUTPUT cache.stamp
    COMMAND "${CMAKE_COMMAND}" -E touch cache.stamp
    VERBATIM
    COMMENT "Faking stamp file '${CMAKE_CURRENT_BINARY_DIR}/cache.stamp'"
)

add_custom_command(
    OUTPUT cache-nofake.stamp
    COMMAND "${CMAKE_COMMAND}" -E env ${typo_toolchain_env}
            "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/typeset.py" --prep-deps --jobname=wip-cache
            "--srcdir=${CMAKE_CURRENT_SOURCE_DIR}" "--bindir=${CMAKE_CURRENT_BINARY_DIR}"
            -- ${paper_dependencies} preamble-local.tex
    COMMAND "${CMAKE_COMMAND}"
            -D "TEX=${LUALATEX_EXECUTABLE}"
            -D "PDFTOPS=${PDFTOPS_EXECUTABLE}"
            -D "jobname=wip-cache"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/update-cache.cmake"
    COMMAND "${CMAKE_COMMAND}" -E touch cache.stamp cache-nofake.stamp
    VERBATIM
    BYPRODUCTS cache.stamp
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/update-cache.cmake" ${paper_dependencies} preamble-local.tex
)

add_custom_command(
    OUTPUT graphstudy.zip
    COMMAND "${CMAKE_COMMAND}" -E remove -f graphstudy.zip
    COMMAND "${ZIP_EXECUTABLE}" -rq graphstudy.zip -- ${pubfiles} cache/
    VERBATIM
    DEPENDS cache-nofake.stamp ${pubfiles}
)

add_custom_target(paper       DEPENDS graphstudy-gd18.pdf graphstudy-arxiv.pdf)
add_custom_target(paper-cache DEPENDS cache-nofake.stamp)
add_custom_target(paper-pubar DEPENDS graphstudy.zip)

add_dependencies(paper       paper-pics)
add_dependencies(paper-cache paper-pics)
add_dependencies(paper-pubar paper-pics)
