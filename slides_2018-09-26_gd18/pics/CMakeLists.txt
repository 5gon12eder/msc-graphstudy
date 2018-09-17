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

set(slides_gd18_picfiles_static
    discriminator.tex
    CACHE INTERNAL ""
)

set(auxlist)

# ----------------- #
#   Non-Technical   #
# ----------------- #

list(APPEND auxlist iosb-logo.pdf)
add_custom_command(
    OUTPUT iosb-logo.pdf
    COMMAND "${CMAKE_COMMAND}" -E copy "${MSC_LOGO_IOSB}" iosb-logo.pdf
    VERBATIM
    DEPENDS "${MSC_LOGO_IOSB}"
)

list(APPEND auxlist titlepicture.pdf)
add_custom_command(
    OUTPUT titlepicture.pdf
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=fe1b440ac95c28404f83b221d1c0950934cbb288118cff2d"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -s -n 2000 -o titlegraph.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" "--node-color=#4664AA" "--edge-color=#4664AA"
            -o titlegraph.svg titlegraph.xml
    COMMAND "${INKSCAPE_EXECUTABLE}" "--export-background=#4664AA" "--export-background-opacity=0.50"
            --export-pdf=titlepicture.pdf titlegraph.svg
    BYPRODUCTS titlegraph.xml titlegraph.svg
    VERBATIM
    DEPENDS mosaic picture
)

# ------------ #
#   Examples   #
# ------------ #

list(APPEND auxlist example-a.tikz)
add_custom_command(
    OUTPUT example-a.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o example-a.tikz
            "${PROJECT_SOURCE_DIR}/octets/2c0ab2fa-dab7fdeb.xml.bz2"
    VERBATIM
    DEPENDS picture "${PROJECT_SOURCE_DIR}/octets/2c0ab2fa-dab7fdeb.xml.bz2"
)

list(APPEND auxlist example-b.tikz)
add_custom_command(
    OUTPUT example-b.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o example-b.tikz
            "${PROJECT_SOURCE_DIR}/octets/2c0ab2fa-fb8d2845.xml.bz2"
    VERBATIM
    DEPENDS picture "${PROJECT_SOURCE_DIR}/octets/2c0ab2fa-fb8d2845.xml.bz2"
)

set(showcases
    2c0ab2fa-dab7fdeb      2c0ab2fa-fb8d2845       # example from introduction
    5b3b66d2-a07125fe-more 5b3b66d2-101708ec-less  # counter-example for HUANG
    8314f2c1-0b878c80-more 8314f2c1-c7b63d0b-less  # counter-example for STRESS_FIT_SCALE
    d8c1498f-4c326179-more d8c1498f-234952e3-less  # counter-example for both
)

foreach(layout ${showcases})
    list(APPEND auxlist ${layout}.tikz)
    add_custom_command(
        OUTPUT ${layout}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
                --tikz -o "${layout}.tikz" "${PROJECT_SOURCE_DIR}/octets/${layout}.xml.bz2"
        VERBATIM
        DEPENDS picture "${PROJECT_SOURCE_DIR}/octets/${layout}.xml.bz2"
    )
endforeach(layout)

# ------------------ #
#   Import Sources   #
# ------------------ #

list(APPEND auxlist import.tikz)
add_custom_command(
    OUTPUT import.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o import.tikz "${PROJECT_SOURCE_DIR}/test/data/kit-layout.xml"
    VERBATIM
    DEPENDS "${PROJECT_SOURCE_DIR}/test/data/kit-layout.xml" picture
)

add_custom_command(
    OUTPUT bcspwr.xml
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/download.py"
            "--checksum=sha256:2da77005782adcccfb7f0d7b0fa088263f0892f292a0a72e51ffaf44be15bb4b"
            "--output=bcspwr.mtx.gz"
            "ftp://math.nist.gov/pub/MatrixMarket2/Harwell-Boeing/bcspwr/bcspwr04.mtx.gz"
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/generators/import" -y -f matrix-market -o bcspwr.xml bcspwr.mtx.gz
    VERBATIM
    BYPRODUCTS bcspwr.mtx.gz
    DEPENDS import
)

add_custom_command(
    OUTPUT grenoble.xml
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/download.py"
            "--checksum=sha256:6b863e4f6f426c7d5ba39827076ec907833b3787ad8ae7c9fbcf871bd4a119d2"
            "--output=grenoble.mtx.gz"
            "ftp://math.nist.gov/pub/MatrixMarket2/Harwell-Boeing/grenoble/gre__115.mtx.gz"
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/generators/import" -y -f matrix-market -o grenoble.xml grenoble.mtx.gz
    VERBATIM
    BYPRODUCTS grenoble.mtx.gz
    DEPENDS import
)

