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

set(slides_kit_picfiles_static
    algo-logo.pdf
    titlepicture.png
    webui-compare.png
    CACHE INTERNAL ""
)

set(auxlist)

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

list(APPEND auxlist quasi4d.tikz)
add_custom_command(
    OUTPUT quasi4d.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=ae0UPRUah1jB"
            "${PROJECT_BINARY_DIR}/src/generators/quasi" -h 4 -n 50 -o quasi4d.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o quasi4d.tikz quasi4d.xml
    BYPRODUCTS quasi4d.xml
    VERBATIM
    DEPENDS quasi picture
)

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
endforeach()

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
endforeach()

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

add_custom_command(
    OUTPUT worse-parent.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=N0ukQGf0Kgq7"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 250 -o worse-parent.xml
    VERBATIM
    DEPENDS grid
)

foreach(worse perturb flip-nodes flip-edges movlsq)
    list(
        APPEND auxlist
               ${worse}-00000.tikz ${worse}-01000.tikz ${worse}-02000.tikz ${worse}-03000.tikz ${worse}-04000.tikz
               ${worse}-05000.tikz ${worse}-06000.tikz ${worse}-07000.tikz ${worse}-08000.tikz ${worse}-09000.tikz
               ${worse}-10000.tikz
    )
    add_custom_command(
        OUTPUT ${worse}-00000.xml ${worse}-01000.xml ${worse}-02000.xml ${worse}-03000.xml ${worse}-04000.xml
               ${worse}-05000.xml ${worse}-06000.xml ${worse}-07000.xml ${worse}-08000.xml ${worse}-09000.xml
               ${worse}-10000.xml
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=8lefwq8Ha0gN"
                "${PROJECT_BINARY_DIR}/src/unitrans/${worse}" -o "${worse}-%.xml"
                -r 0.0 -r 0.1 -r 0.2 -r 0.3 -r 0.4 -r 0.5 -r 0.6 -r 0.7 -r 0.8 -r 0.9 -r 1.0 worse-parent.xml
        VERBATIM
        DEPENDS ${worse} worse-parent.xml
    )
    foreach(rate 00000 01000 02000 03000 04000 05000 06000 07000 08000 09000 10000)
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
    OUTPUT linear-00000.xml linear-01000.xml linear-02000.xml linear-03000.xml linear-04000.xml
           linear-05000.xml linear-06000.xml linear-07000.xml linear-08000.xml linear-09000.xml
           linear-10000.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=5njaKlZeaMUV"
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -o "linear-%.xml"
            -r 0.0 -r 0.1 -r 0.2 -r 0.3 -r 0.4 -r 0.5 -r 0.6 -r 0.7 -r 0.8 -r 0.9 -r 1.0
            inter-parent-a.xml inter-parent-b.xml
    VERBATIM
    DEPENDS interpol inter-parent-a.xml inter-parent-b.xml
)

add_custom_command(
    OUTPUT xlinear-00000.xml xlinear-01000.xml xlinear-02000.xml xlinear-03000.xml xlinear-04000.xml
           xlinear-05000.xml xlinear-06000.xml xlinear-07000.xml xlinear-08000.xml xlinear-09000.xml
           xlinear-10000.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=5njaKlZeaMUV"
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -o "xlinear-%.xml" -c
            -r 0.0 -r 0.1 -r 0.2 -r 0.3 -r 0.4 -r 0.5 -r 0.6 -r 0.7 -r 0.8 -r 0.9 -r 1.0
            inter-parent-c.xml inter-parent-d.xml
    VERBATIM
    DEPENDS interpol inter-parent-c.xml inter-parent-d.xml
)

foreach(inter linear xlinear)
    foreach(rate 00000 01000 02000 03000 04000 05000 06000 07000 08000 09000 10000)
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

add_custom_command(
    OUTPUT demograph-d.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=jfMW1yxGwkZd"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a stress -o demograph-d.xml bcspwr04.xml
    VERBATIM
    DEPENDS force bcspwr04.xml
)

foreach(letter a b c d)
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
    list(APPEND auxlist demograph-princomp-${letter}.tikz)
    add_custom_command(
        OUTPUT demograph-princomp-${letter}.tikz
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/picture-wrapper.py"
                -i demograph-${letter}.xml -o demograph-princomp-${letter}.tikz
                -1 princomp1st-${letter}.json -2 princomp2nd-${letter}.json
                -- "${PROJECT_BINARY_DIR}/src/visualizations/picture"
        VERBATIM
        DEPENDS picture demograph-${letter}.xml princomp1st-${letter}.json princomp2nd-${letter}.json
                "${PROJECT_SOURCE_DIR}/utils/picture-wrapper.py"
    )
endforeach(letter)

set(gplt_princomp_xmin    ""  -5.5   -5.5  -14.0)
set(gplt_princomp_xmax    ""  +5.5   +5.5  +17.0)
set(gplt_princomp_xstep   ""   2      2      5  )
set(gplt_princomp_letters ""   a      b      c  )

