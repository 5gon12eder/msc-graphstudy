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

set(report_picfiles_static
    angular-explanation.tex
    gen-lindenmayer-clique.tex
    gen-lindenmayer-grid.tex
    gen-lindenmayer-ring.tex
    gen-lindenmayer-singleton.tex
    gen-lindenmayer-star.tex
    gen-lindenmayer-wheel.tex
    gen-mosaic-flower.tex
    gen-mosaic-shape.tex
    gen-mosaic-star.tex
    intro-limits-bends.tex
    intro-limits-kinks.tex
    intro-limits-labels.tex
    intro-limits-okay.tex
    intro-vertical.tikz
    methodology-pipeline.tex
    webui-compare.png
    CACHE INTERNAL ""
)

set(auxlist)

list(APPEND auxlist romanesco.jpg)
add_custom_command(
    OUTPUT romanesco.jpg
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/download.py"
            "--checksum=sha256:44ebb4ac00d01470273d9176343adbc7bc98eca0f82d0c88dffc5fb56e041cb0"
            "--output=romanesco.jpg"
            "https://upload.wikimedia.org/wikipedia/commons/4/4f/Fractal_Broccoli.jpg"
    VERBATIM
)

list(APPEND auxlist grid.tikz)
add_custom_command(
    OUTPUT grid.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=xpqXY5GGPLkf"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 200 -o grid.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o grid.tikz grid.xml
    BYPRODUCTS grid.xml
    VERBATIM
    DEPENDS grid picture
)

list(APPEND auxlist torus1.tikz)
add_custom_command(
    OUTPUT torus1.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=CJxghJmRkO2f"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -t 1 -n 100 -o torus1-graph.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o torus1-layout.xml torus1-graph.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o torus1.tikz torus1-layout.xml
    BYPRODUCTS torus1-graph.xml torus1-layout.xml
    VERBATIM
    DEPENDS grid force picture
)

list(APPEND auxlist torus2.tikz)
add_custom_command(
    OUTPUT torus2.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=wpuXPn0QiIjo"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -t 2 -n 100 -o torus2-graph.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o torus2-layout.xml torus2-graph.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o torus2.tikz torus2-layout.xml
    BYPRODUCTS torus2-graph.xml torus2-layout.xml
    VERBATIM
    DEPENDS grid force picture
)

foreach(ar rome north randdag)
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

set(lindenmayer_seeds "" ALto8GN2n6S8 DuiWEBYg0yZy)
set(lindenmayer_suffixes "" a b)
foreach(i RANGE 1 2)
    list(GET lindenmayer_seeds ${i} seed)
    list(GET lindenmayer_suffixes ${i} suffix)
    set(name lindenmayer-${suffix})
    list(APPEND auxlist ${name}.tikz)
    add_custom_command(
        OUTPUT ${name}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=${seed}"
                "${PROJECT_BINARY_DIR}/src/generators/lindenmayer" -n 100 -o ${name}.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o ${name}.tikz ${name}.xml
        BYPRODUCTS ${name}.xml
        VERBATIM
        DEPENDS lindenmayer picture
    )
endforeach(i)

set(quasi_seeds "" "" "" 47ZHSVmTnR9H 1WrdGis9vbjK dtcQeD0JwRp7 rRsdGoXOYr2j)
foreach(i RANGE 3 6)
    set(name quasi${i}d)
    list(GET quasi_seeds ${i} seed)
    list(APPEND auxlist ${name}.tikz)
    add_custom_command(
        OUTPUT ${name}.tikz
        COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=${seed}"
                "${PROJECT_BINARY_DIR}/src/generators/quasi" -h ${i} -n 1000 -o ${name}.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
                "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o ${name}.tikz ${name}.xml
        BYPRODUCTS ${name}.xml
        VERBATIM
        DEPENDS quasi picture
    )
endforeach(i)

list(APPEND auxlist mosaic1.tikz)
add_custom_command(
    OUTPUT mosaic1.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=QP25b4RxFNON"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -n 100 -o mosaic1.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o mosaic1.tikz mosaic1.xml
    BYPRODUCTS mosaic1.xml
    VERBATIM
    DEPENDS mosaic picture
)

list(APPEND auxlist mosaic2.tikz)
add_custom_command(
    OUTPUT mosaic2.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=YWsC3DDTBcDq"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -s -n 100 -o mosaic2.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o mosaic2.tikz mosaic2.xml
    BYPRODUCTS mosaic2.xml
    VERBATIM
    DEPENDS mosaic picture
)

