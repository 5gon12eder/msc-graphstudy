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
        <h1>About this HTTP Server</h1>
        <table class="horizontal" style="white-space:nowrap">
          <xsl:for-each select="info">
            <tr>
              <th><xsl:value-of select="@name" /></th>
              <td><xsl:value-of select="." /></td>
            </tr>
          </xsl:for-each>
        </table>
        <xsl:apply-templates select="environment" />
      </body>
    </html>
  </xsl:template>

  <xsl:template match="environment">
    <h2>Environment</h2>
    <table class="horizontal" style="white-space:nowrap">
      <xsl:for-each select="variable">
        <xsl:sort select="@name" />
        <tr>
          <th><xsl:value-of select="@name" /></th>
          <td><xsl:value-of select="." /></td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

</xsl:transform>
