#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include "xml-utils.h"

#define PROG_NAME "xml-format"
#define VERSION "2.3.1"

/* Formatter options */
#define FORMAT_OVERWRITE	0x01
#define FORMAT_REMWSONLY	0x02
#define FORMAT_OMIT_DECL	0x04
#define FORMAT_COMPACT		0x08

/* The blank text node children in an element are considered removable only if
 * ALL the text node children of that element are blank (no mixed content), or
 * there are no text node children at all.
 *
 * If there is only a single blank text node child, it is considered removable
 * only if FORMAT_REMWSONLY is set.
 */
static bool blanks_are_removable(xmlNodePtr node, int opts)
{
	xmlNodePtr cur;
	int i;

	if (xmlNodeGetSpacePreserve(node) == 1) {
		return false;
	}

	for (cur = node->children, i = 0; cur; cur = cur->next, ++i) {
		if (cur->type == XML_TEXT_NODE && !xmlIsBlankNode(cur)) {
			return false;
		}
	}

	return i > 1 || (node->children && node->children->type != XML_TEXT_NODE) || optset(opts, FORMAT_REMWSONLY);
}

/* Remove blank children. */
static void remove_blanks(xmlNodePtr node)
{
	xmlNodePtr cur;

	cur = node->children;

	while (cur) {
		xmlNodePtr next;
		next = cur->next;

		if (xmlIsBlankNode(cur)) {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
		}

		cur = next;
	}
}

/* Format XML nodes. */
static void format(xmlNodePtr node, int opts)
{
	xmlNodePtr cur;

	if (blanks_are_removable(node, opts)) {
		remove_blanks(node);

		for (cur = node->children; cur; cur = cur->next) {
			format(cur, opts);
		}
	}
}

/* Format an XML file. */
static void format_file(const char *path, const char *out, int opts)
{
	xmlDocPtr doc;
	xmlSaveCtxtPtr save;
	int saveopts = 0;

	if (!(doc = read_xml_doc(path))) {
		return;
	}

	format(xmlDocGetRootElement(doc), opts);

	if (!optset(opts, FORMAT_COMPACT)) {
		saveopts |= XML_SAVE_FORMAT;
	}
	if (optset(opts, FORMAT_OMIT_DECL)) {
		saveopts |= XML_SAVE_NO_DECL;
	}

	if (out) {
		save = xmlSaveToFilename(out, NULL, saveopts);
	} else if (optset(opts, FORMAT_OVERWRITE)) {
		save = xmlSaveToFilename(path, NULL, saveopts);
	} else {
		save = xmlSaveToFilename("-", NULL, saveopts);
	}

	xmlSaveDoc(save, doc);
	xmlSaveClose(save);

	xmlFreeDoc(doc);
}

/* Show usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-cfOwh?] [-i <str>] [-o <path>] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -c, --compact       Compact output.");
	puts("  -f, --overwrite     Overwrite input XML files.");
	puts("  -h, -?, --help      Show usage message.");
	puts("  -i, --indent <str>  Set the indentation string.");
	puts("  -O, --omit-decl     Omit XML declaration.");
	puts("  -o, --out <path>    Output to <path> instead of stdout.");
	puts("  -w, --empty         Treat elements containing only whitespace as empty.");
	puts(" --version            Show version information.");
	puts("  <file>              XML file(s) to format. Otherwise, read from stdin.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
static void show_version(void)
{
	printf("%s (xml-utils) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i;
	const char *sopts = "cfi:Oo:wh?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"compact"  , no_argument      , 0, 'c'},
		{"overwrite", no_argument      , 0, 'f'},
		{"indent"   , required_argument, 0, 'i'},
		{"omit-decl", no_argument      , 0, 'O'},
		{"out"      , required_argument, 0, 'o'},
		{"empty"    , no_argument      , 0, 'w'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	int opts = 0;
	char *indent = NULL;
	char *out = NULL;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'c':
				opts |= FORMAT_COMPACT;
				break;
			case 'f':
				opts |= FORMAT_OVERWRITE;
				break;
			case 'i':
				indent = strdup(optarg);
				break;
			case 'O':
				opts |= FORMAT_OMIT_DECL;
				break;
			case 'o':
				out = strdup(optarg);
				break;
			case 'w':
				opts |= FORMAT_REMWSONLY;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (indent) {
		xmlTreeIndentString = indent;
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			format_file(argv[i], out, opts);
		}
	} else {
		format_file("-", out, opts);
	}

	free(indent);
	free(out);

	xmlCleanupParser();

	return 0;
}
