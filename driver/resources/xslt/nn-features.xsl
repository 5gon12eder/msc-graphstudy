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
        <h1>Neural Network Feature Vector</h1>
        <p>
          The features were extracted on <xsl:value-of select="timestamp" /> (approximately).
        </p>
        <p>
          Inputs to the network are normalized by subtracting the arithmethic mean and then dividing by the standard
          deviation of the feature.
        </p>
        <h2>Features of Graphs (<var>N</var> = <xsl:value-of select="count(features[@of = 'graph']/feature)" />)</h2>
        <xsl:apply-templates select="features[@of = 'graph']" />
        <h2>Features of Layouts (<var>N</var> = <xsl:value-of select="count(features[@of = 'layout']/feature)" />)</h2>
        <xsl:apply-templates select="features[@of = 'layout']" />
      </body>
    </html>
  </xsl:template>

  <xsl:template match="features">
    <table class="pretty">
      <tr>
        <th data-sort-type="d">Index</th>
        <th data-sort-type="s">Name</th>
        <th data-sort-type="f">Arithmetic Mean</th>
        <th data-sort-type="f">Standard Deviation</th>
        <xsl:if test="/root/this">
          <xsl:choose>
            <xsl:when test="@of = 'graph'">
              <th data-sort-type="f"><xsl:value-of select="substring(/root/this/graph, 1, 8)" /></th>
            </xsl:when>
            <xsl:when test="@of = 'layout'">
              <th data-sort-type="f"><xsl:value-of select="substring(/root/this/lhs, 1, 8)" /></th>
              <th data-sort-type="f"><xsl:value-of select="substring(/root/this/rhs, 1, 8)" /></th>
              <th data-sort-type="f">Difference</th>
            </xsl:when>
          </xsl:choose>
        </xsl:if>
      </tr>
      <tr>
        <th>&#x2014;</th>
        <th>&#x2014;</th>
        <th>AU</th>
        <th>AU</th>
        <xsl:if test="/root/this">
          <xsl:choose>
            <xsl:when test="@of = 'graph'">
              <th>100 &#x00D7; &#x03C3;</th>
            </xsl:when>
            <xsl:when test="@of = 'layout'">
              <th>100 &#x00D7; &#x03C3;</th>
              <th>100 &#x00D7; &#x03C3;</th>
              <th>100 &#x00D7; &#x03C3;</th>
            </xsl:when>
          </xsl:choose>
        </xsl:if>
      </tr>
      <xsl:for-each select="feature">
        <xsl:sort select="@index" data-type="number" />
        <tr>
          <td class="number-int"><xsl:value-of select="@index" /></td>
          <td style="text-align:left"><xsl:value-of select="name" /></td>
          <td class="number-float"><xsl:value-of select="mean" /></td>
          <td class="number-float"><xsl:value-of select="stdev" /></td>
          <xsl:if test="/root/this">
            <xsl:choose>
              <xsl:when test="../@of = 'graph'">
                <td class="number-100"><xsl:value-of select="this" /></td>
              </xsl:when>
              <xsl:when test="../@of = 'layout'">
                <td class="number-100"><xsl:value-of select="this-lhs" /></td>
                <td class="number-100"><xsl:value-of select="this-rhs" /></td>
                <td class="number-100"><xsl:value-of select="this-diff" /></td>
              </xsl:when>
            </xsl:choose>
          </xsl:if>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

</xsl:transform>
