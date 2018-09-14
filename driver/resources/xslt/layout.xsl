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
    <xsl:variable name="layoutid">
      <xsl:value-of select="id" />
    </xsl:variable>
    <xsl:variable name="graphid">
      <xsl:value-of select="graph/@id" />
    </xsl:variable>
    <xsl:variable name="layout">
      <xsl:choose>
        <xsl:when test="layouts/layout[@id = $layoutid]/@layout">
          <xsl:value-of select="layouts/layout[@id = $layoutid]/@layout" />
        </xsl:when>
        <xsl:otherwise>UNKNOWN</xsl:otherwise>
      </xsl:choose>
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
        <xsl:call-template name="make-layouts">
          <xsl:with-param name="graphid" select="$graphid" />
          <xsl:with-param name="layoutid" select="$layoutid" />
        </xsl:call-template>
        <h1 class="constant-layouts"><xsl:value-of select="$layout" /></h1>
        <xsl:call-template name="make-summary">
          <xsl:with-param name="graphid" select="$graphid" />
          <xsl:with-param name="layoutid" select="$layoutid" />
        </xsl:call-template>
        <xsl:call-template name="make-picture">
          <xsl:with-param name="graphid" select="$graphid" />
          <xsl:with-param name="layoutid" select="$layoutid" />
        </xsl:call-template>
        <xsl:call-template name="make-worse" />
        <xsl:call-template name="make-princomp">
          <xsl:with-param name="graphid" select="$graphid" />
          <xsl:with-param name="layoutid" select="$layoutid" />
        </xsl:call-template>
        <xsl:call-template name="make-properties">
          <xsl:with-param name="graphid" select="$graphid" />
          <xsl:with-param name="layoutid" select="$layoutid" />
        </xsl:call-template>
      </body>
    </html>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-summary">
    <xsl:param name="graphid" />
    <xsl:param name="layoutid" />
    <h2>Summary</h2>
    <table class="horizontal">
      <tr>
        <th>Graph-ID</th>
        <td>
          <a>
            <xsl:attribute name="href"><xsl:value-of select="concat('/graphs/', $graphid, '/')" /></xsl:attribute>
            <xsl:value-of select="$graphid" />
          </a>
        </td>
      </tr>
      <tr>
        <th>Generator</th>
        <td><xsl:value-of select="graph/generator" /></td>
      </tr>
      <tr>
        <th>Size-Class</th>
        <td><xsl:value-of select="graph/size" /></td>
      </tr>
      <tr>
        <th>Nodes</th>
        <td class="number-int"><xsl:value-of select="graph/nodes" /></td>
      </tr>
      <tr>
        <th>Edges</th>
        <td class="number-int"><xsl:value-of select="graph/edges" /></td>
      </tr>
      <tr>
        <th>Layout-ID</th>
        <td>
          <a>
            <xsl:attribute name="href">
              <xsl:value-of select="concat('/layouts/', $layoutid, '/')" />
            </xsl:attribute>
            <xsl:value-of select="$layoutid" />
          </a>
        </td>
      </tr>
      <xsl:choose>
        <xsl:when test="layouts/layout[@id = $layoutid]/@layout">
          <tr>
            <th>Layout</th>
            <td><xsl:value-of select="layouts/layout[@id = $layoutid]/@layout" /></td>
          </tr>
        </xsl:when>
        <xsl:when test="interpolation">
          <tr>
            <th>Interpolation</th>
            <td><xsl:value-of select="interpolation/@method" /></td>
          </tr>
          <xsl:apply-templates select="interpolation/parent" />
          <tr>
            <th>Rate</th>
            <td><span class="number-percent"><xsl:value-of select="interpolation/@rate" /></span>&#x00A0;%</td>
          </tr>
        </xsl:when>
        <xsl:when test="worsening">
          <tr>
            <th>Worsening</th>
            <td><xsl:value-of select="worsening/@method" /></td>
          </tr>
          <xsl:apply-templates select="worsening/parent" />
          <tr>
            <th>Rate</th>
            <td><span class="number-percent"><xsl:value-of select="worsening/@rate" /></span>&#x00A0;%</td>
          </tr>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message>I'm confused by this kind of layout.</xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </table>
  </xsl:template>

  <xsl:template match="parent">
    <xsl:param name="title" select="'Parent-ID'" />
    <tr>
      <th><xsl:value-of select="$title" /></th>
      <td>
        <a>
          <xsl:attribute name="href">
            <xsl:value-of select="concat('/layouts/', text(), '/')" />
          </xsl:attribute>
          <xsl:value-of select="." />
        </a>
      </td>
    </tr>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-picture">
    <xsl:param name="graphid" />
    <xsl:param name="layoutid" />
    <h2>Picture</h2>
    <p>
      <a>
        <xsl:attribute name="href">
          <xsl:value-of select="concat('/layouts/', $layoutid, '/picture.svg')" />
        </xsl:attribute>
        <img class="pretty">
          <xsl:attribute name="src">
            <xsl:value-of select="concat('/layouts/', $layoutid, '/picture.png')" />
          </xsl:attribute>
        </img>
      </a>
    </p>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-worse">
    <h2>Worse Layouts</h2>
    <xsl:for-each select="all-lay-worse/lay-worse">
      <xsl:sort select="@key" data-type="number" />
      <xsl:variable name="worse"><xsl:value-of select="." /></xsl:variable>
      <xsl:variable name="count">
        <xsl:value-of select="count(/root/worse-layouts/layout[@method = $worse])" />
      </xsl:variable>
      <h3><xsl:value-of select="$worse" /></h3>
      <p>
        <xsl:choose>
          <xsl:when test="$count = 0">
            <xsl:text>No thusly worsened layouts exist.</xsl:text>
          </xsl:when>
          <xsl:when test="$count = 1">
            <span class="number-int"><xsl:value-of select="$count" /></span>
            <xsl:text> thusly worsened layout exist.</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <span class="number-int"><xsl:value-of select="$count" /></span>
            <xsl:text> thusly worsened layouts exist.</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </p>
      <xsl:if test="$count &gt; 0">
        <ul class="layout-glider">
          <xsl:for-each select="/root/worse-layouts/layout[@method = $worse]">
            <xsl:sort select="@rate" data-type="number" />
            <li>
              <xsl:attribute name="data-rate">
                <xsl:value-of select="@rate" />
              </xsl:attribute>
              <xsl:attribute name="data-preview">
                <xsl:value-of select="concat('/layouts/', @id, '/thumbnail.png')" />
              </xsl:attribute>
              <a>
                <xsl:attribute name="href">
                  <xsl:value-of select="concat('/layouts/', @id, '/')" />
                </xsl:attribute>
                <xsl:value-of select="@id" />
              </a>
            </li>
          </xsl:for-each>
        </ul>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-princomp">
    <xsl:param name="graphid" />
    <xsl:param name="layoutid" />
    <h2>Principial Axes</h2>
    <xsl:choose>
      <xsl:when test="princomp">
        <table class="horizontal">
          <tr>
            <th>Major</th>
            <td>(</td>
            <td class="number-float" style="text-align:right"><xsl:value-of select="princomp/major/value[1]" /></td>
            <td>,</td>
            <td class="number-float" style="text-align:right"><xsl:value-of select="princomp/major/value[2]" /></td>
            <td>)</td>
          </tr>
          <tr>
            <th>Minor</th>
            <td>(</td>
            <td class="number-float" style="text-align:right"><xsl:value-of select="princomp/minor/value[1]" /></td>
            <td>,</td>
            <td class="number-float" style="text-align:right"><xsl:value-of select="princomp/minor/value[2]" /></td>
            <td>)</td>
          </tr>
        </table>
        <p>
          <xsl:text>View a </xsl:text>
          <a>
            <xsl:attribute name="href">
              <xsl:value-of select="concat('/layouts/', $layoutid, '/princomp.svg')" />
            </xsl:attribute>
            <xsl:text>drawing</xsl:text>
          </a>
          <xsl:text> of the layout's principial axes.</xsl:text>
        </p>
      </xsl:when>
      <xsl:otherwise>
        <p class="alert">Sorry, no principial component analysis available.</p>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-properties">
    <xsl:param name="graphid" />
    <xsl:param name="layoutid" />
    <xsl:apply-templates select="properties/property">
      <xsl:with-param name="graphid" select="$graphid" />
      <xsl:with-param name="layoutid" select="$layoutid" />
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="property">
    <xsl:param name="graphid" />
    <xsl:param name="layoutid" />
    <xsl:param name="unit" select="'?'" />
    <xsl:param name="prop" select="@name" />
    <xsl:variable name="propJson">
      <xsl:value-of select="translate($prop, '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz')" />
    </xsl:variable>
    <h2>
      <xsl:attribute name="id"><xsl:value-of select="$propJson" /></xsl:attribute>
      <span class="constant-properties"><xsl:value-of select="$prop" /></span>
      <xsl:text> </xsl:text>
      <span class="informal-property-name">
        <xsl:choose>
          <xsl:when test="(@localized = 0) and count(property-data[@kernel = 'BOXED']) &gt; 0">
            <a>
              <xsl:attribute name="href">
                <xsl:value-of select="concat('/properties/', $graphid, '/', $propJson, '/')" />
              </xsl:attribute>
              <xsl:value-of select="$prop" />
            </a>
          </xsl:when>
          <xsl:otherwise><xsl:value-of select="$prop" /></xsl:otherwise>
        </xsl:choose>
      </span>
    </h2>
    <xsl:variable name="disc" select="boolean(property-data[@kernel = 'BOXED'])" />
    <xsl:variable name="cont" select="boolean(property-data[@kernel = 'GAUSSIAN'])" />
    <ul class="kernels">
      <xsl:if test="$disc">
        <li>
          <span class="kernel-symbol">&#x02A4;</span>
          <xsl:text> This property was analyzed via histograms.</xsl:text>
        </li>
      </xsl:if>
      <xsl:if test="$cont">
        <li>
          <span class="kernel-symbol">&#x02A9;</span>
          <xsl:text> This property was analyzed via sliding averages.</xsl:text>
        </li>
      </xsl:if>
    </ul>
    <xsl:for-each select="property-data">
      <xsl:sort select="@vicinity" data-type="number" />
      <xsl:variable name="vicinity" select="@vicinity" />
      <xsl:if test="../@localized &gt; 0">
        <h3>
          <xsl:attribute name="id"><xsl:value-of select="concat($propJson, '-', $vicinity)" /></xsl:attribute>
          Vicinity <var>d</var>&#x20;=&#x20;<span class="number-int"><xsl:value-of select="$vicinity" /></span>
          <xsl:text> </xsl:text>
          <span class="informal-property-name">
            <xsl:choose>
              <xsl:when test="@kernel = 'BOXED'">
                <a>
                  <xsl:attribute name="href">
                    <xsl:value-of select="concat('/properties/', $graphid, '/', $propJson, '/', $vicinity, '/')" />
                  </xsl:attribute>
                  <xsl:value-of select="concat(../@name, '(', $vicinity, ')')" />
                </a>
              </xsl:when>
              <xsl:otherwise><xsl:value-of select="concat(../@name, '(', $vicinity, ')')" /></xsl:otherwise>
            </xsl:choose>
          </span>
        </h3>
      </xsl:if>
      <xsl:apply-templates select=".">
        <xsl:with-param name="unit" select="$unit" />
      </xsl:apply-templates>
      <xsl:apply-templates select="histograms|sliding-averages">
        <xsl:with-param name="layoutid" select="$layoutid" />
        <xsl:with-param name="prop" select="$prop" />
        <xsl:with-param name="vicinity" select="$vicinity" />
        <xsl:with-param name="unit" select="$unit" />
      </xsl:apply-templates>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="property-data">
    <xsl:param name="unit" />
    <xsl:variable name="symbol">
      <xsl:choose>
        <xsl:when test="@kernel = 'BOXED'">&#x02A4;</xsl:when>
        <xsl:when test="@kernel = 'GAUSSIAN'">&#x02A9;</xsl:when>
        <xsl:otherwise>?</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <table class="horizontal">
      <tr>
        <th><xsl:value-of select="$symbol" /> Size</th>
        <td class="number-int" style="text-align:right"><xsl:value-of select="size" /></td>
        <td></td>
      </tr>
      <tr>
        <th><xsl:value-of select="$symbol" /> Minimum</th>
        <td class="number-float" style="text-align:right"><xsl:value-of select="minimum" /></td>
        <td><xsl:value-of select="$unit" /></td>
      </tr>
      <tr>
        <th><xsl:value-of select="$symbol" /> Maximum</th>
        <td class="number-float" style="text-align:right"><xsl:value-of select="maximum" /></td>
        <td><xsl:value-of select="$unit" /></td>
      </tr>
      <tr>
        <th><xsl:value-of select="$symbol" /> Average</th>
        <td class="number-float" style="text-align:right"><xsl:value-of select="mean" /></td>
        <td><xsl:value-of select="$unit" /></td>
      </tr>
      <tr>
        <th><xsl:value-of select="$symbol" /> RMS</th>
        <td class="number-float" style="text-align:right"><xsl:value-of select="rms" /></td>
        <td><xsl:value-of select="$unit" /></td>
      </tr>
      <xsl:if test="entropy-intercept">
        <tr>
          <th><xsl:value-of select="$symbol" /> Entropy Intercept</th>
          <td class="number-float" style="text-align:right"><xsl:value-of select="entropy-intercept" /></td>
          <td>bit</td>
        </tr>
      </xsl:if>
      <xsl:if test="entropy-slope">
        <tr>
          <th><xsl:value-of select="$symbol" /> Entropy Slope</th>
          <td class="number-float" style="text-align:right"><xsl:value-of select="entropy-slope" /></td>
          <td>bit / 1</td>
        </tr>
      </xsl:if>
    </table>
  </xsl:template>

  <xsl:template match="histograms">
    <xsl:param name="layoutid" />
    <xsl:param name="prop" />
    <xsl:param name="vicinity" select="$none" />
    <xsl:param name="unit" />
    <p>
      <xsl:text>There are </xsl:text>
      <span class="number-int"><xsl:value-of select="count(histogram)" /></span>
      <xsl:text> histograms for this layout and property.</xsl:text>
    </p>
    <xsl:call-template name="histogram-table">
      <xsl:with-param name="unit" select="$unit" />
    </xsl:call-template>
    <xsl:call-template name="histogram-figure">
      <xsl:with-param name="layoutid" select="$layoutid" />
      <xsl:with-param name="prop" select="$prop" />
      <xsl:with-param name="vicinity" select="$vicinity" />
    </xsl:call-template>
  </xsl:template>

  <!-- context: histograms -->
  <xsl:template name="histogram-table">
    <xsl:param name="unit" />
    <table class="pretty">
      <tr>
        <th data-sort-type="d">Bin Count</th>
        <th data-sort-type="f">Bin Width</th>
        <th data-sort-type="f">Entropy</th>
        <th data-sort-type="d">Binning</th>
      </tr>
      <tr>
        <th>1</th>
        <th><xsl:value-of select="$unit" /></th>
        <th>bit</th>
        <th>&#x2014;</th>
      </tr>
      <xsl:for-each select="histogram">
        <xsl:sort select="bincount" data-type="number" />
        <tr>
          <td class="number-int"><xsl:value-of select="bincount" /></td>
          <td class="number-float"><xsl:value-of select="binwidth" /></td>
          <td class="number-float"><xsl:value-of select="entropy" /></td>
          <td>
            <xsl:variable name="binning"><xsl:value-of select="@binning" /></xsl:variable>
            <xsl:attribute name="data-sort-key">
              <xsl:value-of select="/root/all-binnings[text() = $binning]/@key" />
            </xsl:attribute>
            <xsl:value-of select="$binning" />
          </td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <!-- context: histograms -->
  <xsl:template name="histogram-figure">
    <xsl:param name="layoutid" />
    <xsl:param name="prop" />
    <xsl:param name="vicinity" select="$none" />
    <xsl:variable name="propJson">
      <xsl:value-of select="translate($prop, '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz')" />
    </xsl:variable>
    <xsl:variable name="directory">
      <xsl:choose>
        <xsl:when test="$vicinity &gt; 0">
          <xsl:value-of select="concat('/property-plots/', $propJson, '/', $vicinity, '/boxed/')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('/property-plots/', $propJson, '/boxed/')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="ctrlid">
      <xsl:value-of select="concat($propJson, '-histo-control-anchor')" />
    </xsl:variable>
    <xsl:variable name="histoid">
      <xsl:value-of select="concat($propJson, '-histo')" />
    </xsl:variable>
    <xsl:variable name="defaultcount">
      <xsl:choose>
        <xsl:when test="histogram[binning = 'SCOTT_NORMAL_REFERENCE']/bincount">
          <xsl:value-of select="histogram[binning = 'SCOTT_NORMAL_REFERENCE']/bincount" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="histogram[1]/bincount" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="haveauto">
      <xsl:choose>
        <xsl:when test="histogram[binning = 'SCOTT_NORMAL_REFERENCE']/bincount">1</xsl:when>
        <xsl:otherwise>0</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <p>
      <xsl:attribute name="id"><xsl:value-of select="concat($propJson, '-histo-control-anchor')" /></xsl:attribute>
      <xsl:text>Number of histogram bins:</xsl:text>
      <xsl:text>&#x20;</xsl:text>
      <select>
        <xsl:attribute name="onchange">
          <xsl:value-of select="concat('updateHistogram(', $apos, $histoid, $apos, ', this.value)')" />
        </xsl:attribute>
        <xsl:if test="$haveauto &gt; 0">
          <option disabled="disabled" selected="selected">auto</option>
        </xsl:if>
        <xsl:for-each select="histogram">
          <xsl:sort select="bincount" data-type="number" />
          <xsl:if test="@binning = 'FIXED_COUNT'">
            <option>
              <xsl:attribute name="value">
                <xsl:value-of select="concat($directory, $layoutid, '.svg?bincount=', bincount)" />
              </xsl:attribute>
              <xsl:value-of select="bincount" />
            </option>
          </xsl:if>
        </xsl:for-each>
      </select>
      <a class="anchor"><xsl:attribute name="href">
        <xsl:value-of select="concat('#', $ctrlid)" /></xsl:attribute>
        <xsl:text>&#x00b6;</xsl:text>
      </a>
    </p>
    <p>
      <img class="pretty">
        <xsl:attribute name="id"><xsl:value-of select="$histoid" /></xsl:attribute>
        <xsl:attribute name="data-property-name"><xsl:value-of select="$prop" /></xsl:attribute>
        <xsl:attribute name="src">
          <xsl:value-of select="concat($directory, $layoutid, '.svg?bincount=', $defaultcount)" />
        </xsl:attribute>
      </img>
    </p>
  </xsl:template>

  <xsl:template match="sliding-averages">
    <xsl:param name="layoutid" />
    <xsl:param name="prop" />
    <xsl:param name="vicinity" select="$none" />
    <xsl:param name="unit" />
    <p>
      <xsl:text>There are </xsl:text>
      <span class="number-int"><xsl:value-of select="count(sliding-average)" /></span>
      <xsl:text> sliding averages for this layout and property.</xsl:text>
    </p>
    <xsl:call-template name="sliding-average-table">
      <xsl:with-param name="unit" select="$unit" />
    </xsl:call-template>
    <xsl:call-template name="sliding-average-figure">
      <xsl:with-param name="layoutid" select="$layoutid" />
      <xsl:with-param name="prop" select="$prop" />
      <xsl:with-param name="vicinity" select="$vicinity" />
    </xsl:call-template>
  </xsl:template>

  <!-- context: sliding-averages -->
  <xsl:template name="sliding-average-table">
    <xsl:param name="unit" />
    <table class="pretty">
      <tr>
        <th data-sort-type="f">Sigma</th>
        <th data-sort-type="d">Points</th>
        <th data-sort-type="f">Differential Entropy</th>
      </tr>
      <tr>
        <th><xsl:value-of select="$unit" /></th>
        <th>1</th>
        <th>not one bit</th>
      </tr>
      <xsl:for-each select="sliding-average">
        <xsl:sort select="sigma" data-type="number" />
        <tr>
          <td class="number-float"><xsl:value-of select="sigma" /></td>
          <td class="number-int"><xsl:value-of select="points" /></td>
          <td class="number-float"><xsl:value-of select="entropy" /></td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <!-- context: sliding-averages -->
  <xsl:template name="sliding-average-figure">
    <xsl:param name="layoutid" />
    <xsl:param name="prop" />
    <xsl:param name="vicinity" select="$none" />
    <xsl:variable name="propJson">
      <xsl:value-of select="translate($prop, '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz')" />
    </xsl:variable>
    <xsl:if test="count(sliding-average) != 1">
      <xsl:message>Expected exactly one 'sliding-average' tag.</xsl:message>
    </xsl:if>
    <p>
      <img class="pretty">
        <xsl:attribute name="data-property-name"><xsl:value-of select="$prop" /></xsl:attribute>
        <xsl:attribute name="src">
          <xsl:choose>
            <xsl:when test="$vicinity != $none">
              <xsl:value-of select="concat('/property-plots/', $propJson, '/', $vicinity, '/gaussian/', $layoutid, '.svg')" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat('/property-plots/', $propJson, '/gaussian/', $layoutid, '.svg')" />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </img>
    </p>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-layouts">
    <xsl:param name="graphid" />
    <xsl:param name="layoutid" />
    <p class="nocss">All layouts for this graph:</p>
    <ul class="top-navigation">
      <xsl:for-each select="all-layouts/layout">
        <xsl:sort select="@key" data-type="number" />
        <xsl:variable name="layout">
          <xsl:value-of select="." />
        </xsl:variable>
        <li>
          <xsl:if test="/root/layouts/layout[@layout = $layout]/@id = $layoutid">
            <xsl:attribute name="class">current</xsl:attribute>
          </xsl:if>
          <xsl:choose>
            <xsl:when test="/root/layouts/layout[@layout = $layout]">
              <a>
                <xsl:attribute name="href">
                  <xsl:value-of select="concat('/layouts/', /root/layouts/layout[@layout = $layout]/@id)" />
                </xsl:attribute>
                <xsl:attribute name="title">
                  <xsl:value-of select="/root/layouts/layout[@layout = $layout]/@id" />
                </xsl:attribute>
                <xsl:value-of select="." />
              </a>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="." />
            </xsl:otherwise>
          </xsl:choose>
        </li>
      </xsl:for-each>
    </ul>
  </xsl:template>

</xsl:transform>