list(APPEND auxlist bottle.tex bottle.tikz)
add_custom_command(
    OUTPUT bottle.tex bottle.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=TlyMEd0mpaox"
            "${PROJECT_BINARY_DIR}/src/generators/bottle" -n 1000 -o bottle.xml -m bottle.json
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/get-graph-info.py" -o bottle.tex bottle.json
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o bottle.tikz bottle.xml
    BYPRODUCTS bottle.json bottle.xml
    VERBATIM
    DEPENDS bottle picture get-graph-info.py
)

add_executable(gen-quasi-concept-tikz gen-quasi-concept-tikz.cxx)
target_link_libraries(gen-quasi-concept-tikz PUBLIC common)

list(APPEND auxlist gen-quasi-concept.tex)
add_custom_command(
    OUTPUT gen-quasi-concept.tex
    COMMAND gen-quasi-concept-tikz > gen-quasi-concept.tex
    DEPENDS gen-quasi-concept-tikz
    VERBATIM
)

add_custom_command(
    OUTPUT worse-parent.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=N0ukQGf0Kgq7"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 250 -o worse-parent.xml
    VERBATIM
    DEPENDS grid
)

list(
    APPEND auxlist
           perturb-00000.tikz
           perturb-00500.tikz
           perturb-01000.tikz
           perturb-02000.tikz
           perturb-05000.tikz
           perturb-10000.tikz
)

add_custom_command(
    OUTPUT perturb-00000.tikz perturb-00500.tikz perturb-01000.tikz
           perturb-02000.tikz perturb-05000.tikz perturb-10000.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=8lefwq8Ha0gN"
            "${PROJECT_BINARY_DIR}/src/unitrans/perturb" -o "perturb-%.xml"
            -r 0.00 -r 0.05 -r 0.10 -r 0.20 -r 0.50 -r 1.00 worse-parent.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o perturb-00000.tikz perturb-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o perturb-00500.tikz perturb-00500.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o perturb-01000.tikz perturb-01000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o perturb-02000.tikz perturb-02000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o perturb-05000.tikz perturb-05000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o perturb-10000.tikz perturb-10000.xml
    VERBATIM
    BYPRODUCTS perturb-00000.xml perturb-00500.xml perturb-01000.xml
               perturb-02000.xml perturb-05000.xml perturb-10000.xml
    DEPENDS perturb picture worse-parent.xml
)

list(
    APPEND auxlist
           flip-nodes-00000.tikz
           flip-nodes-00500.tikz
           flip-nodes-01000.tikz
           flip-nodes-02000.tikz
           flip-nodes-05000.tikz
           flip-nodes-10000.tikz
)

add_custom_command(
    OUTPUT flip-nodes-00000.tikz flip-nodes-00500.tikz flip-nodes-01000.tikz
           flip-nodes-02000.tikz flip-nodes-05000.tikz flip-nodes-10000.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=pv3mUhOcmOIf"
            "${PROJECT_BINARY_DIR}/src/unitrans/flip-nodes" -o "flip-nodes-%.xml"
            -r 0.00 -r 0.05 -r 0.10 -r 0.20 -r 0.50 -r 1.00 worse-parent.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-nodes-00000.tikz flip-nodes-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-nodes-00500.tikz flip-nodes-00500.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-nodes-01000.tikz flip-nodes-01000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-nodes-02000.tikz flip-nodes-02000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-nodes-05000.tikz flip-nodes-05000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-nodes-10000.tikz flip-nodes-10000.xml
    VERBATIM
    BYPRODUCTS flip-nodes-00000.xml flip-nodes-00500.xml flip-nodes-01000.xml
               flip-nodes-02000.xml flip-nodes-05000.xml flip-nodes-10000.xml
    DEPENDS flip-nodes picture worse-parent.xml
)

list(
    APPEND auxlist
           flip-edges-00000.tikz
           flip-edges-00500.tikz
           flip-edges-01000.tikz
           flip-edges-02000.tikz
           flip-edges-05000.tikz
           flip-edges-10000.tikz
)

