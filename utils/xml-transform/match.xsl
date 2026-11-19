<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:param name="_path"/>
  <xsl:output omit-xml-declaration="yes"/>
  <xsl:template match="/">
    <xsl:if>
      <xsl:value-of select="$_path"/>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>
