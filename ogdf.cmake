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

include(ExternalProject)

set(depsdir "${CMAKE_BINARY_DIR}/dependencies")
set(ogdfdir "${depsdir}/ogdf")

# TODO: Consider performing this download also via our Python script for consistency.  The CMake command supports a
# `DOWNLOAD_COMMAND` parameter allowing us to call our own script.  Unfortunately, our script cannot currently extract
# all members of an archive, even less so performing path translation.

externalproject_add(
    ogdf-dependency
    PREFIX "${depsdir}"
    STAMP_DIR "${depsdir}/ogdf-stamp"
    SOURCE_DIR "${ogdfdir}"
    URL "http://ogdf.net/lib/exe/fetch.php/tech:ogdf-snapshot-2018-03-28.zip"
    DOWNLOAD_NAME "ogdf-snapshot-2018-03-28.zip"
    URL_HASH "SHA256=347c3fa8dcdbb094f9c43008cbc2934bc5a64a532af4d3fe360a5fa54488440f"
    DOWNLOAD_NO_PROGRESS TRUE
    CMAKE_ARGS "-DCMAKE_COLOR_MAKEFILE=${CMAKE_COLOR_MAKEFILE}"
               "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
               "-DCMAKE_CXX_COMPILER=${OGDF_CMAKE_CXX_COMPILER}"
               "-DCMAKE_CXX_FLAGS=${OGDF_CMAKE_CXX_FLAGS}"
               "-DCMAKE_CXX_FLAGS_DEBUG=${OGDF_CMAKE_CXX_FLAGS_DEBUG}"
               "-DCMAKE_CXX_FLAGS_RELEASE=${OGDF_CMAKE_CXX_FLAGS_RELEASE}"
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
)

add_library(ogdf INTERFACE)
add_dependencies(ogdf ogdf-dependency)
target_link_libraries(ogdf INTERFACE "${ogdfdir}/libOGDF.a" "${ogdfdir}/libCOIN.a")
target_include_directories(ogdf SYSTEM INTERFACE "${ogdfdir}/include")
