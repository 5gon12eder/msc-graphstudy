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

set(paper_picfiles_static
    discriminator.tex
    gen-lindenmayer-clique.tex
    gen-lindenmayer-grid.tex
    gen-lindenmayer-ring.tex
    gen-lindenmayer-singleton.tex
    gen-lindenmayer-star.tex
    gen-lindenmayer-wheel.tex
    gen-mosaic-flower.tex
    gen-mosaic-shape.tex
    gen-mosaic-star.tex
    CACHE INTERNAL ""
)

set(auxlist)

set(example_layouts
    8314f2c1-4d644640-more 8314f2c1-bac12550-less
    d8c1498f-8c5ad49b-more d8c1498f-4c2beafc-less
    5b3b66d2-49e8882a-more 5b3b66d2-f60fcc04-less
)
foreach(layout ${example_layouts})
    list(APPEND auxlist ${layout}.tikz)
    add_custom_command(
        OUTPUT ${layout}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
                --tikz -o "${layout}.tikz" "${CMAKE_CURRENT_SOURCE_DIR}/${layout}.xml.bz2"
        VERBATIM
        DEPENDS picture "${CMAKE_CURRENT_SOURCE_DIR}/${layout}.xml.bz2"
    )
endforeach(layout)

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

foreach(letter a b c)
    set(basename rdf-global-${letter})
    set(demobase demograph-${letter})
    set(sigma 25.0)
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/rdf-global"
                -k gaussian -w ${sigma} -p 1024 -o ${basename}.txt ${demobase}.xml
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'" -e "SIGMA=${sigma}" -e "XSTEP=5"
                "${CMAKE_CURRENT_SOURCE_DIR}/rdf-global.gplt"
        VERBATIM
        DEPENDS rdf-global property-config.gplt rdf-global.gplt ${demobase}.xml
    )
endforeach(letter)

foreach(letter a b c)
    set(basename angular-${letter})
    set(demobase demograph-${letter})
    set(sigma 0.1)
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/angular"
                -k gaussian -w ${sigma} -p 512 -o ${basename}.txt ${demobase}.xml
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'" -e "SIGMA=${sigma}" -e "XSTEP=5"
                "${CMAKE_CURRENT_SOURCE_DIR}/angular.gplt"
        VERBATIM
        DEPENDS angular property-config.gplt angular.gplt ${demobase}.xml
    )
endforeach(letter)

list(APPEND auxlist lindenmayer.tikz)
add_custom_command(
    OUTPUT lindenmayer.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=j0gIm3pLkt4b"
            "${PROJECT_BINARY_DIR}/src/generators/lindenmayer" -n 50 -o lindenmayer.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o lindenmayer.tikz lindenmayer.xml
    BYPRODUCTS lindenmayer.xml
    VERBATIM
    DEPENDS lindenmayer picture
)

foreach(dim 3 4 5 6)
list(APPEND auxlist quasi${dim}d.tikz)
    add_custom_command(
        OUTPUT quasi${dim}d.tikz
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=ae0UPRUah1jB"
                "${PROJECT_BINARY_DIR}/src/generators/quasi" -h ${dim} -n 50 -o quasi${dim}d.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o quasi${dim}d.tikz quasi${dim}d.xml
        BYPRODUCTS quasi${dim}d.xml
        VERBATIM
        DEPENDS quasi picture
    )
endforeach(dim)

list(APPEND auxlist import.tikz)
add_custom_command(
    OUTPUT import.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o import.tikz "${PROJECT_SOURCE_DIR}/test/data/kit-layout.xml"
    VERBATIM
    DEPENDS "${PROJECT_SOURCE_DIR}/test/data/kit-layout.xml" picture
)

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
endforeach(ar)

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
endforeach(ar)

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
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=3PlNaAPysWm0"
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
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=S6c1Yd9KzFeM"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -n 30 -o mosaic1.xml
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
endforeach(algo)

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
endforeach(dist)

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

add_custom_command(
    OUTPUT worse-parent.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=N0ukQGf0Kgq7"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 100 -o worse-parent.xml
    VERBATIM
    DEPENDS grid
)

