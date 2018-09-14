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

foreach(dir "cache")
    if(EXISTS "${dir}/")
        message(STATUS "Deleting directory '${dir}/' ...")
        file(REMOVE_RECURSE "${dir}/")
    endif()
endforeach(dir)

file(
    GLOB garbage_files
         "*.aux"
         "*.bbl"
         "*.bcf"
         "*.blg"
         "*.cfg"
         "*.idx"
         "*.ilg"
         "*.ind"
         "*.lof"
         "*.log"
         "*.lot"
         "*.out"
         "*.run.xml"
         "*.stamp"
         "*.toc"
         "*.upa"
         "*.upb"
         "wip-*"
)

list(SORT garbage_files)
list(REMOVE_DUPLICATES garbage_files)
list(LENGTH garbage_files garbage_file_count)
message(STATUS "There are ${garbage_file_count} garbage files")

if(${garbage_file_count} GREATER 0)
    foreach(item ${garbage_files})
        message(STATUS "Deleting garbage file ${item}")
    endforeach(item)
    file(REMOVE ${garbage_files})
endif()
