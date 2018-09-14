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

add_custom_command(
    OUTPUT Roboto-Regular.ttf Roboto-Bold.ttf
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/download.py"
            "--checksum=sha256:3d0a957f7070355aa776cd0b09e26d002d4bb141962b53ef6daa893572d45d48"
            "--format=zip"
            "--extract=Roboto-Regular.ttf"
            "--extract=Roboto-Bold.ttf"
            "https://fonts.google.com/download?family=Roboto"
    VERBATIM
)

add_custom_target(report-fonts DEPENDS Roboto-Regular.ttf Roboto-Bold.ttf)
