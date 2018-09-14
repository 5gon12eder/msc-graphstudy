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

set(slides_gd18_picfiles_static CACHE INTERNAL "")
set(auxlist CACHE INTERNAL "")

set(slides_gd18_picfiles_dynamic ${auxlist} CACHE INTERNAL "")
set(slides_gd18_picfiles ${slides_gd18_picfiles_static} ${slides_gd18_picfiles_dynamic} CACHE INTERNAL "")
add_custom_target(slides-gd18-pics DEPENDS ${slides_gd18_picfiles_dynamic})
