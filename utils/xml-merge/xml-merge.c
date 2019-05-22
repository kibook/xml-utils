#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "xml-utils.h"

#define PROG_NAME "xml-merge"
#define VERSION "1.3.0"

void showHelp(void)
{
	puts("Usage: " PROG_NAME "[-fh?] <dst> <src>");
	puts("");
	puts("Options:");
	puts("  -f, --overwrite  Overwrite <dst> file.");
	puts("  -h, -?, --help   Show help/usage message");
	puts("  --version        Show version information");
	puts("  dst              XML file to <src> in to");
	puts("  src              XML file to merge in to <dst>");
	LIBXML2_PARSE_LONGOPT_HELP
}

void show_version(void)
{
	printf("%s (xml-utils) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

xmlNodePtr firstXPathNode(char *xpath, xmlDocPtr doc)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
		node = NULL;
	else
		node = obj->nodesetval->nodeTab[0];
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}

int main(int argc, char **argv)
{
	const char *fname1 = "-";
	const char *fname2 = "-";

	char xpath[256];

	xmlNodePtr target1;
	xmlNodePtr target2;

	xmlDocPtr doc1;
	xmlDocPtr doc2;

	int c;

	const char *sopts = "fh?";
	struct option lopts[] = {
		{"version"  , no_argument, 0, 0},
		{"help"     , no_argument, 0, 'h'},
		{"overwrite", no_argument, 0, 'f'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	bool overwrite = false;

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'f':
				overwrite = true;
				break;
			case 'h':
			case '?':
				showHelp();
				return 0;
		}
	}

	if (optind < argc) fname1 = argv[optind];
	if (optind + 1 < argc) fname2 = argv[optind + 1];

	doc1 = read_xml_doc(fname1);
	doc2 = read_xml_doc(fname2);

	target2 = xmlDocGetRootElement(doc2);

	sprintf(xpath, "//%s", (char *) target2->name);

	target1 = firstXPathNode(xpath, doc1);

	if (target1) {
		xmlAddNextSibling(target1, xmlCopyNode(target2, 1));
		xmlUnlinkNode(target1);
		xmlFreeNode(target1);
	} else {
		fprintf(stderr, "xml-merge: ERROR: No element %s to merge on.\n", target2->name);
	}

	if (overwrite) {
		xmlSaveFile(fname1, doc1);
	} else {
		xmlSaveFile("-", doc1);
	}

	xmlFreeDoc(doc1);
	xmlFreeDoc(doc2);

	xmlCleanupParser();

	return 0;
}
