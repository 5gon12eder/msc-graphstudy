<?xml version="1.0" encoding="UTF-8" ?>

<!-- Copyright (C) 2018 Karlsruhe Institute of Technology                                                            -->
<!-- Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>                                             -->
<!--                                                                                                                 -->
<!-- This program is free software: you can redistribute it and/or modify it under the terms of the GNU General      -->
<!-- Public License as published by the Free Software Foundation, either version 3 of the License, or (at your       -->
<!-- option) any later version.                                                                                      -->
<!--                                                                                                                 -->
<!-- This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the      -->
<!-- implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    -->
<!-- for more details.                                                                                               -->
<!--                                                                                                                 -->
<!-- You should have received a copy of the GNU General Public License along with this program.  If not, see         -->
<!-- <http://www.gnu.org/licenses/>.                                                                                 -->

<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" >

  <xsl:output method="html" indent="yes"/>

  <xsl:template match="root">
    <html>
      <head>
        <meta charset="UTF-8" />
        <title>Graphstudy</title>
        <link rel="shortcut icon" type="image/png" href="/graphstudy.png" />
        <link rel="stylesheet" type="text/css" href="/graphstudy.css" />
        <script type="text/javascript" src="/graphstudy.js"></script>
      </head>
      <body onload="init()">
        <h1>Tool Performance Summary</h1>
        <table class="pretty">
          <tr>
            <th class="clicksort" data-sort-type="s">Tool</th>
            <th class="clicksort" data-sort-type="d">Calls</th>
            <th class="clicksort" data-sort-type="f">Absolute</th>
            <th class="clicksort" data-sort-type="f">Relative</th>
            <th class="clicksort" data-sort-type="f">Minimum</th>
            <th class="clicksort" data-sort-type="f">Maximum</th>
            <th class="clicksort" data-sort-type="f">Median</th>
            <th class="clicksort" data-sort-type="f">Mean</th>
          </tr>
          <tr>
            <th>&#x2014;</th>
            <th>1</th>
            <th>H:MM:SS</th>
            <th>%</th>
            <th>H:MM:SS</th>
            <th>H:MM:SS</th>
            <th>H:MM:SS</th>
            <th>H:MM:SS</th>
          </tr>
          <xsl:for-each select="tool">
            <tr>
              <td><xsl:value-of select="@name" /></td>
              <td class="number-int"     ><xsl:value-of select="cnt" /></td>
              <td class="number-duration"><xsl:value-of select="abs" /></td>
              <td class="number-percent" ><xsl:value-of select="rel" /></td>
              <td class="number-duration"><xsl:value-of select="min" /></td>
              <td class="number-duration"><xsl:value-of select="max" /></td>
              <td class="number-duration"><xsl:value-of select="med" /></td>
              <td class="number-duration"><xsl:value-of select="avg" /></td>
            </tr>
          </xsl:for-each>
        </table>
      </body>
    </html>
  </xsl:template>

</xsl:transform>
