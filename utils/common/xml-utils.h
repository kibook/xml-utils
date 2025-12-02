#ifndef XML_UTILS_H
#define XML_UTILS_H

#include <stdbool.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxml/HTMLparser.h>

/* Default global XML parser options. */
extern int DEFAULT_PARSE_OPTS;

/* Whether to parse input files as HTML. */
extern bool PARSE_AS_HTML;

/* Macros for standard libxml2 parser options. */
#define LIBXML2_PARSE_LONGOPT_DEFS \
	{"dtdload",  no_argument, 0, 0},\
	{"html",     no_argument, 0, 0},\
	{"huge",     no_argument, 0, 0},\
	{"net",      no_argument, 0, 0},\
	{"noent",    no_argument, 0, 0},\
	{"xinclude", no_argument, 0, 0},\
	{"xml-catalog", required_argument, 0, 0},
#define LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg) \
	else if (strcmp(lopts[loptind].name, "dtdload") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_DTDLOAD;\
	} else if (strcmp(lopts[loptind].name, "html") == 0) {\
		PARSE_AS_HTML = true;\
	} else if (strcmp(lopts[loptind].name, "huge") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_HUGE;\
	} else if (strcmp(lopts[loptind].name, "net") == 0) {\
		DEFAULT_PARSE_OPTS &= ~XML_PARSE_NONET;\
	} else if (strcmp(lopts[loptind].name, "noent") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_NOENT;\
	} else if (strcmp(lopts[loptind].name, "xinclude") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_XINCLUDE | XML_PARSE_NOBASEFIX | XML_PARSE_NOXINCNODE;\
	} else if (strcmp(lopts[loptind].name, "xml-catalog") == 0) {\
		xmlLoadCatalog(optarg);\
	}
#define LIBXML2_PARSE_LONGOPT_HELP \
	puts("");\
	puts("XML parser options:");\
	puts("  --dtdload             Load external DTD.");\
	puts("  --html                Parse input files as HTML.");\
	puts("  --huge                Remove any internal arbitrary parser limits.");\
	puts("  --net                 Allow network access.");\
	puts("  --noent               Resolve entities.");\
	puts("  --xinclude            Do XInclude processing.");\
	puts("  --xml-catalog <file>  Use an XML catalog.");

/* libxml < 2.15.0 included a built-in HTTP client that would be used to
 * load entities, which needed to be disabled when not using the --net option.
 * This has been removed in 2.15.0, so disabling the entity loader is no longer
 * necessary.
 */
#if LIBXML_VERSION < 21500
#define LIBXML2_PARSE_INIT \
	if (optset(DEFAULT_PARSE_OPTS, XML_PARSE_NONET)) {\
		xmlSetExternalEntityLoader(xmlNoNetExternalEntityLoader);\
	}
#else
#define LIBXML2_PARSE_INIT
#endif

/* Return whether a bitset contains an option. */
bool optset(int opts, int opt);

/* Read an XML document from a file. */
xmlDocPtr read_xml_doc(const char *path, const bool html);

/* Read an XML document from memory. */
xmlDocPtr read_xml_mem(const char *buffer, int size);

/* Save an XML document to a file. */
int save_xml_doc(xmlDocPtr doc, const char *path);

#endif