foreach(i RANGE 1 3)
    list(GET gplt_princomp_letters ${i} letter)
    list(GET gplt_princomp_xmin ${i} xmin)
    list(GET gplt_princomp_xmax ${i} xmax)
    list(GET gplt_princomp_xstep ${i} xstep)
    set(demobase demograph-${letter})
    set(base1st princomp1st-${letter})
    set(base2nd princomp2nd-${letter})
    set(sigma 15.0)
    # 1st component
    list(APPEND auxlist ${base1st}.pgf)
    add_custom_command(
        OUTPUT ${base1st}.json ${base1st}.pgf
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/princomp"
                -1 -k gaussian -w ${sigma} -p 512 -o ${base1st}.txt -m ${base1st}.json ${demobase}.xml
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${base1st}.pgf'" -e "INFILE='${base1st}.txt'" -e "SIGMA=${sigma}"
                -e "XMIN=${xmin}" -e "XMAX=${xmax}" -e "XSTEP=${xstep}" "${CMAKE_CURRENT_SOURCE_DIR}/princomp.gplt"
        VERBATIM
        DEPENDS princomp property-config.gplt princomp.gplt ${demobase}.xml
    )
    # 2nd component
    list(APPEND auxlist ${base2nd}.pgf)
    add_custom_command(
        OUTPUT ${base2nd}.json ${base2nd}.pgf
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/princomp"
                -2 -k gaussian -w ${sigma} -p 512 -o ${base2nd}.txt -m ${base2nd}.json ${demobase}.xml
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${base2nd}.pgf'" -e "INFILE='${base2nd}.txt'" -e "SIGMA=${sigma}"
                -e "XMIN=${xmin}" -e "XMAX=${xmax}" -e "XSTEP=${xstep}" "${CMAKE_CURRENT_SOURCE_DIR}/princomp.gplt"
        VERBATIM
        DEPENDS princomp property-config.gplt princomp.gplt ${demobase}.xml
    )
endforeach(i)

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
                -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'" -e "SIGMA=${sigma}"
                "${CMAKE_CURRENT_SOURCE_DIR}/angular.gplt"
        VERBATIM
        DEPENDS angular property-config.gplt angular.gplt ${demobase}.xml
    )
endforeach(letter)

foreach(letter a b c)
    set(basename edge-length-${letter})
    set(demobase demograph-${letter})
    set(sigma 5.0)
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/edge-length"
                -k gaussian -w ${sigma} -p 512 -o ${basename}.txt ${demobase}.xml
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'" -e "SIGMA=${sigma}"
                "${CMAKE_CURRENT_SOURCE_DIR}/edge-length.gplt"
        VERBATIM
        DEPENDS edge-length property-config.gplt edge-length.gplt ${demobase}.xml
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
    add_custom_command(
        OUTPUT rdf-local-0-${letter}.txt rdf-local-1-${letter}.txt rdf-local-2-${letter}.txt
               rdf-local-3-${letter}.txt rdf-local-4-${letter}.txt rdf-local-5-${letter}.txt
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/rdf-local"
                -k gaussian -w 10.0 -p 1024 -o "rdf-local-%-${letter}.txt" demograph-${letter}.xml
        VERBATIM
        DEPENDS rdf-local demograph-${letter}.xml
    )
    foreach(i RANGE 0 5)
        list(APPEND auxlist rdf-local-${i}-${letter}.pgf)
        add_custom_command(
            OUTPUT rdf-local-${i}-${letter}.pgf
            COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                    -e "OUTFILE='rdf-local-${i}-${letter}.pgf'" -e "INFILE='rdf-local-${i}-${letter}.txt'"
                    -e "XSTEP=5" -e "XMAX=25"
                    "${CMAKE_CURRENT_SOURCE_DIR}/rdf-local.gplt"
            VERBATIM
            DEPENDS rdf-local property-config.gplt rdf-local.gplt rdf-local-${i}-${letter}.txt
        )
    endforeach(i)
endforeach(letter)

set(gplt_tension_letters "" a    b    c    d   )
set(gplt_tension_xmin    "" 0.65 0.40 0.00 0.00)
set(gplt_tension_xmax    "" 1.05 1.60 2.50 2.00)
set(gplt_tension_xstep   "" 0.10 0.50 0.50 0.50)
foreach(i RANGE 1 4)
    list(GET gplt_tension_letters ${i} letter)
    set(basename tension-${letter})
    set(demobase demograph-${letter})
    list(GET gplt_tension_xmin  ${i} xmin)
    list(GET gplt_tension_xmax  ${i} xmax)
    list(GET gplt_tension_xstep ${i} xstep)
    set(sigma 0.01)
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/tension"
                -k gaussian -w ${sigma} -p 512 -o ${basename}.txt ${demobase}.xml
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'"
                -e "XMIN=${xmin}" -e "XMAX=${xmax}" -e "XSTEP=${xstep}" -e "SIGMA=${sigma}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tension.gplt"
        VERBATIM
        DEPENDS tension property-config.gplt tension.gplt ${demobase}.xml
    )
endforeach(i)

