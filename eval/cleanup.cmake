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

file(GLOB cleanup_files "*.json" "*.stamp" ".progress")
file(GLOB cleanup_dirs "config-*/")

list(LENGTH cleanup_files cleanup_file_count)
list(LENGTH cleanup_dirs cleanup_dir_count)
message(STATUS "Removing ${cleanup_file_count} files and ${cleanup_dir_count} directories ...")

if(${cleanup_file_count} GREATER 0)
    file(REMOVE ${cleanup_files})
endif()

if(${cleanup_dir_count} GREATER 0)
    file(REMOVE_RECURSE ${cleanup_dirs})
endif()
