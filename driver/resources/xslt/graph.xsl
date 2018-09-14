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
        <h1><xsl:value-of select="id" /></h1>
        <xsl:call-template name="make-summary" />
        <h2>Proper Layouts</h2>
        <p class="alert">TODO: This is not implemented yet.</p>
        <h2>Interpolated Layouts</h2>
        <p class="alert">TODO: This is not implemented yet.</p>
      </body>
    </html>
  </xsl:template>

  <!-- context: /root -->
  <xsl:template name="make-summary">
    <h2>Summary</h2>
    <table class="horizontal">
      <tr>
        <th>Graph-ID</th>
        <td>
          <a>
            <xsl:attribute name="href"><xsl:value-of select="concat('/graphs/', id, '/')" /></xsl:attribute>
            <xsl:value-of select="id" />
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

</xsl:transform>
