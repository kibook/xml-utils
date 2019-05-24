#ifndef XML_UTILS_H
#define XML_UTILS_H

#include <stdbool.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

/* Default global XML parser options. */
extern int DEFAULT_PARSE_OPTS;

/* Macros for standard libxml2 parser options. */
#define LIBXML2_PARSE_LONGOPT_DEFS \
	{"dtdload",  no_argument, 0, 0},\
	{"net",      no_argument, 0, 0},\
	{"noent",    no_argument, 0, 0},\
	{"xinclude", no_argument, 0, 0},
#define LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind) \
	else if (strcmp(lopts[loptind].name, "dtdload") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_DTDLOAD;\
	} else if (strcmp(lopts[loptind].name, "net") == 0) {\
		DEFAULT_PARSE_OPTS &= ~XML_PARSE_NONET;\
	} else if (strcmp(lopts[loptind].name, "noent") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_NOENT;\
	} else if (strcmp(lopts[loptind].name, "xinclude") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_XINCLUDE | XML_PARSE_NOBASEFIX | XML_PARSE_NOXINCNODE;\
	}
#define LIBXML2_PARSE_LONGOPT_HELP \
	puts("");\
	puts("XML parser options:");\
	puts("  --dtdload   Load external DTD.");\
	puts("  --net       Allow network access.");\
	puts("  --noent     Resolve entities.");\
	puts("  --xinclude  Do XInclude processing.");

/* Return whether a bitset contains an option. */
bool optset(int opts, int opt);

/* Read an XML document from a file. */
xmlDocPtr read_xml_doc(const char *path);

/* Read an XML document from memory. */
xmlDocPtr read_xml_mem(const char *buffer, int size);

#endif