add_custom_command(
    OUTPUT psadmit.xml
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/download.py"
            "--checksum=sha256:55a2551e699253653e1b297467da1fdb7b647f850b05eb2c6647f8d66a1d8748"
            "--output=psadmit.mtx.gz"
            "ftp://math.nist.gov/pub/MatrixMarket2/Harwell-Boeing/psadmit/494_bus.mtx.gz"
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/generators/import" -y -f matrix-market -o psadmit.xml psadmit.mtx.gz
    VERBATIM
    BYPRODUCTS psadmit.mtx.gz
    DEPENDS import
)

add_custom_command(
    OUTPUT smtape.xml
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/download.py"
            "--checksum=sha256:6a719b0c5e130f340f6286020c04813264bda815570e50ab77e9f83efe673133"
            "--output=smtape.mtx.gz"
            "ftp://math.nist.gov/pub/MatrixMarket2/Harwell-Boeing/smtape/will57.mtx.gz"
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/generators/import" -y -f matrix-market -o smtape.xml smtape.mtx.gz
    VERBATIM
    BYPRODUCTS smtape.mtx.gz
    DEPENDS import
)

foreach(ar "bcspwr" "grenoble" "psadmit" "smtape")
    list(APPEND auxlist ${ar}.tikz)
    add_custom_command(
        OUTPUT ${ar}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=L1DZNhZZI92q"
                "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o ${ar}-layout.xml ${ar}.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o ${ar}.tikz ${ar}-layout.xml
        BYPRODUCTS ${ar}-layout.xml
        VERBATIM
        DEPENDS ${ar}.xml force picture
    )
endforeach()

foreach(ar "rome" "north" "randdag")
    list(APPEND auxlist ${ar}.tikz)
    add_custom_command(
        OUTPUT ${ar}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/layouts/force" -a STRESS -o ${ar}-layout.xml
                "${PROJECT_SOURCE_DIR}/octets/sample-graph-${ar}.xml.bz2"
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o ${ar}.tikz ${ar}-layout.xml
        BYPRODUCTS ${ar}-graph.xml ${ar}-layout.xml
        VERBATIM
        DEPENDS force picture "${PROJECT_SOURCE_DIR}/octets/sample-graph-${ar}.xml.bz2"
    )
endforeach()

# -------------- #
#   Generators   #
# -------------- #

list(APPEND auxlist lindenmayer.tikz)
add_custom_command(
    OUTPUT lindenmayer.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=yeanWQ2ib3Ko"
            "${PROJECT_BINARY_DIR}/src/generators/lindenmayer" -n 50 -o lindenmayer.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o lindenmayer.tikz lindenmayer.xml
    BYPRODUCTS lindenmayer.xml
    VERBATIM
    DEPENDS lindenmayer picture
)

foreach(n RANGE 3 6)
    list(APPEND auxlist quasi${n}d.tikz)
    add_custom_command(
        OUTPUT quasi${n}d.tikz
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=ae0UPRUah1jB"
                "${PROJECT_BINARY_DIR}/src/generators/quasi" -h ${n} -n 50 -o quasi${n}d.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o quasi${n}d.tikz quasi${n}d.xml
        BYPRODUCTS quasi${n}d.xml
        VERBATIM
        DEPENDS quasi picture
    )
endforeach(n)

set(grid_example_seed QfW3TWXlyT7p)

list(APPEND auxlist grid.tikz)
add_custom_command(
    OUTPUT grid.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=${grid_example_seed}"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 50 -o grid.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o grid.tikz grid.xml
    BYPRODUCTS grid.xml
    VERBATIM
    DEPENDS grid picture
)

foreach(i 1 2)
    list(APPEND auxlist torus${i}.tikz)
    add_custom_command(
        OUTPUT torus${i}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=${grid_example_seed}"
                "${PROJECT_BINARY_DIR}/src/generators/grid" -t ${i} -n 50 -o torus${i}-graph.xml
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=fqZ6Cb4vedtz"
                "${PROJECT_BINARY_DIR}/src/layouts/force" -a stress -o torus${i}.xml torus${i}-graph.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o torus${i}.tikz torus${i}.xml
        BYPRODUCTS torus${i}-graph.xml torus${i}.xml
        VERBATIM
        DEPENDS grid force picture
    )
endforeach(i)

list(APPEND auxlist bottle.tikz)
add_custom_command(
    OUTPUT bottle.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=nkKVGqaAPOi5"
            "${PROJECT_BINARY_DIR}/src/generators/bottle" -n 50 -o bottle.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o bottle.tikz bottle.xml
    BYPRODUCTS bottle.xml
    VERBATIM
    DEPENDS bottle picture
)