add_custom_command(
    OUTPUT flip-edges-00000.tikz flip-edges-00500.tikz flip-edges-01000.tikz
           flip-edges-02000.tikz flip-edges-05000.tikz flip-edges-10000.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=LjEjwUe57ixs"
            "${PROJECT_BINARY_DIR}/src/unitrans/flip-edges" -o "flip-edges-%.xml"
            -r 0.00 -r 0.05 -r 0.10 -r 0.20 -r 0.50 -r 1.00 worse-parent.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-edges-00000.tikz flip-edges-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-edges-00500.tikz flip-edges-00500.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-edges-01000.tikz flip-edges-01000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-edges-02000.tikz flip-edges-02000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-edges-05000.tikz flip-edges-05000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o flip-edges-10000.tikz flip-edges-10000.xml
    VERBATIM
    BYPRODUCTS flip-edges-00000.xml flip-edges-00500.xml flip-edges-01000.xml
               flip-edges-02000.xml flip-edges-05000.xml flip-edges-10000.xml
    DEPENDS flip-edges picture worse-parent.xml
)

list(
    APPEND auxlist
           movlsq-00000.tikz
           movlsq-00500.tikz
           movlsq-01000.tikz
           movlsq-02000.tikz
           movlsq-05000.tikz
           movlsq-10000.tikz
)

add_custom_command(
    OUTPUT movlsq-00000.tikz movlsq-00500.tikz movlsq-01000.tikz
           movlsq-02000.tikz movlsq-05000.tikz movlsq-10000.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=hdc1B39REE6V"
            "${PROJECT_BINARY_DIR}/src/unitrans/movlsq" -o "movlsq-%.xml" --meta=movlsq.json
            -r 0.00 -r 0.05 -r 0.10 -r 0.20 -r 0.50 -r 1.00 worse-parent.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o movlsq-00000.tikz movlsq-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o movlsq-00500.tikz movlsq-00500.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o movlsq-01000.tikz movlsq-01000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o movlsq-02000.tikz movlsq-02000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o movlsq-05000.tikz movlsq-05000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o movlsq-10000.tikz movlsq-10000.xml
    VERBATIM
    BYPRODUCTS movlsq.json
               movlsq-00000.xml movlsq-00500.xml movlsq-01000.xml movlsq-02000.xml movlsq-05000.xml movlsq-10000.xml
    DEPENDS movlsq picture worse-parent.xml
)

add_custom_command(
    OUTPUT problem-a.xml problem-b.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=XGyK9HeEmiWN"
            "${PROJECT_BINARY_DIR}/src/generators/lindenmayer" -n 100 -o problem.xml
    COMMAND "${CMAKE_COMMAND}" -E copy problem.xml problem-a.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o problem-b.xml problem.xml
    VERBATIM
    BYPRODUCTS problem.xml
    DEPENDS lindenmayer force rotate
)

list(APPEND auxlist problem-linear-00000.tikz problem-linear-05000.tikz problem-linear-10000.tikz)
add_custom_command(
    OUTPUT problem-linear-00000.tikz problem-linear-05000.tikz problem-linear-10000.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -o "problem-linear-%.xml" -r 0.0 -r 0.5 -r 1.0
            problem-a.xml problem-b.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o problem-linear-00000.tikz
            problem-linear-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o problem-linear-05000.tikz
            problem-linear-05000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o problem-linear-10000.tikz
            problem-linear-10000.xml
    VERBATIM
    BYPRODUCTS problem-linear-00000.xml problem-linear-05000.xml problem-linear-10000.xml
    DEPENDS interpol picture problem-a.xml problem-b.xml
)

list(APPEND auxlist problem-xlinear-00000.tikz problem-xlinear-05000.tikz problem-xlinear-10000.tikz)
add_custom_command(
    OUTPUT problem-xlinear-00000.tikz problem-xlinear-05000.tikz problem-xlinear-10000.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -o "problem-xlinear-%.xml" -r 0.0 -r 0.5 -r 1.0 --clever
            problem-a.xml problem-b.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o problem-xlinear-00000.tikz problem-xlinear-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o problem-xlinear-05000.tikz problem-xlinear-05000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o problem-xlinear-10000.tikz problem-xlinear-10000.xml
    VERBATIM
    BYPRODUCTS problem-xlinear-00000.xml problem-xlinear-05000.xml problem-xlinear-10000.xml
    DEPENDS interpol picture problem-a.xml problem-b.xml
)

set(interparent "${CMAKE_CURRENT_SOURCE_DIR}/inter-parent.xml.bz2")

