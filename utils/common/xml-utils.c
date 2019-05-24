#include "xml-utils.h"

/* Default global XML parsing options.
 *
 * XML_PARSE_NONET
 *   Disable network access as a safety precaution.
 */
int DEFAULT_PARSE_OPTS = XML_PARSE_NONET;

/* Determine if an option is set. */
bool optset(int opts, int opt)
{
	return ((opts & opt) == opt);
}

/* Read an XML document from a file. */
xmlDocPtr read_xml_doc(const char *path)
{
	xmlDocPtr doc;

	doc = xmlReadFile(path, NULL, DEFAULT_PARSE_OPTS);

	if (optset(DEFAULT_PARSE_OPTS, XML_PARSE_XINCLUDE)) {
		xmlXIncludeProcessFlags(doc, DEFAULT_PARSE_OPTS);
	}

	return doc;
}

/* Read an XML document from memory. */
xmlDocPtr read_xml_mem(const char *buffer, int size)
{
	xmlDocPtr doc;

	doc = xmlReadMemory(buffer, size, NULL, NULL, DEFAULT_PARSE_OPTS);

	if (optset(DEFAULT_PARSE_OPTS, XML_PARSE_XINCLUDE)) {
		xmlXIncludeProcessFlags(doc, DEFAULT_PARSE_OPTS);
	}

	return doc;
}
