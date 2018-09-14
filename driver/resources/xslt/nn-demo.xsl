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
        <h1>Neural Network Demo</h1>
        <p id="compare-random">Compare <var>N</var> random pairs of layouts:</p>
        <form action="#compare-random" method="GET">
          <p>
            <input type="number" name="random" required="required" placeholder="number" min="0"
                   title="Enter the number of pairs to choose randomly" style="width:5em">
              <xsl:if test="results/result[@random = 1]">
                <xsl:attribute name="value">
                  <xsl:value-of select="count(results/result[@random = 1])" />
                </xsl:attribute>
              </xsl:if>
            </input>
            <xsl:text> </xsl:text>
            <input type="submit" value="Compare" />
          </p>
        </form>
        <p id="compare-specific">Compare two specific layouts (IDs may be abbreviated to unambiguous prefixes of four or more digits):</p>
        <form action="#compare-specific" metho="GET">
          <xsl:call-template name="make-id-input">
            <xsl:with-param name="index" select="1" />
          </xsl:call-template>
          <xsl:text> </xsl:text>
          <xsl:call-template name="make-id-input">
            <xsl:with-param name="index" select="2" />
          </xsl:call-template>
          <xsl:text> </xsl:text>
          <input type="submit" value="Compare" />
        </form>
        <xsl:apply-templates select="timings" />
        <xsl:apply-templates select="results" />
      </body>
    </html>
  </xsl:template>

  <xsl:template name="make-id-input">
    <xsl:param name="index" />
    <input type="text" inputmode="verbatim" required="required" spellcheck="false" size="32" minlength="4" maxlength="32" pattern="[A-Fa-f0-9]+">
      <xsl:choose>
        <xsl:when test="$index = 1">
          <xsl:attribute name="name">lhs</xsl:attribute>
          <xsl:attribute name="placeholder">1st layout ID</xsl:attribute>
          <xsl:attribute name="title">Enter the ID (hex string) of the first layout to compare</xsl:attribute>
          <xsl:if test="/root/results/result[@random = 0]/lhs">
            <xsl:attribute name="value"><xsl:value-of select="/root/results/result[@random = 0]/lhs" /></xsl:attribute>
          </xsl:if>
        </xsl:when>
        <xsl:when test="$index = 2">
          <xsl:attribute name="name">rhs</xsl:attribute>
          <xsl:attribute name="placeholder">2nd layout ID</xsl:attribute>
          <xsl:attribute name="title">Enter the ID (hex string) of the second layout to compare</xsl:attribute>
          <xsl:if test="/root/results/result[@random = 0]/rhs">
            <xsl:attribute name="value"><xsl:value-of select="/root/results/result[@random = 0]/rhs" /></xsl:attribute>
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">Confused by layout ID index other than 1 or 2</xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </input>
  </xsl:template>

  <xsl:template match="timings">
    <h2>Performance</h2>
    <table class="horizontal">
      <tr>
        <th>Loading Model</th>
        <td class="number-duration"><xsl:value-of select="load" /></td>
        <!-- <td class="number-percent"><xsl:value-of select="number(load) div number(total)" /></td> -->
      </tr>
      <tr>
        <th>Evaluating Model</th>
        <td class="number-duration"><xsl:value-of select="eval" /></td>
        <!-- <td class="number-percent"><xsl:value-of select="number(eval) div number(total)" /></td> -->
      </tr>
      <tr>
        <th>Total Time</th>
        <td class="number-duration"><xsl:value-of select="total" /></td>
        <!-- <td class="number-percent"><xsl:value-of select="number(total) div number(total)" /></td> -->
      </tr>
    </table>
  </xsl:template>

  <xsl:template match="results">
    <xsl:for-each select="result">
      <h2>
        <xsl:text>Result of Comparing Layouts </xsl:text>
        <xsl:value-of select="substring(lhs, 1, 8)" />
        <xsl:text> and </xsl:text>
        <xsl:value-of select="substring(rhs, 1, 8)" />
        <xsl:text> (</xsl:text>
        <var>p</var>
        <xsl:text>&#x00A0;=&#x00A0;</xsl:text>
        <span class="number-100"><xsl:value-of select="value[@name = 'NN_FORWARD']" /></span>
        <xsl:text>)</xsl:text>
      </h2>
      <p>
        <xsl:text>Compare the </xsl:text>
        <a>
          <xsl:attribute name="href">
            <xsl:value-of select="concat('/nn/features/?lhs=', lhs, ';rhs=', rhs)" />
          </xsl:attribute>
          <xsl:text>feature vectors</xsl:text>
        </a>
        <xsl:text> of the two layouts.</xsl:text>
      </p>
      <div class="layout-comparison">
        <div class="layout-pair">
          <div class="pretty layout lhs"><xsl:apply-templates select="lhs" /></div>
          <div class="pretty layout rhs"><xsl:apply-templates select="rhs" /></div>
        </div>
        <div class="ruler" style="display:none">
          <xsl:for-each select="value">
            <xsl:attribute name="{concat('data-value-', @name)}">
              <xsl:value-of select="." />
            </xsl:attribute>
          </xsl:for-each>
          <xsl:text>This should show a fancy ruler if your Browser supports JavaScript and CSS.</xsl:text>
        </div>
      </div>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="lhs|rhs">
    <a>
      <xsl:attribute name="href">
        <xsl:value-of select="concat('/layouts/', text(), '/')" />
      </xsl:attribute>
      <xsl:attribute name="title">
        <xsl:value-of select="@informal" />
      </xsl:attribute>
      <img>
        <xsl:attribute name="src">
          <xsl:value-of select="concat('/layouts/', text(), '/picture.png')" />
        </xsl:attribute>
      </img>
    </a>
  </xsl:template>

</xsl:transform>
