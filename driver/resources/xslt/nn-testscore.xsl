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
    <xsl:variable name="total">
      <xsl:value-of select="count(test-cases/test-case)" />
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
        <h1>Neural Network Test Score</h1>
        <h2>Confusion</h2>
        <xsl:call-template name="confusion-matrix" />
        <h2>Competing Metrics</h2>
        <xsl:call-template name="metrics" />
        <h2>Detailed Results</h2>
        <p>View this information <a href="?graphical=true">graphically</a>.</p>
        <table class="pretty">
          <tr>
            <th data-sort-type="s">ID</th>
            <th data-sort-type="d">Layout</th>
            <th data-sort-type="s">ID</th>
            <th data-sort-type="d">Layout</th>
            <th data-sort-type="f">Expected</th>
            <th data-sort-type="f">Actual</th>
            <th data-sort-type="f">Error</th>
            <th data-sort-type="d">Status</th>
          </tr>
          <xsl:for-each select="test-cases/test-case/value[@name = 'NN_FORWARD']">
            <xsl:variable name="index"><xsl:value-of select="../@index" /></xsl:variable>
            <xsl:variable name="status"><xsl:value-of select="@status" /></xsl:variable>
            <tr>
              <xsl:apply-templates select="../lhs" />
              <xsl:apply-templates select="../rhs" />
              <td class="number-100"><xsl:value-of select="../@expected" /></td>
              <td class="number-100"><xsl:value-of select="." /></td>
              <td class="number-100"><xsl:value-of select="@error" /></td>
              <td>
                <xsl:attribute name="data-sort-key">
                  <xsl:value-of select="/root/all-status/status[text() = $status]/@key" />
                </xsl:attribute>
                <a>
                  <xsl:attribute name="href">
                    <xsl:value-of select="concat('?graphical=true#test-case-', position())" />
                  </xsl:attribute>
                  <xsl:value-of select="$status" />
                </a>
              </td>
            </tr>
          </xsl:for-each>
        </table>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="lhs|rhs">
    <xsl:variable name="layout"><xsl:value-of select="@layout" /></xsl:variable>
    <td>
      <a>
        <xsl:attribute name="href">
          <xsl:value-of select="concat('/layouts/', text(), '/')" />
        </xsl:attribute>
        <xsl:value-of select="substring(text(), 1, 8)" />
      </a>
    </td>
    <td><xsl:value-of select="@informal" /></td>
  </xsl:template>

  <xsl:template name="confusion-matrix">
    <xsl:variable name="total">
      <xsl:value-of select="count(/root/test-cases/test-case)" />
    </xsl:variable>
    <xsl:variable name="TN">
      <xsl:value-of select="count(/root/test-cases/test-case/value[@name = 'NN_FORWARD' and @status = 'TRUE_NEGATIVE'])" />
    </xsl:variable>
    <xsl:variable name="TP">
      <xsl:value-of select="count(/root/test-cases/test-case/value[@name = 'NN_FORWARD' and @status = 'TRUE_POSITIVE'])" />
    </xsl:variable>
    <xsl:variable name="FN">
      <xsl:value-of select="count(/root/test-cases/test-case/value[@name = 'NN_FORWARD' and @status = 'FALSE_NEGATIVE'])" />
    </xsl:variable>
    <xsl:variable name="FP">
      <xsl:value-of select="count(/root/test-cases/test-case/value[@name = 'NN_FORWARD' and @status = 'FALSE_POSITIVE'])" />
    </xsl:variable>
    <h3>Absolute</h3>
    <table class="pretty">
      <tr>
        <th></th>
        <th>Condition-Negative</th>
        <th>Condition-Positive</th>
        <th>&#x03A3;</th>
      </tr>
      <tr>
        <th style="text-align:left">Prediction-Negative</th>
        <td class="number-int"><xsl:value-of select="$TN" /></td>
        <td class="number-int"><xsl:value-of select="$FN" /></td>
        <td class="number-int"><xsl:value-of select="$TN + $FN" /></td>
      </tr>
      <tr>
        <th style="text-align:left">Prediction-Positive</th>
        <td class="number-int"><xsl:value-of select="$FP" /></td>
        <td class="number-int"><xsl:value-of select="$TP" /></td>
        <td class="number-int"><xsl:value-of select="$FP + $TP" /></td>
      </tr>
      <tr>
        <th style="text-align:left">&#x03A3;</th>
        <td class="number-int"><xsl:value-of select="$TN + $FP" /></td>
        <td class="number-int"><xsl:value-of select="$FN + $TP" /></td>
        <td class="number-int"><xsl:value-of select="$TN + $FN + $FP + $TP" /></td>
      </tr>
    </table>
    <h3>Relative</h3>
    <table class="pretty">
      <tr>
        <th></th>
        <th>Condition-Negative</th>
        <th>Condition-Positive</th>
        <th>&#x03A3;</th>
      </tr>
      <tr>
        <th style="text-align:left">Prediction-Negative</th>
        <td class="number-percent"><xsl:value-of select="$TN div $total" /></td>
        <td class="number-percent"><xsl:value-of select="$FN div $total" /></td>
        <td class="number-percent"><xsl:value-of select="($TN + $FN) div $total" /></td>
      </tr>
      <tr>
        <th style="text-align:left">Prediction-Positive</th>
        <td class="number-percent"><xsl:value-of select="$FP div $total" /></td>
        <td class="number-percent"><xsl:value-of select="$TP div $total" /></td>
        <td class="number-percent"><xsl:value-of select="($FP + $TP) div $total" /></td>
      </tr>
      <tr>
        <th style="text-align:left">&#x03A3;</th>
        <td class="number-percent"><xsl:value-of select="($TN + $FP) div $total" /></td>
        <td class="number-percent"><xsl:value-of select="($FN + $TP) div $total" /></td>
        <td class="number-percent"><xsl:value-of select="($TN + $FN + $FP + $TP) div $total" /></td>
      </tr>
    </table>
  </xsl:template>

  <xsl:template name="metrics">
    <table class="pretty">
      <tr>
        <th data-sort-type="d">Metric</th>
        <th data-sort-type="d">Count</th>
        <th data-sort-type="d">True Negatives</th>
        <th data-sort-type="d">True Positives</th>
        <th data-sort-type="d">False Negatives</th>
        <th data-sort-type="d">False Positives</th>
        <th data-sort-type="f">Success Rate</th>
        <th data-sort-type="f">Failure Rate</th>
      </tr>
      <xsl:for-each select="/root/all-tests/test">
        <xsl:sort select="@key" data-type="number" />
        <xsl:variable name="test"><xsl:value-of select="." /></xsl:variable>
        <xsl:variable name="total">
          <xsl:value-of select="count(/root/test-cases/test-case/value[@name = $test])" />
        </xsl:variable>
        <xsl:if test="$total &gt; 0">
          <xsl:variable name="TN">
            <xsl:value-of select="count(/root/test-cases/test-case/value[@name = $test and @status = 'TRUE_NEGATIVE'])" />
          </xsl:variable>
          <xsl:variable name="TP">
            <xsl:value-of select="count(/root/test-cases/test-case/value[@name = $test and @status = 'TRUE_POSITIVE'])" />
          </xsl:variable>
          <xsl:variable name="FN">
            <xsl:value-of select="count(/root/test-cases/test-case/value[@name = $test and @status = 'FALSE_NEGATIVE'])" />
          </xsl:variable>
          <xsl:variable name="FP">
            <xsl:value-of select="count(/root/test-cases/test-case/value[@name = $test and @status = 'FALSE_POSITIVE'])" />
          </xsl:variable>
          <tr>
            <td style="text-align:left">
              <xsl:attribute name="data-sort-key"><xsl:value-of select="@key" /></xsl:attribute>
              <xsl:value-of select="." />
            </td>
            <td class="number-int"><xsl:value-of select="$total" /></td>
            <td class="number-int"><xsl:value-of select="$TN" /></td>
            <td class="number-int"><xsl:value-of select="$TP" /></td>
            <td class="number-int"><xsl:value-of select="$FN" /></td>
            <td class="number-int"><xsl:value-of select="$FP" /></td>
            <td class="number-percent"><xsl:value-of select="($TP + $TN) div $total" /></td>
            <td class="number-percent"><xsl:value-of select="($FP + $FN) div $total" /></td>
          </tr>
        </xsl:if>
      </xsl:for-each>
    </table>
  </xsl:template>

</xsl:transform>
