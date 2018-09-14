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
        <h1>
          <xsl:choose>
            <xsl:when test="query-results">
              <xsl:text>Results of Your SQL Query</xsl:text>
            </xsl:when>
            <xsl:otherwise>
          <xsl:text>Query the SQL Database</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </h1>
        <p>
          <xsl:text>Have a look at the </xsl:text>
          <a href="/schemata.sql" title="schemata.sql">schema definitions</a>
          <xsl:text> and the </xsl:text>
          <a href="/views.sql" title="views.sql">view definitions</a>
          <xsl:text> for detailed information about the available tables / views and the data therein.</xsl:text>
        </p>
        <xsl:apply-templates select="all-tables" />
        <xsl:apply-templates select="query-results/table" />
      </body>
    </html>
  </xsl:template>

  <xsl:template match="all-tables">
    <p>There are <span class="number-int"><xsl:value-of select="count(table)" /></span> tables in the database.</p>
    <form action="" method="GET">
      <p>
        <select name="table">
          <option value="*" selected="selected">&#x2014; all tables &#x2014;</option>
          <option value="">&#x2014; no tables &#x2014;</option>
          <xsl:for-each select="table">
            <xsl:sort select="." />
            <option>
              <xsl:attribute name="value"><xsl:value-of select="." /></xsl:attribute>
              <xsl:value-of select="." />
            </option>
          </xsl:for-each>
          <xsl:for-each select="view">
            <xsl:sort select="." />
            <option>
              <xsl:attribute name="value"><xsl:value-of select="." /></xsl:attribute>
              <xsl:value-of select="concat(text(), ' (view)')" />
            </option>
          </xsl:for-each>
        </select>
        <xsl:text> </xsl:text>
        <input type="submit" value="Select" />
      </p>
      <p>
        <input id="only-count" type="checkbox" name="count" />
        <label for="only-count">Only fetch the number of rows per table</label>
      </p>
    </form>
  </xsl:template>

  <xsl:template match="table">
    <h2><xsl:value-of select="@name" /></h2>
    <xsl:choose>
      <xsl:when test="columns">
        <p>
          <xsl:text>The table contains </xsl:text>
          <a>
            <xsl:attribute name="href">
              <xsl:value-of select="concat('?table=', @name)" />
            </xsl:attribute>
            <span class="number-int"><xsl:value-of select="@rows" /></span>
            <xsl:text> rows</xsl:text>
          </a>
          <xsl:text> &#x2013; </xsl:text>
          <span class="number-int"><xsl:value-of select="count(row)" /></span>
          <xsl:text> of which (</xsl:text>
          <span class="number-percent"><xsl:value-of select="count(row) div @rows" /></span>
          <xsl:text> %) are shown.</xsl:text>
        </p>
      </xsl:when>
      <xsl:otherwise>
        <p>
          <xsl:text>The table contains </xsl:text>
          <a>
            <xsl:attribute name="href">
              <xsl:value-of select="concat('?table=', @name)" />
            </xsl:attribute>
            <span class="number-int"><xsl:value-of select="@rows" /></span>
            <xsl:text> rows</xsl:text>
          </a>
          <xsl:text>.</xsl:text>
        </p>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="count(row)">
      <table class="pretty sql">
        <tr>
          <xsl:for-each select="columns/column">
            <th>
              <xsl:if test="@type">
                <xsl:attribute name="class">clicksort</xsl:attribute>
                <xsl:attribute name="data-sort-type"><xsl:value-of select="@type" /></xsl:attribute>
              </xsl:if>
              <xsl:value-of select="." />
            </th>
          </xsl:for-each>
        </tr>
        <xsl:for-each select="row">
          <tr>
            <xsl:for-each select="value">
              <td>
                <xsl:variable name="idx"><xsl:value-of select="position()" /></xsl:variable>
                <xsl:variable name="typ">
                  <xsl:value-of select="../../columns/column[position() = $idx]/@type" />
                </xsl:variable>
                <xsl:if test="$typ = 's'">
                  <xsl:attribute name="style">text-align:left</xsl:attribute>
                </xsl:if>
                <xsl:choose>
                  <xsl:when test="@key">
                    <xsl:attribute name="data-sort-key"><xsl:value-of select="@key" /></xsl:attribute>
                  </xsl:when>
                  <xsl:when test="$typ = 'd'">
                    <xsl:attribute name="class">number-int</xsl:attribute>
                  </xsl:when>
                  <xsl:when test="$typ = 'f'">
                    <xsl:attribute name="class">number-float</xsl:attribute>
                  </xsl:when>
                </xsl:choose>
                <xsl:value-of select="." />
              </td>
            </xsl:for-each>
          </tr>
        </xsl:for-each>
      </table>
    </xsl:if>
  </xsl:template>

</xsl:transform>