add_custom_command(
    OUTPUT inter-parent-fmmm.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=bV6xboqlXWAR"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o inter-parent-fmmm.xml "${interparent}"
    VERBATIM
    DEPENDS force "${interparent}"
)

add_custom_command(
    OUTPUT inter-parent-stress.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=UuQ7ABfpfwLM"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a stress -o inter-parent-stress.xml "${interparent}"
    VERBATIM
    DEPENDS force "${interparent}"
)

add_custom_command(
    OUTPUT inter-parent-random-uniform.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=0hwnhwoTMc79"
            "${PROJECT_BINARY_DIR}/src/layouts/random" -d uniform -o inter-parent-random-uniform.xml "${interparent}"
    VERBATIM
    DEPENDS force "${interparent}"
)

list(
    APPEND auxlist
           pathological-xlinear-00000.tikz
           pathological-xlinear-02500.tikz
           pathological-xlinear-05000.tikz
           pathological-xlinear-07500.tikz
           pathological-xlinear-10000.tikz
)

add_custom_command(
    OUTPUT pathological-xlinear-00000.tikz
           pathological-xlinear-02500.tikz
           pathological-xlinear-05000.tikz
           pathological-xlinear-07500.tikz
           pathological-xlinear-10000.tikz
    # We process the inputs through 'import' because the hand-written GraphML files are not normalized.
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/generators/import" -f graphml -l -o pathological-xlinear-aa.xml
            "${CMAKE_CURRENT_SOURCE_DIR}/pathological-xlinear-a.xml"
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/generators/import" -f graphml -l -o pathological-xlinear-bb.xml
            "${CMAKE_CURRENT_SOURCE_DIR}/pathological-xlinear-b.xml"
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -c -o "pathological-xlinear-%.xml"
            -r 0.00 -r 0.25 -r 0.50 -r 0.75 -r 1.00
            pathological-xlinear-aa.xml pathological-xlinear-bb.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o pathological-xlinear-00000.tikz pathological-xlinear-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o pathological-xlinear-02500.tikz pathological-xlinear-02500.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o pathological-xlinear-05000.tikz pathological-xlinear-05000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o pathological-xlinear-07500.tikz pathological-xlinear-07500.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o pathological-xlinear-10000.tikz pathological-xlinear-10000.xml
    VERBATIM
    BYPRODUCTS pathological-xlinear-aa.xml pathological-xlinear-bb.xml
               pathological-xlinear-00000.xml
               pathological-xlinear-02500.xml
               pathological-xlinear-05000.xml
               pathological-xlinear-07500.xml
               pathological-xlinear-10000.xml
    DEPENDS import interpol picture pathological-xlinear-a.xml pathological-xlinear-b.xml
)

list(
    APPEND auxlist
           xlinear-pp-00000.tikz
           xlinear-pp-02000.tikz
           xlinear-pp-04000.tikz
           xlinear-pp-06000.tikz
           xlinear-pp-08000.tikz
           xlinear-pp-10000.tikz
)

add_custom_command(
    OUTPUT xlinear-pp-00000.tikz xlinear-pp-02000.tikz xlinear-pp-04000.tikz
           xlinear-pp-06000.tikz xlinear-pp-08000.tikz xlinear-pp-10000.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -c -o "xlinear-pp-%.xml"
            -r 0.00 -r 0.20 -r 0.40 -r 0.60 -r 0.80 -r 1.00 inter-parent-fmmm.xml inter-parent-stress.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o xlinear-pp-00000.tikz xlinear-pp-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o xlinear-pp-02000.tikz xlinear-pp-02000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o xlinear-pp-04000.tikz xlinear-pp-04000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o xlinear-pp-06000.tikz xlinear-pp-06000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o xlinear-pp-08000.tikz xlinear-pp-08000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o xlinear-pp-10000.tikz xlinear-pp-10000.xml
    VERBATIM
    BYPRODUCTS xlinear-pp-00000.xml xlinear-pp-02000.xml xlinear-pp-04000.xml
               xlinear-pp-06000.xml xlinear-pp-08000.xml xlinear-pp-10000.xml
    DEPENDS interpol picture inter-parent-fmmm.xml inter-parent-stress.xml
)

list(
    APPEND auxlist
           xlinear-pg-00000.tikz
           xlinear-pg-02000.tikz
           xlinear-pg-04000.tikz
           xlinear-pg-06000.tikz
           xlinear-pg-08000.tikz
           xlinear-pg-10000.tikz
)

