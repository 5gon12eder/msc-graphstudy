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

if(EXISTS "cache/")
    file(REMOVE_RECURSE "cache/")
endif()

file(MAKE_DIRECTORY cache)
file(WRITE "${jobname}.stdin" "")

set(extratexdefs "\\def\\OnlyGDXVIII#1{#1} \\def\\OnlyArxiv#1{#1}")

message(STATUS "Exporting tikz pictures from main TeX file ...")
execute_process(
    COMMAND "${TEX}" -halt-on-error -interaction=batchmode "-jobname=${jobname}"
            "${extratexdefs} \\def\\TikzExternalMode{list only} \\input{graphstudy.tex}"
    INPUT_FILE "${jobname}.stdin"
    OUTPUT_FILE "${jobname}.stdout"
    ERROR_FILE "${jobname}.stderr"
    RESULT_VARIABLE status
)
if(NOT ${status} EQUAL 0)
    message(FATAL_ERROR "${TEX} quit with status ${status}")
endif()

file(STRINGS "${jobname}.figlist" figlist)
list(REMOVE_DUPLICATES figlist)
list(FILTER figlist EXCLUDE REGEX "^cache/image-")
list(LENGTH figlist n)

message(STATUS "Converting ${n} pictures; please be patient ...")
foreach(fig ${figlist})
    message(STATUS "${fig}")
    execute_process(
        COMMAND "${TEX}" -halt-on-error -interaction=batchmode "-jobname=${fig}"
                "${extratexdefs} \\def\\tikzexternalrealjob{${jobname}}\\input{graphstudy.tex}"
        INPUT_FILE "${jobname}.stdin"
        OUTPUT_FILE "${jobname}.stdout"
        ERROR_FILE "${jobname}.stderr"
        RESULT_VARIABLE status
    )
    if(NOT ${status} EQUAL 0)
        message(FATAL_ERROR "${TEX} quit with status ${status}")
    endif()
    execute_process(
        COMMAND "${PDFTOPS}" -level3 -eps "${fig}.pdf" "${fig}.eps"
        #       "${pdf2ps}" -dLanguageLevel=3 "${fig}.pdf" "${fig}.ps"
        #       "${gs}" -q -dNOCACHE -dNOPAUSE -dBATCH -dSAFER -sDEVICE=eps2write -sOutputFile="${fig}.eps" "${fig}.pdf"
        #       "${inkscape}" --export-eps="${fig}.eps" "${fig}.pdf"
        INPUT_FILE "${jobname}.stdin"
        OUTPUT_FILE "${jobname}.stdout"
        ERROR_FILE "${jobname}.stderr"
        RESULT_VARIABLE status
    )
    if(NOT ${status} EQUAL 0)
        message(FATAL_ERROR "PDF to EPS conversion command quit with status ${status}")
    endif()
endforeach(fig)

file(GLOB garbage_files "${jobname}.*" "cache/*.aux" "cache/*.log" "cache/*.out")
list(LENGTH garbage_files garbage_file_count)
message(STATUS "Removing ${garbage_file_count} garbage files")
if(${garbage_file_count} GREATER 0)
    file(REMOVE ${garbage_files})
endif()
