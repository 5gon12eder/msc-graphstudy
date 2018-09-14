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
        <h1>Problems</h1>
        <xsl:apply-templates select="filename" />
        <xsl:for-each select="all-actions/action">
          <xsl:sort select="@key" data-type="number" />
          <xsl:variable name="action"><xsl:value-of select="." /></xsl:variable>
          <xsl:if test="/root/problems[@action = $action]">
            <xsl:variable name="problems-total"><xsl:value-of select="/root/problems[@action = $action]/@count" /></xsl:variable>
            <xsl:variable name="problems-shown"><xsl:value-of select="count(/root/problems[@action = $action]/problem)" /></xsl:variable>
            <h2 class="constant-actions"><xsl:value-of select="$action" /></h2>
            <xsl:choose>
              <xsl:when test="$problems-total &gt; 0">
                <p>
                  <xsl:text>There were </xsl:text>
                  <span class="number-int"><xsl:value-of select="$problems-total" /></span>
                  <xsl:text> problems with this action (showing a random selection of </xsl:text>
                  <span class="number-int"><xsl:value-of select="$problems-shown" /></span>
                  <xsl:text> = </xsl:text>
                  <span class="number-percent"><xsl:value-of select="$problems-shown div $problems-total" /></span>
                  <xsl:text>&#x00A0;% thereof).</xsl:text>
                </p>
                <xsl:apply-templates select="/root/problems[@action = $action]" />
              </xsl:when>
              <xsl:otherwise>
                <p>There were no problems with this action.</p>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:if>
        </xsl:for-each>
      </body>
    </html>
  </xsl:template>

  <!-- Generic template for any action (must be the first to be defined).  -->
  <xsl:template match="problems">
    <ul>
      <xsl:for-each select="problem">
        <li><xsl:value-of select="." /></li>
      </xsl:for-each>
    </ul>
  </xsl:template>

  <xsl:template match="problems[@action = 'LAYOUTS']">
    <table class="pretty">
      <tr>
        <th class="clicksort" data-sort-type="s">Graph-ID</th>
        <th class="clicksort" data-sort-type="d">Layout</th>
        <th class="clicksort" data-sort-type="s">Error</th>
      </tr>
      <xsl:for-each select="problem">
        <tr>
          <td>
            <a>
              <xsl:attribute name="href">
                <xsl:value-of select="concat('/graphs/', @graph-id, '/')" />
              </xsl:attribute>
              <xsl:value-of select="substring(@graph-id, 1, 8)" />
            </a>
          </td>
          <td>
            <xsl:variable name="layout"><xsl:value-of select="@layout" /></xsl:variable>
            <xsl:attribute name="data-sort-key">
              <xsl:value-of select="/root/all-layouts/layout[text() = $layout]/@key" />
            </xsl:attribute>
            <xsl:value-of select="@layout" />
          </td>
          <td style="text-align:left">
            <xsl:value-of select="." />
          </td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <xsl:template match="problems[@action = 'LAY_WORSE']">
    <table class="pretty">
      <tr>
        <th class="clicksort" data-sort-type="d">Parent</th>
        <th class="clicksort" data-sort-type="d">Method</th>
        <th class="clicksort" data-sort-type="s">Error</th>
      </tr>
      <xsl:for-each select="problem">
        <tr>
          <td>
            <a>
              <xsl:attribute name="href">
                <xsl:value-of select="concat('/layouts/', @parent, '/')" />
              </xsl:attribute>
              <xsl:value-of select="substring(@parent, 1, 8)" />
            </a>
          </td>
          <td>
            <xsl:variable name="method"><xsl:value-of select="@method" /></xsl:variable>
            <xsl:attribute name="data-sort-key">
              <xsl:value-of select="/root/all-lay-worse/lay-worse[text() = $method]/@key" />
            </xsl:attribute>
            <xsl:value-of select="$method" />
          </td>
          <td style="text-align:left">
            <xsl:value-of select="." />
          </td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <xsl:template match="problems[@action = 'LAY_INTER']">
    <table class="pretty">
      <tr>
        <th class="clicksort" data-sort-type="d">1st Parent</th>
        <th class="clicksort" data-sort-type="d">2nd Parent</th>
        <th class="clicksort" data-sort-type="d">Method</th>
        <th class="clicksort" data-sort-type="s">Error</th>
      </tr>
      <xsl:for-each select="problem">
        <tr>
          <td>
            <a>
              <xsl:attribute name="href">
                <xsl:value-of select="concat('/layouts/', @parent-1st, '/')" />
              </xsl:attribute>
              <xsl:value-of select="substring(@parent-1st, 1, 8)" />
            </a>
          </td>
          <td>
            <a>
              <xsl:attribute name="href">
                <xsl:value-of select="concat('/layouts/', @parent-2nd, '/')" />
              </xsl:attribute>
              <xsl:value-of select="substring(@parent-2nd, 1, 8)" />
            </a>
          </td>
          <td>
            <xsl:variable name="method"><xsl:value-of select="@method" /></xsl:variable>
            <xsl:attribute name="data-sort-key">
              <xsl:value-of select="/root/all-lay-inter/lay-inter[text() = $method]/@key" />
            </xsl:attribute>
            <xsl:value-of select="$method" />
          </td>
          <td style="text-align:left">
            <xsl:value-of select="." />
          </td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <xsl:template match="problems[@action = 'PROPERTIES']">
    <table class="pretty">
      <tr>
        <th class="clicksort" data-sort-type="s">Layout-ID</th>
        <th class="clicksort" data-sort-type="d">Property</th>
        <th class="clicksort" data-sort-type="s">Error</th>
      </tr>
      <xsl:for-each select="problem">
        <tr>
          <td>
            <a>
              <xsl:attribute name="href">
                <xsl:value-of select="concat('/layouts/', @layout-id, '/')" />
              </xsl:attribute>
              <xsl:value-of select="substring(@layout-id, 1, 8)" />
            </a>
          </td>
          <td>
            <xsl:variable name="property"><xsl:value-of select="@property" /></xsl:variable>
            <xsl:attribute name="data-sort-key">
              <xsl:value-of select="/root/all-properties/property[text() = $property]/@key" />
            </xsl:attribute>
            <xsl:value-of select="$property" />
          </td>
          <td style="text-align:left">
            <xsl:value-of select="." />
          </td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <xsl:template match="problems[@action = 'METRICS']">
    <table class="pretty">
      <tr>
        <th class="clicksort" data-sort-type="s">Layout-ID</th>
        <th class="clicksort" data-sort-type="d">Metric</th>
        <th class="clicksort" data-sort-type="s">Error</th>
      </tr>
      <xsl:for-each select="problem">
        <tr>
          <td>
            <a>
              <xsl:attribute name="href">
                <xsl:value-of select="concat('/layouts/', @layout-id, '/')" />
              </xsl:attribute>
              <xsl:value-of select="substring(@layout-id, 1, 8)" />
            </a>
          </td>
          <td>
            <xsl:variable name="metric"><xsl:value-of select="@metric" /></xsl:variable>
            <xsl:attribute name="data-sort-key">
              <xsl:value-of select="/root/all-metrics/metric[text() = $metric]/@key" />
            </xsl:attribute>
            <xsl:value-of select="$metric" />
          </td>
          <td style="text-align:left">
            <xsl:value-of select="." />
          </td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <xsl:template match="filename">
    <xsl:choose>
      <xsl:when test="@timestamp">
        <p>
          <xsl:text>This data was loaded from file </xsl:text>
          <xsl:value-of select="." />
          <xsl:text> and has timestamp </xsl:text>
          <xsl:value-of select="@timestamp" />
          <xsl:text>.</xsl:text>
        </p>
      </xsl:when>
      <xsl:otherwise>
        <p class="alert">
          <xsl:text>The file </xsl:text>
          <xsl:value-of select="." />
          <xsl:text> does not exist.</xsl:text>
        </p>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:transform>
