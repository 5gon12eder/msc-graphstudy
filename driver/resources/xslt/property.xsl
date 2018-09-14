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

  <xsl:variable name="apos">&apos;</xsl:variable>
  <xsl:variable name="quot">&quot;</xsl:variable>
  <xsl:variable name="none"></xsl:variable>

  <xsl:template match="root">
    <xsl:variable name="graphid">
      <xsl:value-of select="graph/@id" />
    </xsl:variable>
    <xsl:variable name="prop">
      <xsl:value-of select="property" />
    </xsl:variable>
    <xsl:variable name="prop-json">
      <xsl:value-of select="translate(property, '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz')" />
    </xsl:variable>
    <html>
      <head>
        <meta charset="UTF-8" />
        <title>Graphstudy</title>
        <link rel="shortcut icon" type="image/png" href="/graphstudy.png" />
        <link rel="stylesheet" type="text/css" href="/graphstudy.css" />
        <script type="text/javascript" src="/graphstudy.js"></script>
      </head>
      <body onload="init()">
        <xsl:apply-templates select="all-properties">
          <xsl:with-param name="graphid" select="$graphid" />
          <xsl:with-param name="property" select="$prop" />
          <xsl:with-param name="vicinity" select="property/@vicinity" />
        </xsl:apply-templates>
        <h1>
          <span class="constant-properties"><xsl:value-of select="$prop" /></span>
          <xsl:if test="property/@vicinity">
            <xsl:value-of select="concat(' (Vicinity: ', property/@vicinity, ')')" />
          </xsl:if>
        </h1>
        <xsl:apply-templates select="graph" />
        <xsl:apply-templates select="all-layouts" />
        <h2>Entropy (Scaled Histogram)</h2>
        <xsl:call-template name="make-entropy-table-scaled-histogram">
          <xsl:with-param name="graphid" select="$graphid" />
          <xsl:with-param name="prop-json" select="$prop-json" />
        </xsl:call-template>
        <xsl:call-template name="make-entropy-figure">
          <xsl:with-param name="graphid" select="$graphid" />
          <xsl:with-param name="prop-json" select="$prop-json" />
          <xsl:with-param name="vicinity" select="property/@vicinity" />
        </xsl:call-template>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="graph">
    <h2>Summary</h2>
    <table class="horizontal">
      <tr>
        <th>Graph-ID</th>
        <td>
          <a>
            <xsl:attribute name="href"><xsl:value-of select="concat('/graphs/', @id, '/')" /></xsl:attribute>
            <xsl:value-of select="@id" />
          </a>
        </td>
      </tr>
      <tr>
        <th>Generator</th>
        <td><xsl:value-of select="generator" /></td>
      </tr>
      <tr>
        <th>Size-Class</th>
        <td><xsl:value-of select="size" /></td>
      </tr>
      <tr>
        <th>Nodes</th>
        <td class="number-int"><xsl:value-of select="nodes" /></td>
      </tr>
      <tr>
        <th>Edges</th>
        <td class="number-int"><xsl:value-of select="edges" /></td>
      </tr>
    </table>
  </xsl:template>

  <xsl:template match="all-layouts">
    <h2>Layouts</h2>
    <form action="" method="GET">
      <fieldset>
        <legend>Select all layouts of this graph to be included in the comparison.</legend>
        <xsl:for-each select="layout">
          <xsl:sort select="@key" data-type="number" />
          <xsl:variable name="layout"><xsl:value-of select="." /></xsl:variable>
          <xsl:variable name="checkid">
            <xsl:value-of select="concat('check-', translate($layout, '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz'))" />
          </xsl:variable>
          <div>
            <xsl:attribute name="id">
              <xsl:value-of select="$checkid" />
            </xsl:attribute>
            <input type="checkbox" name="layouts">
              <xsl:choose>
                <xsl:when test="../../layouts/layout-data[@layout = $layout]">
                  <xsl:attribute name="value">
                    <xsl:value-of select="../../layouts/layout-data[@layout = $layout]/@id" />
                  </xsl:attribute>
                  <xsl:if test="boolean(number(../../layouts/layout-data[@layout = $layout]/@selected))">
                    <xsl:attribute name="checked" />
                  </xsl:if>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:attribute name="disabled" />
                  <xsl:attribute name="title">No property computed or layout does not exist.</xsl:attribute>
                </xsl:otherwise>
              </xsl:choose>
            </input>
            <label>
            <xsl:attribute name="for">
              <xsl:value-of select="$checkid" />
            </xsl:attribute>
            <xsl:value-of select="." />
            </label>
          </div>
        </xsl:for-each>
      </fieldset>
      <p>
        <input type="button" class="jsonly" disabled="disabled" value="All" onclick="checkAllCheckBoxes('layouts', true)" />
        <xsl:text> </xsl:text>
        <input type="button" class="jsonly" disabled="disabled" value="None" onclick="checkAllCheckBoxes('layouts', false)" />
        <xsl:text> </xsl:text>
        <input type="submit" value="Update" />
      </p>
    </form>
  </xsl:template>

  <xsl:template match="all-properties">
    <xsl:param name="graphid" />
    <xsl:param name="property" />
    <xsl:param name="vicinity" select="0" />
    <p class="nocss">All properties for this graph:</p>
    <ul class="top-navigation">
      <xsl:for-each select="property">
        <xsl:sort select="@key" data-type="number" />
        <xsl:variable name="prop-json">
          <xsl:value-of select="translate(text(), '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz')" />
        </xsl:variable>
        <xsl:variable name="url">
          <xsl:value-of select="concat('/properties/', $graphid, '/', $prop-json, '/')" />
        </xsl:variable>
        <li>
          <xsl:if test="text() = $property">
            <xsl:attribute name="class">current</xsl:attribute>
          </xsl:if>
          <a>
            <xsl:attribute name="href"><xsl:value-of select="$url" /></xsl:attribute>
            <xsl:value-of select="." />
            <xsl:if test="@localized &gt; 0">(<var>d</var>)</xsl:if>
          </a>
          <xsl:if test="@localized &gt; 0">
            <ul class="top-navigation-sub">
              <xsl:for-each select="../../all-vicinities/vicinity">
                <li>
                  <xsl:if test="text() = $vicinity">
                    <xsl:attribute name="class">current</xsl:attribute>
                  </xsl:if>
                  <a>
                    <xsl:attribute name="href"><xsl:value-of select="concat($url, text())" /></xsl:attribute>
                    <var>d</var> = <xsl:value-of select="." />
                  </a>
                </li>
              </xsl:for-each>
            </ul>
          </xsl:if>
        </li>
      </xsl:for-each>
    </ul>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-entropy-table-scaled-histogram">
    <xsl:param name="graphid" />
    <xsl:param name="prop-json" />
    <xsl:variable name="scott">
      <xsl:choose>
        <xsl:when test="count(/root/layouts/layout-data[@binning = 'SCOTT_NORMAL_REFERENCE']/entropy) &gt; 0">1</xsl:when>
        <xsl:when test="count(/root/layouts/layout-data/entropy) = 0">1</xsl:when>
        <xsl:otherwise>0</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <table class="pretty">
      <tr>
        <th data-sort-type="s">Layout-ID</th>
        <th data-sort-type="d">Layout</th>
        <xsl:if test="$scott &gt; 0"><th colspan="2">Scott's Normal Reference</th></xsl:if>
        <xsl:if test="/root/all-bincounts/bincount">
          <th>
            <xsl:attribute name="colspan"><xsl:value-of select="count(/root/all-bincounts/bincount)" /></xsl:attribute>
            <xsl:text>Entropy (by bin count)</xsl:text>
          </th>
        </xsl:if>
      </tr>
      <tr>
        <th>&#x2014;</th>
        <th>&#x2014;</th>
        <xsl:if test="$scott &gt; 0">
          <th data-sort-type="d">Bin Count</th>
          <th data-sort-type="f">Entropy</th>
        </xsl:if>
        <xsl:for-each select="/root/all-bincounts/bincount">
          <th data-sort-type="d"><xsl:value-of select="." /></th>
        </xsl:for-each>
      </tr>
      <xsl:for-each select="layouts/layout-data[boolean(number(@selected))]">
        <xsl:sort select="@id" />
        <xsl:variable name="layoutid">
          <xsl:value-of select="@id" />
        </xsl:variable>
        <xsl:variable name="layout">
            <xsl:value-of select="@layout" />
        </xsl:variable>
        <tr>
          <xsl:attribute name="id"><xsl:value-of select="@id" /></xsl:attribute>
          <td>
            <a>
              <xsl:attribute name="href">
                <xsl:value-of select="concat('/layouts/', @id, '/')" />
              </xsl:attribute>
              <xsl:value-of select="substring(@id, 1, 8)" />
            </a>
          </td>
          <td>
            <xsl:attribute name="data-sort-key">
              <xsl:value-of select="/root/all-layouts/layout[text() = $layout]/@key" />
            </xsl:attribute>
            <xsl:value-of select="$layout" />
          </td>
          <xsl:if test="$scott &gt; 0">
            <td class="number-int">
              <xsl:value-of select="entropy[@binning = 'SCOTT_NORMAL_REFERENCE']/@bincount" />
            </td>
            <td class="number-float">
              <xsl:value-of select="entropy[@binning = 'SCOTT_NORMAL_REFERENCE']" />
            </td>
          </xsl:if>
          <xsl:for-each select="/root/all-bincounts/bincount">
            <xsl:sort select="." data-type="number" />
            <xsl:variable name="bincount">
              <xsl:value-of select="." />
            </xsl:variable>
            <td class="number-float">
              <xsl:value-of select="/root/layouts/layout-data[@id = $layoutid]/entropy[@binning = 'FIXED_COUNT' and @bincount = $bincount]" />
            </td>
          </xsl:for-each>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-entropy-figure">
    <xsl:param name="graphid" />
    <xsl:param name="prop-json" />
    <xsl:param name="vicinity" select="$none" />
    <p>
      <img class="pretty">
        <xsl:attribute name="src">
          <xsl:variable name="ploturl">
            <xsl:choose>
              <xsl:when test="$vicinity != $none">
                <xsl:value-of select="concat('/property-plots/', $prop-json, '/', $vicinity, '/entropy/', $graphid, '.svg')" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="concat('/property-plots/', $prop-json, '/entropy/', $graphid, '.svg')" />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:variable name="plotquery">
            <xsl:apply-templates select="/root/layouts/layout-data[boolean(number(@selected))]" />
          </xsl:variable>
          <xsl:variable name="plotqueryall">
            <xsl:apply-templates select="/root/layout" />
          </xsl:variable>
          <xsl:choose>
            <xsl:when test="$plotquery = $plotqueryall">
              <xsl:value-of select="$ploturl" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat($ploturl, '?', $plotquery)" />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </img>
    </p>
  </xsl:template>

  <xsl:template name="make-plot-query-string" match="/root/layouts/layout-data">
    <xsl:value-of select="concat('layouts=', @id)" />
    <xsl:if test="not(position() = last())">
      <xsl:text>&#x3B;</xsl:text>
    </xsl:if>
  </xsl:template>

</xsl:transform>