add_custom_command(
    OUTPUT xlinear-pg-00000.tikz xlinear-pg-02000.tikz xlinear-pg-04000.tikz
           xlinear-pg-06000.tikz xlinear-pg-08000.tikz xlinear-pg-10000.tikz
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/bitrans/interpol" -c -o "xlinear-pg-%.xml"
            -r 0.00 -r 0.20 -r 0.40 -r 0.60 -r 0.80 -r 1.00 inter-parent-fmmm.xml inter-parent-random-uniform.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o xlinear-pg-00000.tikz xlinear-pg-00000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o xlinear-pg-02000.tikz xlinear-pg-02000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o xlinear-pg-04000.tikz xlinear-pg-04000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o xlinear-pg-06000.tikz xlinear-pg-06000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o xlinear-pg-08000.tikz xlinear-pg-08000.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o xlinear-pg-10000.tikz xlinear-pg-10000.xml
    VERBATIM
    BYPRODUCTS xlinear-pg-00000.xml xlinear-pg-02000.xml xlinear-pg-04000.xml
               xlinear-pg-06000.xml xlinear-pg-08000.xml xlinear-pg-10000.xml
    DEPENDS interpol picture inter-parent-fmmm.xml inter-parent-random-uniform.xml
)

list(
    APPEND auxlist
           force-char-native.tikz
           force-char-fmmm.tikz
           force-char-knot.tikz
           force-char-stress.tikz
)

add_custom_command(
    OUTPUT force-char-native.tikz force-char-fmmm.tikz force-char-knot.tikz force-char-stress.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=JyiYntNS8inx"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 500 -o force-char-native-pre.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=2eeCY7p892UT"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o force-char-fmmm-pre.xml force-char-native-pre.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=5YIklbfs0Pug"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o force-char-knot-pre.xml force-char-native-pre.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=8UDXoLtY1xEs"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a stress -o force-char-stress-pre.xml force-char-native-pre.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/bitrans/interpol"
            -c -r 0.5 -o force-char-native.xml force-char-native-pre.xml force-char-native-pre.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/bitrans/interpol"
            -c -r 0.5 -o force-char-fmmm.xml force-char-fmmm-pre.xml force-char-fmmm-pre.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/bitrans/interpol"
            -c -r 0.5 -o force-char-knot.xml force-char-knot-pre.xml force-char-knot-pre.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/bitrans/interpol"
            -c -r 0.5 -o force-char-stress.xml force-char-stress-pre.xml force-char-stress-pre.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o force-char-native.tikz force-char-native.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o force-char-fmmm.tikz force-char-fmmm.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o force-char-knot.tikz force-char-knot.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o force-char-stress.tikz force-char-stress.xml
    BYPRODUCTS force-char-native.xml force-char-fmmm.xml force-char-knot.xml force-char-stress.xml
               force-char-native-pre.xml force-char-fmmm-pre.xml force-char-knot-pre.xml force-char-stress-pre.xml
    VERBATIM
    DEPENDS grid force picture
)

list(APPEND auxlist force-comp.tex force-comp-fmmm.tikz force-comp-stress.tikz)
add_custom_command(
    OUTPUT force-comp.tex force-comp-fmmm.tikz force-comp-stress.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=IYHZ0YaYBsqB"
            "${PROJECT_BINARY_DIR}/src/generators/grid" -n 300 -t 2 -m force-comp.json -o force-comp.xml
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/get-graph-info.py" -o force-comp.tex force-comp.json
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=ixvu236u6Len"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o force-comp-fmmm.xml force-comp.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=Hf8YM9N1t6Ez"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a stress -o force-comp-stress.xml force-comp.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o force-comp-fmmm.tikz force-comp-fmmm.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED=
            "${PROJECT_BINARY_DIR}/src/visualizations/picture" --tikz -o force-comp-stress.tikz force-comp-stress.xml
    VERBATIM
    BYPRODUCTS force-comp.json force-comp.xml force-comp-fmmm.xml force-comp-stress.xml
    DEPENDS interpol picture inter-parent-fmmm.xml inter-parent-random-uniform.xml get-graph-info.py
)

list(
    APPEND auxlist
           garbage-native.tikz
           garbage-random-uniform.tikz
           garbage-random-normal.tikz
           garbage-phantom.tikz
           garbage-the-phantom.tikz
)

