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
        <h1>Graph Overview</h1>
        <xsl:choose>
          <xsl:when test="count(graphs/graph) = 0">
            <p class="alert">Sorry, there ain't no graphs.</p>
          </xsl:when>
          <xsl:otherwise>
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
            <table class="pretty">
              <tr>
                <th class="clicksort" data-sort-type="s">ID</th>
                <th class="clicksort" data-sort-type="d">Generator</th>
                <th class="clicksort" data-sort-type="d">Size</th>
                <th class="clicksort" data-sort-type="d">Nodes</th>
                <th class="clicksort" data-sort-type="d">Edges</th>
                <th class="clicksort" data-sort-type="f">Sparsity</th>
                <th class="hiding" data-hide="x-layo">
                  <xsl:attribute name="colspan">
                    <xsl:value-of select="count(/root/all-layouts/layout)" />
                  </xsl:attribute>
                  <xsl:text>Layouts</xsl:text>
                </th>
                <th class="hiding" data-hide="x-prop">
                  <xsl:attribute name="colspan">
                    <xsl:value-of select="count(/root/all-properties/property)" />
                  </xsl:attribute>
                  <xsl:text>Properties</xsl:text>
                </th>
              </tr>
              <tr>
                <th>&#x2014;</th>
                <th>&#x2014;</th>
                <th>&#x2014;</th>
                <th>1</th>
                <th>1</th>
                <th>%</th>
                <xsl:for-each select="/root/all-layouts/layout">
                  <th class="x-layo"><span><xsl:value-of select="text()" /></span></th>
                </xsl:for-each>
                <xsl:for-each select="/root/all-properties/property">
                  <th class="x-prop"><span><xsl:value-of select="text()" /></span></th>
                </xsl:for-each>
              </tr>
              <xsl:for-each select="graphs/graph">
                <xsl:variable name="graphid"><xsl:value-of select="@id" /></xsl:variable>
                <tr>
                  <xsl:attribute name="id"><xsl:value-of select="$graphid" /></xsl:attribute>
                  <td>
                    <a>
                      <xsl:attribute name="href">
                        <xsl:value-of select="concat('/graphs/', $graphid, '/')" />
                      </xsl:attribute>
                      <xsl:value-of select="substring($graphid, 1, 8)" />
                    </a>
                  </td>
                  <td>
                    <xsl:variable name="constant">
                      <xsl:value-of select="generator" />
                    </xsl:variable>
                    <xsl:variable name="sortkey">
                      <xsl:value-of select="/root/all-generators/generator[text() = $constant]/@key" />
                    </xsl:variable>
                    <xsl:attribute name="data-sort-key"><xsl:value-of select="$sortkey" /></xsl:attribute>
                    <xsl:value-of select="generator" />
                  </td>
                  <td>
                    <xsl:variable name="constant">
                      <xsl:value-of select="size" />
                    </xsl:variable>
                    <xsl:variable name="sortkey">
                      <xsl:value-of select="/root/all-sizes/size[text() = $constant]/@key" />
                    </xsl:variable>
                    <xsl:attribute name="data-sort-key"><xsl:value-of select="$sortkey" /></xsl:attribute>
                    <xsl:value-of select="size" />
                  </td>
                  <td class="number-int"><xsl:value-of select="nodes" /></td>
                  <td class="number-int"><xsl:value-of select="edges" /></td>
                  <td class="number-percent"><xsl:value-of select="sparsity" /></td>
                  <xsl:for-each select="/root/all-layouts/layout">
                    <xsl:variable name="layo-enum">
                      <xsl:value-of select="text()" />
                    </xsl:variable>
                    <xsl:variable name="layo-json">
                      <xsl:value-of select="translate(text(), '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz')" />
                    </xsl:variable>
                    <xsl:variable name="layoutid">
                      <xsl:value-of select="/root/graphs/graph[@id = $graphid]/layouts/layout[@layout = $layo-enum]/@id" />
                    </xsl:variable>
                    <td class="x-layo">
                      <xsl:if test="$layoutid != ''">
                        <a>
                          <xsl:attribute name="href">
                            <!-- TODO: Get away from this deprecated URL scheme and refer to layouts by-ID instead. -->
                            <xsl:value-of select="concat('/graphs/', $graphid, '/', $layo-json, '/')" />
                          </xsl:attribute>
                          <xsl:value-of select="substring($layoutid, 1, 8)" />
                        </a>
                      </xsl:if>
                    </td>
                  </xsl:for-each>
                  <xsl:for-each select="/root/all-properties/property">
                    <xsl:variable name="prop">
                      <xsl:value-of select="translate(text(), '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz')" />
                    </xsl:variable>
                    <td class="x-prop">
                      <a>
                        <xsl:attribute name="href">
                          <xsl:value-of select="concat('/properties/', $graphid, '/', $prop, '/')" />
                        </xsl:attribute>
                        <xsl:value-of select="'goto'" />
                      </a>
                    </td>
                  </xsl:for-each>
                </tr>
              </xsl:for-each>
            </table>
          </xsl:otherwise>
        </xsl:choose>
      </body>
    </html>
  </xsl:template>

</xsl:transform>