add_custom_command(
    OUTPUT thingy-native.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=E9d0JuAYzEWD"
            "${PROJECT_BINARY_DIR}/src/generators/lindenmayer" -n 200 -o thingy-native.xml
    VERBATIM
    DEPENDS lindenmayer
)

add_custom_command(
    OUTPUT thingy-fmmm.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=dFGSzl9peQ4a"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o thingy-fmmm.xml thingy-native.xml
    VERBATIM
    DEPENDS force thingy-native.xml
)

add_custom_command(
    OUTPUT thingy-stress.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=CoHAaQeZmDs9"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a stress -o thingy-stress.xml thingy-native.xml
    VERBATIM
    DEPENDS force thingy-native.xml
)

add_custom_command(
    OUTPUT thingy-phantom.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=Xn7BUZU2Q49H"
            "${PROJECT_BINARY_DIR}/src/layouts/phantom" -o thingy-phantom.xml thingy-native.xml
    VERBATIM
    DEPENDS phantom thingy-native.xml
)

add_custom_command(
    OUTPUT thingy-random-uniform.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=xa1DS0WYfgOn"
            "${PROJECT_BINARY_DIR}/src/layouts/random" -d uniform -o thingy-random-uniform.xml thingy-native.xml
    VERBATIM
    DEPENDS random thingy-native.xml
)

add_custom_command(
    OUTPUT thingy-random-normal.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=aoKSH4g1c4hl"
            "${PROJECT_BINARY_DIR}/src/layouts/random" -d normal -o thingy-random-normal.xml thingy-native.xml
    VERBATIM
    DEPENDS random thingy-native.xml
)

list(APPEND auxlist thingy.tikz)
add_custom_command(
    OUTPUT thingy.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o thingy.tikz thingy-native.xml
    VERBATIM
    DEPENDS picture thingy-native.xml
)

set(regression_binflags -b 8  -b 16  -b 32  -b 64  -b 128  -b 256  -b 512)
foreach(layout in native fmmm stress phantom random-uniform random-normal)
    add_custom_command(
        OUTPUT entropy-regression-${layout}.json entropy-regression-${layout}-auto.json
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/rdf-global"
                -k boxed ${regression_binflags} -o NULL -m entropy-regression-${layout}.json thingy-${layout}.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/rdf-global"
                -k boxed -o NULL -m entropy-regression-${layout}-auto.json thingy-${layout}.xml
        VERBATIM
        DEPENDS rdf-global thingy-${layout}.xml
    )
    add_custom_command(
        OUTPUT entropy-regression-${layout}.txt entropy-regression-${layout}.gplt
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/prepare-entropy-regression.py"
                -k bincount -l ${layout}
                -o entropy-regression-${layout}.txt -r entropy-regression-${layout}.gplt
                -a entropy-regression-${layout}-auto.json entropy-regression-${layout}.json
        VERBATIM
        DEPENDS entropy-regression-${layout}.json entropy-regression-${layout}-auto.json
                "${PROJECT_SOURCE_DIR}/utils/prepare-entropy-regression.py"
    )
endforeach(layout)

list(APPEND auxlist entropy-regression.pgf)
add_custom_command(
    OUTPUT entropy-regression.pgf
    COMMAND "${GNUPLOT_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/entropy-regression.gplt"
    VERBATIM
    DEPENDS entropy-regression.gplt
            entropy-regression-native.txt         entropy-regression-native.gplt
            entropy-regression-fmmm.txt           entropy-regression-fmmm.gplt
            entropy-regression-stress.txt         entropy-regression-stress.gplt
            entropy-regression-phantom.txt        entropy-regression-phantom.gplt
            entropy-regression-random-uniform.txt entropy-regression-random-uniform.gplt
            entropy-regression-random-normal.txt  entropy-regression-random-normal.gplt
)

list(APPEND auxlist nn-structure-total.pdf nn-structure-shared.pdf)
add_custom_command(
    OUTPUT nn-structure-total.pdf nn-structure-shared.pdf
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/nn-dump/"
    COMMAND "${Python3_EXECUTABLE}" -m driver.model --model "--debugdir=${CMAKE_CURRENT_BINARY_DIR}/nn-dump/"
            "--configdir=${MSC_CONFIG_DIR}" "--datadir=${MSC_DATA_DIR}" "--bindir=${PROJECT_BINARY_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_CURRENT_BINARY_DIR}/nn-dump/total.pdf" "${CMAKE_CURRENT_BINARY_DIR}/nn-structure-total.pdf"
    COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_CURRENT_BINARY_DIR}/nn-dump/shared.pdf" "${CMAKE_CURRENT_BINARY_DIR}/nn-structure-shared.pdf"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/nn-dump/"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
    DEPENDS omnibus
)

set(slides_kit_picfiles_dynamic ${auxlist} CACHE INTERNAL "")
set(slides_kit_picfiles ${slides_kit_picfiles_static} ${slides_kit_picfiles_dynamic} CACHE INTERNAL "")
add_custom_target(slides-kit-pics DEPENDS ${slides_kit_picfiles_dynamic})