foreach(worse perturb flip-nodes flip-edges movlsq)
    list(APPEND auxlist ${worse}-00000.tikz ${worse}-01500.tikz ${worse}-05000.tikz ${worse}-10000.tikz)
    add_custom_command(
        OUTPUT ${worse}-00000.xml ${worse}-01500.xml ${worse}-05000.xml ${worse}-10000.xml
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=SMr6hRcYXvCn"
                "${PROJECT_BINARY_DIR}/src/unitrans/${worse}" -r 0.0 -r 0.15 -r 0.50 -r 1.0
                -o "${worse}-%.xml" worse-parent.xml
        VERBATIM
        DEPENDS ${worse} worse-parent.xml
    )
    foreach(rate 00000 01500 05000 10000)
        add_custom_command(
            OUTPUT ${worse}-${rate}.tikz
            COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
                    --tikz -o ${worse}-${rate}.tikz ${worse}-${rate}.xml
            VERBATIM
            DEPENDS picture ${worse}-${rate}.xml
        )
    endforeach(rate)
endforeach(worse)

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
    OUTPUT inter-parent-c.xml inter-parent-d.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=mAmMN4KaWGUN"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o inter-parent-c.xml inter-parent-a.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=fDk9lt9Xi6TX"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o inter-parent-d.xml inter-parent-a.xml
    VERBATIM
    DEPENDS force inter-parent-a.xml
)

add_custom_command(
    OUTPUT linear-00000.xml linear-02500.xml linear-07500.xml linear-10000.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=5njaKlZeaMUV"
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -o "linear-%.xml" -r 0.00 -r 0.25 -r 0.75 -r 1.00
            inter-parent-a.xml inter-parent-b.xml
    VERBATIM
    DEPENDS interpol inter-parent-a.xml inter-parent-b.xml
)

add_custom_command(
    OUTPUT xlinear-00000.xml xlinear-02500.xml xlinear-07500.xml xlinear-10000.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=5njaKlZeaMUV"
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -c -o "xlinear-%.xml" -r 0.00 -r 0.25 -r 0.75 -r 1.00
            inter-parent-c.xml inter-parent-d.xml
    VERBATIM
    DEPENDS interpol inter-parent-c.xml inter-parent-d.xml
)

foreach(inter linear xlinear)
    foreach(rate 00000 02500 07500 10000)
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

add_custom_command(
    OUTPUT entropy-regression.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=xe8jFVGvR9iE"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 1000 -o entropy-regression.xml
    VERBATIM
    DEPENDS grid
)

add_custom_command(
    OUTPUT entropy-regression-00100.xml
           entropy-regression-00200.xml
           entropy-regression-00500.xml
           entropy-regression-01000.xml
           entropy-regression-02000.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=JsggEOr9x3dQ"
            "${PROJECT_BINARY_DIR}/src/unitrans/perturb" -r 0.01 -r 0.02 -r 0.05 -r 0.10 -r 0.20
            -o entropy-regression-%.xml entropy-regression.xml
    VERBATIM
    DEPENDS perturb entropy-regression.xml
)

set(regression_binflags -b 8 -b 11 -b 16 -b 23 -b 32 -b 45 -b 64 -b 91 -b 128 -b 181 -b 256 -b 362 -b 512)
foreach(rate 00100 00200 00500 01000 02000)
    add_custom_command(
        OUTPUT entropy-regression-${rate}.json
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/angular"
                -k boxed ${regression_binflags} -o NULL -m entropy-regression-${rate}.json
                entropy-regression-${rate}.xml
        VERBATIM
        DEPENDS angular entropy-regression-${rate}.xml
    )
    add_custom_command(
        OUTPUT entropy-regression-${rate}.txt entropy-regression-${rate}.gplt
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/prepare-entropy-regression.py"
                -k bincount -l ${rate}
                -o entropy-regression-${rate}.txt
                -r entropy-regression-${rate}.gplt
                entropy-regression-${rate}.json
        VERBATIM
        DEPENDS entropy-regression-${rate}.json
                "${PROJECT_SOURCE_DIR}/utils/prepare-entropy-regression.py"
    )
endforeach(rate)

list(APPEND auxlist entropy-regression.pgf)
add_custom_command(
    OUTPUT entropy-regression.pgf
    COMMAND "${GNUPLOT_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/entropy-regression.gplt"
    VERBATIM
    DEPENDS entropy-regression.gplt
            entropy-regression-00100.txt
            entropy-regression-00200.txt
            entropy-regression-00500.txt
            entropy-regression-01000.txt
            entropy-regression-02000.txt
)

set(paper_picfiles_dynamic ${auxlist} CACHE INTERNAL "")
set(paper_picfiles ${paper_picfiles_static} ${paper_picfiles_dynamic} CACHE INTERNAL "")
add_custom_target(paper-pics DEPENDS ${paper_picfiles_dynamic})
