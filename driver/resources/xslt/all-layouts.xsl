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
        <h1>Layout Overview</h1>
        <xsl:choose>
          <xsl:when test="count(graphs/graph/layouts/layout) &gt; 0">
            <p>
              <xsl:text>There are </xsl:text>
              <a href="/graphs/">
                <span class="number-int"><xsl:value-of select="count(graphs/graph)" /></span>
                <xsl:text> graphs</xsl:text>
              </a>
              <xsl:text> and </xsl:text>
              <a href="/layouts/">
                <span class="number-int"><xsl:value-of select="count(graphs/graph/layouts/layout)" /></span>
                <xsl:text> layouts</xsl:text>
              </a>
              <xsl:text> in total.</xsl:text>
            </p>
            <table class="thumbgrid">
              <tr>
                <xsl:for-each select="all-layouts/layout">
                  <xsl:sort select="@key" data-type="number" />
                  <th><xsl:value-of select="." /></th>
                </xsl:for-each>
              </tr>
              <xsl:for-each select="graphs/graph">
                <xsl:variable name="graphid">
                  <xsl:value-of select="@id" />
                </xsl:variable>
                <tr>
                  <xsl:for-each select="/root/all-layouts/layout">
                    <xsl:sort select="@key" data-type="number" />
                    <xsl:variable name="layo">
                      <xsl:value-of select="." />
                    </xsl:variable>
                    <td>
                      <xsl:apply-templates select="/root/graphs/graph[@id = $graphid]/layouts/layout[@layout = $layo]" />
                    </td>
                  </xsl:for-each>
                </tr>
              </xsl:for-each>
            </table>
          </xsl:when>
          <xsl:otherwise>
            <p class="alert">Sorry, there ain't no layouts.</p>
          </xsl:otherwise>
        </xsl:choose>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="layout">
    <a>
      <xsl:attribute name="href">
        <xsl:value-of select="concat('/layouts/', @id, '/')" />
      </xsl:attribute>
      <div class="thumbnail">
        <img>
          <xsl:attribute name="title">
            <xsl:value-of select="concat(../../generator, ' / ', ../../size, ' / ', @layout)" />
          </xsl:attribute>
          <xsl:attribute name="alt">
            <xsl:value-of select="substring(@id, 1, 8)" />
          </xsl:attribute>
          <xsl:attribute name="src">
            <xsl:value-of select="concat('/layouts/', @id, '/thumbnail.png')" />
          </xsl:attribute>
        </img>
      </div>
    </a>
  </xsl:template>

</xsl:transform>