list(APPEND auxlist mosaic1.tikz)
add_custom_command(
    OUTPUT mosaic1.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=pH5sTIq0aknr"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -n 50 -o mosaic1.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o mosaic1.tikz mosaic1.xml
    BYPRODUCTS mosaic1.xml
    VERBATIM
    DEPENDS mosaic picture
)

list(APPEND auxlist mosaic2.tikz)
add_custom_command(
    OUTPUT mosaic2.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=78rJsfvEW6YP"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -s -n 50 -o mosaic2.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o mosaic2.tikz mosaic2.xml
    BYPRODUCTS mosaic2.xml
    VERBATIM
    DEPENDS mosaic picture
)

list(APPEND auxlist tree.tikz)
add_custom_command(
    OUTPUT tree.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=1QC1A7wMtYJS"
            "${PROJECT_BINARY_DIR}/src/generators/tree" -n 50 -o tree-graph.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=Iq8pIVSWDXMl"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o tree-layout.xml tree-graph.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o tree.tikz tree-layout.xml
    BYPRODUCTS tree-graph.xml tree-layout.xml
    VERBATIM
    DEPENDS tree picture
)

# ----------- #
#   Layouts   #
# ----------- #

list(APPEND auxlist native.tikz)
add_custom_command(
    OUTPUT native.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=42WQduACAPYk"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -s -n 100 -o native.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o native.tikz native.xml
    BYPRODUCTS native.xml
    VERBATIM
    DEPENDS mosaic picture
)

foreach(algo fmmm stress)
    list(APPEND auxlist ${algo}.tikz)
    add_custom_command(
        OUTPUT ${algo}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=ffk08M8NB0Ey"
                "${PROJECT_BINARY_DIR}/src/layouts/force" -a ${algo} -o ${algo}.xml native.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o ${algo}.tikz ${algo}.xml
        BYPRODUCTS ${algo}.xml
        VERBATIM
        DEPENDS native.xml force picture
    )
endforeach()

foreach(dist uniform normal)
    list(APPEND auxlist random-${dist}.tikz)
    add_custom_command(
        OUTPUT random-${dist}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=is5y2daxuKOT"
                "${PROJECT_BINARY_DIR}/src/layouts/random" -d ${dist} -o random-${dist}.xml native.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o random-${dist}.tikz random-${dist}.xml
        BYPRODUCTS random-${dist}.xml
        VERBATIM
        DEPENDS native.xml random picture
    )
endforeach()

list(APPEND auxlist phantom.tikz)
add_custom_command(
    OUTPUT phantom.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=DvcEx6XYuWA6"
            "${PROJECT_BINARY_DIR}/src/layouts/phantom" -o phantom.xml native.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o phantom.tikz phantom.xml
    BYPRODUCTS phantom.xml
    VERBATIM
    DEPENDS native.xml phantom picture
)

# -------------------- #
#   Layout Worsening   #
# -------------------- #

add_custom_command(
    OUTPUT worse-parent.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=N0ukQGf0Kgq7"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 100 -o worse-parent.xml
    VERBATIM
    DEPENDS grid
)

list(APPEND auxlist worse-parent.tikz)
add_custom_command(
    OUTPUT worse-parent.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o worse-parent.tikz worse-parent.xml
    VERBATIM
    DEPENDS picture worse-parent.xml
)

foreach(worse perturb flip-nodes flip-edges movlsq)
    list(APPEND auxlist ${worse}-01000.tikz ${worse}-02000.tikz ${worse}-05000.tikz)
    add_custom_command(
        OUTPUT ${worse}-01000.xml ${worse}-02000.xml ${worse}-05000.xml
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=SMr6hRcYXvCn"
                "${PROJECT_BINARY_DIR}/src/unitrans/${worse}" -r 0.10 -r 0.20 -r 0.50
                -o "${worse}-%.xml" worse-parent.xml
        VERBATIM
        DEPENDS ${worse} worse-parent.xml
    )
    foreach(rate 01000 02000 05000)
        add_custom_command(
            OUTPUT ${worse}-${rate}.tikz
            COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
                    --tikz -o ${worse}-${rate}.tikz ${worse}-${rate}.xml
            VERBATIM
            DEPENDS picture ${worse}-${rate}.xml
        )
    endforeach(rate)
endforeach(worse)

# ------------------------ #
#   Layout Interpolation   #
# ------------------------ #

add_custom_command(
    OUTPUT inter-parent-a.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=NentNdHAVSo3"
            "${PROJECT_BINARY_DIR}/src/generators/lindenmayer" -n 300 -o inter-parent-a.xml
    VERBATIM
    DEPENDS lindenmayer
)

