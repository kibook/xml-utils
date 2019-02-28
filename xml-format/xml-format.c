#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <libxml/tree.h>

#define PROG_NAME "xml-format"
#define VERSION "1.1.0"

/* Returns true if: 
 * - The node contains a mix of text and non-text children
 * - ALL the text nodes are empty text nodes
 */
bool blanks_are_removable(xmlNodePtr node)
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

	return i > 1;
}

/* Remove blank children. */
void remove_blanks(xmlNodePtr node)
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
void format(xmlNodePtr node)
{
	xmlNodePtr cur;

	if (blanks_are_removable(node)) {
		remove_blanks(node);
	}

	for (cur = node->children; cur; cur = cur->next) {
		format(cur);
	}
}

/* Format an XML file. */
void format_file(const char *path, const char *out, bool overwrite)
{
	xmlDocPtr doc;

	if (!(doc = xmlReadFile(path, NULL, 0))) {
		return;
	}

	format(xmlDocGetRootElement(doc));

	if (out) {
		xmlSaveFormatFile(out, doc, 1);
	} else if (overwrite) {
		xmlSaveFormatFile(path, doc, 1);
	} else {
		xmlSaveFormatFile("-", doc, 1);
	}

	xmlFreeDoc(doc);
}

/* Show usage message. */
void show_help(void)
{
	puts("Usage: " PROG_NAME " [-fh?] [-i <str>] [-o <path>] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -f         Overwrite input XML files.");
	puts("  -h -?      Show usage message.");
	puts("  -i <str>   Set the indentation string.");
	puts("  -o <path>  Output to <path> instead of stdout.");
	puts(" --version   Show version information.");
	puts("  <file>     XML file(s) to format. Otherwise, read from stdin.");
}

/* Show version information. */
void show_version(void)
{
	printf("%s (xml-utils) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i;
	const char *sopts = "fi:o:h?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	bool overwrite = false;
	char *indent = NULL;
	char *out = NULL;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'f':
				overwrite = true;
				break;
			case 'i':
				indent = strdup(optarg);
				break;
			case 'o':
				out = strdup(optarg);
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
			format_file(argv[i], out, overwrite);
		}
	} else {
		format_file("-", out, false);
	}

	free(indent);
	free(out);

	return 0;
}