add_custom_command(
    OUTPUT garbage-native.tikz garbage-random-uniform.tikz garbage-random-normal.tikz garbage-phantom.tikz
           garbage-the-phantom.tikz
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=vtGS19PwTwD9"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -s -n 50 -o garbage-native.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=hbUI6GBnzgjF"
            "${PROJECT_BINARY_DIR}/src/layouts/random" -d uniform -o garbage-random-uniform.xml garbage-native.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=1mlspYHTOEvp"
            "${PROJECT_BINARY_DIR}/src/layouts/random" -d normal -o garbage-random-normal.xml garbage-native.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=QyHdP61PWbcO" "MSC_DUMP_PHANTOM=garbage-the-phantom.xml"
            "${PROJECT_BINARY_DIR}/src/layouts/phantom" -o garbage-phantom.xml garbage-native.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o garbage-native.tikz garbage-native.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o garbage-random-uniform.tikz garbage-random-uniform.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o garbage-random-normal.tikz garbage-random-normal.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o garbage-phantom.tikz garbage-phantom.xml
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/visualizations/picture"
            --tikz -o garbage-the-phantom.tikz garbage-the-phantom.xml
    VERBATIM
    BYPRODUCTS garbage-native.xml garbage-random-uniform.xml garbage-random-normal.xml garbage-phantom.xml
               garbage-native.tikz garbage-random-uniform.tikz garbage-random-normal.tikz garbage-phantom.tikz
               garbage-the-phantom.xml garbage-the-phantom.tikz
    DEPENDS mosaic random phantom picture
)

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
            "ftp://math.nist.gov/pub/MatrixMarket2/Harwell-Boeing/bcspwr/bcspwr04.mtx.gz"
            "--output=bcspwr04.mtx.gz"
            "--checksum=sha256:2da77005782adcccfb7f0d7b0fa088263f0892f292a0a72e51ffaf44be15bb4b"
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
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=Z3bBbVRDcanC"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a stress -o demograph-d.xml bcspwr04.xml
    VERBATIM
    DEPENDS force bcspwr04.xml
)

add_custom_command(
    OUTPUT demograph-e.xml demograph-e.json
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=sU3dtHcWhCPl"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -s -n 1000 -o demograph-e.xml -m demograph-e.json
    VERBATIM
    DEPENDS mosaic
)

list(APPEND auxlist demograph-x.tex)
add_custom_command(
    OUTPUT demograph-x.xml demograph-x.tex
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=kVmJlA48naB4"
            "${PROJECT_BINARY_DIR}/src/generators/mosaic" -s -n 1000 -o demograph-x.xml -m demograph-x.json
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/get-graph-info.py" -o demograph-x.tex demograph-x.json
    VERBATIM
    BYPRODUCTS demograph-x.json
    DEPENDS mosaic get-graph-info.py
)

add_custom_command(
    OUTPUT demograph-y.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=4ez7vXI2epxC"
            "${PROJECT_BINARY_DIR}/src/layouts/force" -a fmmm -o demograph-y.xml demograph-x.xml
    VERBATIM
    DEPENDS force demograph-x.xml
)

add_custom_command(
    OUTPUT demograph-z.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=eAKFnoliA2gJ"
            "${PROJECT_BINARY_DIR}/src/layouts/phantom" -o demograph-z.xml demograph-x.xml
    VERBATIM
    DEPENDS phantom demograph-x.xml
)

add_custom_command(
    OUTPUT demograph-w.xml
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=0YEiQWSz2umF"
            "${PROJECT_BINARY_DIR}/src/unitrans/perturb" -r 0.15 -o demograph-w.xml demograph-x.xml
    VERBATIM
    DEPENDS perturb demograph-x.xml
)

set(demograph_letters "" x y z w)
set(demograph_layouts "" native fmmm phantom worse)

foreach(letter a b c d e x y z w)
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

add_custom_command(
    OUTPUT demograph-e_rdf-local.json
           rdf-local-0.txt rdf-local-1.txt rdf-local-2.txt rdf-local-3.txt rdf-local-4.txt rdf-local-5.txt
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/rdf-local"
            -k gaussian -o "rdf-local-%.txt" -m demograph-e_rdf-local.json demograph-e.xml
    VERBATIM
    DEPENDS rdf-local demograph-e.xml
)

list(APPEND auxlist demograph-e.tex)
add_custom_command(
    OUTPUT demograph-e.tex
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/get-graph-info.py"
            -o demograph-e.tex --rdf-local=demograph-e_rdf-local.json demograph-e.json
    VERBATIM
    DEPENDS get-graph-info.py demograph-e.json demograph-e_rdf-local.json
)

