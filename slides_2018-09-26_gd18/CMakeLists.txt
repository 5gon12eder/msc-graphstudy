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
)

foreach(item ${slides_gd18_picfiles})
    list(APPEND slides_gd18_dependencies "pics/${item}")
endforeach(item)
foreach(item ${slides_gd18_picfiles_dynamic})
    set_source_files_properties("pics/${item}" PROPERTIES GENERATED TRUE)
endforeach(item)

set(MSC_KIT_BEAMER_ZIP
    "${CMAKE_CURRENT_BINARY_DIR}/kit-beamer.zip"
    CACHE PATH "ZIP archive to provide the KIT's beamer theme in a drop-in fashion"
)

add_custom_command(
    OUTPUT kit-beamer.stamp
    COMMAND "${CMAKE_COMMAND}" -E tar xf "${MSC_KIT_BEAMER_ZIP}" --format=zip
    COMMAND "${CMAKE_COMMAND}" -E touch kit-beamer.stamp
    VERBATIM
    DEPENDS "${MSC_KIT_BEAMER_ZIP}"
)

add_custom_command(
    OUTPUT kit-beamer.zip
    COMMAND "${CMAKE_COMMAND}" -E touch kit-beamer.stamp
    COMMAND "${CMAKE_COMMAND}" -E tar cf kit-beamer.zip --format=zip -- kit-beamer.stamp
    VERBATIM
    BYPRODUCTS kit-beamer.stamp
    COMMENT "Creating stub archive '${CMAKE_CURRENT_BINARY_DIR}/kit-beamer.zip'"
)

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

add_custom_target(slides-gd18 DEPENDS graphstudy.pdf)
add_dependencies(slides-gd18 slides-gd18-pics)
