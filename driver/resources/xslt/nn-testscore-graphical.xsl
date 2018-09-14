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
        <script type="text/javascript" src="/graphstudy.js" />
        <script type="text/javascript" src="/nn/testscore.js" />
      </head>
      <body onload="init(); NN.init();">
        <h1>Neural Network Test Score</h1>
        <p>View this information <a href="?graphics=false">tabulated</a>.</p>
        <p id="something-went-wrong" class="alert">
          Sorry, the test case you're looking for does not seem to exist or your browser might experience trouble
          displaying this page.  If you reckon that the latter might be the case, please make sure that you have CSS and
          JavaScript support enabled.
        </p>
        <form action="" method="GET" class="special-test-case-selection">
          <input type="hidden" name="graphical" value="true" />
          <p>Only show test cases with the following outcomes:</p>
          <table>
            <tr>
              <xsl:for-each select="/root/all-tests/test">
                <xsl:sort select="@key" data-type="number" />
                <xsl:choose>
                  <xsl:when test="text() = 'EXPECTED'" />
                  <xsl:otherwise><td><xsl:value-of select="." /></td></xsl:otherwise>
                </xsl:choose>
              </xsl:for-each>
            </tr>
            <tr>
              <xsl:for-each select="/root/all-tests/test">
                <xsl:sort select="@key" data-type="number" />
                <xsl:variable name="enum"><xsl:value-of select="." /></xsl:variable>
                <xsl:variable name="name">
                  <xsl:value-of select="translate(text(), '_ABCDEFGHIJKLMNOPQRSTUVWXYZ', '-abcdefghijklmnopqrstuvwxyz')" />
                </xsl:variable>
                <xsl:choose>
                  <xsl:when test="text() = 'EXPECTED'" />
                  <xsl:otherwise>
                    <td>
                      <select>
                        <xsl:attribute name="name">
                          <xsl:value-of select="concat('only-', $name)" />
                        </xsl:attribute>
                        <option value="">
                          <xsl:if test="not /root/special-selection/only[@test = $enum]">
                            <xsl:attribute name="selected">selected</xsl:attribute>
                          </xsl:if>
                        </option>
                        <option value="1">
                          <xsl:if test="/root/special-selection/only[@test = $enum and @when = '1']">
                            <xsl:attribute name="selected">selected</xsl:attribute>
                          </xsl:if>
                          <xsl:text>correct</xsl:text>
                        </option>
                        <option value="0">
                          <xsl:if test="/root/special-selection/only[@test = $enum and @when = '0']">
                            <xsl:attribute name="selected">selected</xsl:attribute>
                          </xsl:if>
                          <xsl:text>wrong</xsl:text>
                        </option>
                      </select>
                    </td>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:for-each>
              <td><input type="submit" value="Go" /></td>
            </tr>
          </table>
        </form>
        <xsl:apply-templates select="test-cases/test-case">
          <xsl:with-param name="total" select="count(test-cases/test-case)" />
        </xsl:apply-templates>
        <p id="scroll-for-more" style="display:none">Scroll down to show more test cases</p>
        <div id="vertical-fill" />
      </body>
    </html>
  </xsl:template>

  <xsl:template match="test-case">
    <xsl:param name="total" />
    <div style="display:none">
      <h2>
        <xsl:attribute name="id">
          <xsl:value-of select="concat('test-case-', @index)" />
        </xsl:attribute>
        <xsl:text>Test Case </xsl:text>
        <span class="number-int"><xsl:value-of select="@index + 1" /></span>
        <xsl:text> of </xsl:text>
        <span class="number-int"><xsl:value-of select="$total" /></span>
        <a class="anchor" title="link to this test case">
          <xsl:attribute name="href">
            <xsl:value-of select="concat('#test-case-', @index)" />
          </xsl:attribute>
          <xsl:text>&#x00A7;</xsl:text>
        </a>
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
      <div class="wannabe-layout-comparison">
        <xsl:attribute name="data-test-case">
          <xsl:value-of select="@index" />
        </xsl:attribute>
        <xsl:attribute name="data-info">
          <xsl:call-template name="test-case-to-json" />
        </xsl:attribute>
      </div>
    </div>
  </xsl:template>

  <xsl:template match="lhs|rhs">
    <a>
      <xsl:attribute name="href">
        <xsl:value-of select="concat('/layouts/', text(), '/')" />
      </xsl:attribute>
      <img>
        <xsl:attribute name="src">
          <xsl:value-of select="concat('/layouts/', text(), '/picture.png')" />
        </xsl:attribute>
      </img>
    </a>
  </xsl:template>

  <xsl:template name="test-case-to-json">
    <xsl:variable name="quot">&quot;</xsl:variable>
    <xsl:text>{ </xsl:text>
    <xsl:text>"lhs" : </xsl:text>
    <xsl:value-of select="concat($quot, lhs, $quot)" />
    <xsl:text>, </xsl:text>
    <xsl:text>"rhs" : </xsl:text>
    <xsl:value-of select="concat($quot, rhs, $quot)" />
    <xsl:text>, </xsl:text>
    <xsl:text>"lhsInformal" : </xsl:text>
    <xsl:value-of select="concat($quot, lhs/@informal, $quot)" />
    <xsl:text>, </xsl:text>
    <xsl:text>"rhsInformal" : </xsl:text>
    <xsl:value-of select="concat($quot, rhs/@informal, $quot)" />
    <xsl:text>, </xsl:text>
    <xsl:text>"values" : { </xsl:text>
    <xsl:value-of select="concat('&quot;EXPECTED&quot; : ', @expected)" />
    <xsl:for-each select="value">
      <xsl:text>, </xsl:text>
      <xsl:value-of select="concat('&quot;', @name, '&quot; : ', text())" />
    </xsl:for-each>
    <xsl:text> }, </xsl:text>
    <xsl:text>"status" : </xsl:text>
    <xsl:value-of select="concat($quot, value[@name = 'NN_FORWARD']/@status, $quot)" />
    <xsl:text> }</xsl:text>
  </xsl:template>

</xsl:transform>