add_custom_command(
    OUTPUT inter-parent-b.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=dWjgbyy9jGmF"
            "${PROJECT_BINARY_DIR}/src/layouts/random" -d normal -o inter-parent-b.xml inter-parent-a.xml
    VERBATIM
    DEPENDS random inter-parent-a.xml
)

add_custom_command(
    OUTPUT linear-00000.xml linear-02000.xml linear-04000.xml linear-06000.xml linear-08000.xml linear-10000.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=5njaKlZeaMUV"
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -o "linear-%.xml"
            -r 0.00 -r 0.20 -r 0.40 -r 0.60 -r 0.80 -r 1.00
            inter-parent-a.xml inter-parent-b.xml
    VERBATIM
    DEPENDS interpol inter-parent-a.xml inter-parent-b.xml
)

foreach(inter linear)
    foreach(rate 00000 02000 04000 06000 08000 10000)
        list(APPEND auxlist ${inter}-${rate}.tikz)
        add_custom_command(
            OUTPUT ${inter}-${rate}.tikz
            COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
                    --tikz -o ${inter}-${rate}.tikz ${inter}-${rate}.xml
            VERBATIM
            DEPENDS picture ${inter}-${rate}.xml
        )
    endforeach(rate)
endforeach(inter)

# -------------- #
#   Properties   #
# -------------- #

add_custom_command(
    OUTPUT demograph-a.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=GUCR52A2kDTD"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 100 -o demograph-a.xml
    VERBATIM
    DEPENDS grid
)

add_custom_command(
    OUTPUT demograph-b.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=Fvcr62zM0qWR"
            "${PROJECT_BINARY_DIR}/src/unitrans/perturb" -r 0.15 -o demograph-b.xml demograph-a.xml
    VERBATIM
    DEPENDS perturb demograph-a.xml
)

add_custom_command(
    OUTPUT bcspwr04.xml
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/download.py"
            "--checksum=sha256:2da77005782adcccfb7f0d7b0fa088263f0892f292a0a72e51ffaf44be15bb4b"
            "--output=bcspwr04.mtx.gz"
            "ftp://math.nist.gov/pub/MatrixMarket2/Harwell-Boeing/bcspwr/bcspwr04.mtx.gz"
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/generators/import" -y -f matrix-market -o bcspwr04.xml bcspwr04.mtx.gz
    VERBATIM
    BYPRODUCTS bcspwr04.mtx.gz
    DEPENDS import
)

add_custom_command(
    OUTPUT demograph-c.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=mkZBvzhuPXS9"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o demograph-c.xml bcspwr04.xml
    VERBATIM
    DEPENDS force bcspwr04.xml
)

foreach(letter a b c)
    set(basename demograph-${letter})
    list(APPEND auxlist ${basename}.tikz)
    add_custom_command(
        OUTPUT ${basename}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o ${basename}.tikz ${basename}.xml
        VERBATIM
        DEPENDS picture ${basename}.xml
    )
endforeach(letter)

set(demoletters a b c)
set(democolors "#82BE3C" "#82BE3C" "#4664AA")

foreach(i RANGE 2)
    list(GET demoletters ${i} letter)
    list(GET democolors ${i} color)
    set(basename angular-${letter})
    set(demobase demograph-${letter})
    set(sigma 0.1)
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/angular"
                -k gaussian -w ${sigma} -p 512 -o ${basename}.txt ${demobase}.xml
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'"
                -e "SIGMA=${sigma}" -e "COLOR='${color}'"
                "${CMAKE_CURRENT_SOURCE_DIR}/angular.gplt"
        VERBATIM
        DEPENDS angular property-config.gplt angular.gplt ${demobase}.xml
    )
endforeach(i)

foreach(i RANGE 2)
    list(GET demoletters ${i} letter)
    list(GET democolors ${i} color)
    set(basename rdf-global-${letter})
    set(demobase demograph-${letter})
    set(sigma 25.0)
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/rdf-global"
                -k gaussian -w ${sigma} -p 1024 -o ${basename}.txt ${demobase}.xml
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'"
                -e "SIGMA=${sigma}" -e "XSTEP=5" -e "COLOR='${color}'"
                "${CMAKE_CURRENT_SOURCE_DIR}/rdf-global.gplt"
        VERBATIM
        DEPENDS rdf-global property-config.gplt rdf-global.gplt ${demobase}.xml
    )
endforeach(i)

set(slides_gd18_picfiles_dynamic ${auxlist} CACHE INTERNAL "")
set(slides_gd18_picfiles ${slides_gd18_picfiles_static} ${slides_gd18_picfiles_dynamic} CACHE INTERNAL "")
add_custom_target(slides-gd18-pics DEPENDS ${slides_gd18_picfiles_dynamic})
