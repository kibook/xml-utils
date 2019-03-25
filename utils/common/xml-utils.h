#ifndef XML_UTILS_H
#define XML_UTILS_H

#include <libxml/parser.h>

/* Default global XML parser options. */
extern int DEFAULT_PARSE_OPTS;

/* Macros for standard libxml2 parser options. */
#define LIBXML2_PARSE_LONGOPT_DEFS \
	{"dtdload", no_argument, 0, 0},\
	{"net",     no_argument, 0, 0},\
	{"noent",   no_argument, 0, 0},
#define LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind) \
	else if (strcmp(lopts[loptind].name, "dtdload") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_DTDLOAD;\
	} else if (strcmp(lopts[loptind].name, "net") == 0) {\
		DEFAULT_PARSE_OPTS &= ~XML_PARSE_NONET;\
	} else if (strcmp(lopts[loptind].name, "noent") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_NOENT;\
	}
#define LIBXML2_PARSE_LONGOPT_HELP \
	puts("");\
	puts("XML parser options:");\
	puts("  --dtdload  Load external DTD.");\
	puts("  --net      Allow network access.");\
	puts("  --noent    Resolve entities.");

/* Common read/write macros that use the default parser options. */
#define read_xml_doc(path) xmlReadFile(path, NULL, DEFAULT_PARSE_OPTS)
#define read_xml_mem(buffer, size) xmlReadMemory(buffer, size, NULL, NULL, DEFAULT_PARSE_OPTS)

#endif