set(gplt_rdf_local_xmax   2.5  4.5  9.0 15.0 20.0 20.0)
set(gplt_rdf_local_xstep  1    1    2    5    5    5  )
foreach(i RANGE 0 5)
    list(GET gplt_rdf_local_xmax ${i} xmax)
    list(GET gplt_rdf_local_xstep ${i} xstep)
    set(basename rdf-local-${i})
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "PREAMBLE='${CMAKE_CURRENT_SOURCE_DIR}/property-config.gplt'"
                -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'" -e "XMAX=${xmax}" -e "XSTEP=${xstep}"
                "${CMAKE_CURRENT_SOURCE_DIR}/rdf-local.gplt"
        VERBATIM
        DEPENDS property-config.gplt rdf-local.gplt ${basename}.txt
    )
endforeach(i)

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

set(histoscale_bin_counts -b1 -b2 -b4 -b8 -b16 -b32 -b64 -b128 -b256 -b512 -b1024 -b2048 -b4096 -b8192 -b16384)

add_custom_command(
    OUTPUT histoscale-0.txt histoscale-1.txt histoscale-2.txt
           histoscale-3.txt histoscale-4.txt histoscale-5.txt
           histoscale-6.txt histoscale-7.txt histoscale-8.txt
           histoscale-9.txt histoscale-10.txt histoscale-11.txt
           histoscale-12.txt histoscale-13.txt
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/princomp"
            -1 -k boxed ${histoscale_bin_counts} -o "histoscale-%.txt" demograph-x.xml
    VERBATIM
    DEPENDS princomp demograph-x.xml
)

add_custom_command(
    OUTPUT histoscale-auto.txt
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/princomp"
            -1 -k boxed -o histoscale-auto.txt demograph-x.xml
    VERBATIM
    DEPENDS princomp demograph-x.xml
)

foreach(i RANGE 3 13)
    set(basename histoscale-${i})
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'" -e "LINE=1"
                "${CMAKE_CURRENT_SOURCE_DIR}/histoscale.gplt"
        VERBATIM
        DEPENDS histoscale.gplt ${basename}.txt
    )
endforeach(i)

list(APPEND auxlist histoscale-auto.pgf)
add_custom_command(
    OUTPUT histoscale-auto.pgf
    COMMAND "${GNUPLOT_EXECUTABLE}" -e "OUTFILE='histoscale-auto.pgf" -e "INFILE='histoscale-auto.txt'"  -e "LINE=2"
            "${CMAKE_CURRENT_SOURCE_DIR}/histoscale.gplt"
    VERBATIM
    DEPENDS histoscale.gplt histoscale-auto.txt
)

