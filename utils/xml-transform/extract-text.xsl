<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:exsl="http://exslt.org/common" version="1.0">
  <xsl:param name="separator" select="'&#10;'"/>
  <xsl:param name="terminator" select="'&#10;'"/>
  <xsl:output method="text"/>
  <xsl:template match="/">
    <xsl:variable name="node"/>
    <xsl:choose>
      <xsl:when test="count(exsl:node-set($node)) &gt; 1">
        <xsl:for-each select="exsl:node-set($node)">
          <xsl:value-of select="."/>
          <xsl:if test="position() != last()">
            <xsl:value-of select="$separator"/>
          </xsl:if>
        </xsl:for-each>
        <xsl:value-of select="$terminator"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$node"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