set(regression_binflags -b 8  -b 16  -b 32  -b 64  -b 128  -b 256  -b 512)
foreach(i RANGE 1 4)

    list(GET demograph_letters ${i} letter)
    list(GET demograph_layouts ${i} layout)

    add_custom_command(
        OUTPUT entropy-regression-${layout}.json entropy-regression-${layout}-auto.json
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/princomp"
                -1 -k boxed ${regression_binflags} -o NULL -m entropy-regression-${layout}.json demograph-${letter}.xml
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/princomp"
                -1 -k boxed -o NULL -m entropy-regression-${layout}-auto.json demograph-${letter}.xml
        VERBATIM
        DEPENDS princomp demograph-${letter}.xml
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

endforeach(i)

list(APPEND auxlist entropy-regression.pgf)
add_custom_command(
    OUTPUT entropy-regression.pgf
    COMMAND "${GNUPLOT_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/entropy-regression.gplt"
    VERBATIM
    DEPENDS entropy-regression.gplt
            entropy-regression-native.txt  entropy-regression-native.gplt
            entropy-regression-fmmm.txt    entropy-regression-fmmm.gplt
            entropy-regression-worse.txt   entropy-regression-worse.gplt
            entropy-regression-phantom.txt entropy-regression-phantom.gplt
)

add_custom_command(
    OUTPUT glidescale-0.txt glidescale-1.txt glidescale-2.txt
           glidescale-3.txt glidescale-4.txt glidescale-5.txt
           glidescale-6.txt glidescale-7.txt glidescale-8.txt
           glidescale-9.txt glidescale-10.txt glidescale-11.txt
           glidescale-12.txt glidescale-13.txt
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/princomp"
            -1 -k gaussian -p 512 -b1 -b2 -b4 -b8 -b16 -b32 -b64 -b128 -b256 -b512 -b 1024 -b2048 -b4096 -b8192 -b16384
            -o "glidescale-%.txt" demograph-x.xml
    VERBATIM
    DEPENDS princomp demograph-x.xml
)

add_custom_command(
    OUTPUT glidescale-auto.txt
    COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/princomp"
            -1 -k gaussian -p 512 -o glidescale-auto.txt demograph-x.xml
    VERBATIM
    DEPENDS princomp demograph-x.xml
)

foreach(i RANGE 3 13)
    set(basename glidescale-${i})
    list(APPEND auxlist ${basename}.pgf)
    add_custom_command(
        OUTPUT ${basename}.pgf
        COMMAND "${GNUPLOT_EXECUTABLE}" -e "OUTFILE='${basename}.pgf'" -e "INFILE='${basename}.txt'"  -e "LINE=1"
                "${CMAKE_CURRENT_SOURCE_DIR}/glidescale.gplt"
        VERBATIM
        DEPENDS glidescale.gplt ${basename}.txt
    )
endforeach(i)

list(APPEND auxlist glidescale-auto.pgf)
add_custom_command(
    OUTPUT glidescale-auto.pgf
    COMMAND "${GNUPLOT_EXECUTABLE}" -e "OUTFILE='glidescale-auto.pgf" -e "INFILE='glidescale-auto.txt'"  -e "LINE=2"
            "${CMAKE_CURRENT_SOURCE_DIR}/glidescale.gplt"
    VERBATIM
    DEPENDS glidescale.gplt glidescale-auto.txt
)

add_executable(adaptive-sampling adaptive-sampling.cxx)
target_link_libraries(adaptive-sampling PUBLIC common)

list(APPEND auxlist adaptive-sampling.pgf)
add_custom_command(
    OUTPUT adaptive-sampling.pgf
    COMMAND ./adaptive-sampling --adaptive > adaptive-sampling.txt
    COMMAND "${GNUPLOT_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/adaptive-sampling.gplt"
    BYPRODUCTS adaptive-sampling.txt
    VERBATIM
    DEPENDS adaptive-sampling adaptive-sampling.gplt
)

foreach(i RANGE 1 4)
    list(GET demograph_letters ${i} letter)
    list(GET demograph_layouts ${i} layout)
    add_custom_command(
        OUTPUT diffent-rdf-local-${layout}.json
        COMMAND "${CMAKE_COMMAND}" -E env MSC_RANDOM_SEED= "${PROJECT_BINARY_DIR}/src/properties/rdf-local"
                -k gaussian -o NULL -m diffent-rdf-local-${layout}.json demograph-${letter}.xml
        VERBATIM
        DEPENDS rdf-local demograph-${letter}.xml
    )
    add_custom_command(
        OUTPUT diffent-rdf-local-${layout}.txt diffent-rdf-local-${layout}.gplt
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/prepare-local-diffent-regression.py"
                -l ${layout} -o diffent-rdf-local-${layout}.txt -r diffent-rdf-local-${layout}.gplt
                diffent-rdf-local-${layout}.json
        VERBATIM
        DEPENDS diffent-rdf-local-${layout}.json
                "${PROJECT_SOURCE_DIR}/utils/prepare-local-diffent-regression.py"
    )
endforeach(i)

list(APPEND auxlist diffent-rdf-local.pgf)
add_custom_command(
    OUTPUT diffent-rdf-local.pgf
    COMMAND "${GNUPLOT_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/diffent-rdf-local.gplt"
    VERBATIM
    DEPENDS diffent-rdf-local.gplt
            diffent-rdf-local-native.txt  diffent-rdf-local-native.gplt
            diffent-rdf-local-fmmm.txt    diffent-rdf-local-fmmm.gplt
            diffent-rdf-local-worse.txt   diffent-rdf-local-worse.gplt
            diffent-rdf-local-phantom.txt diffent-rdf-local-phantom.gplt
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

set(report_picfiles_dynamic ${auxlist} CACHE INTERNAL "")
set(report_picfiles ${report_picfiles_static} ${report_picfiles_dynamic} CACHE INTERNAL "")
add_custom_target(report-pics DEPENDS ${report_picfiles_dynamic})
