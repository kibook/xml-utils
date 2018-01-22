#include <stdio.h>
#include <unistd.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

void showHelp(void)
{
	puts("Usage: xml-merge <dst> <src>");
	puts("");
	puts("Options:");
	puts("  dst      XML file to <src> in to");
	puts("  src      XML file to merge in to <dst>");
	puts("  -h -?    Show help/usage message");
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

	while ((c = getopt(argc, argv, "h?")) != -1) {
		switch (c) {
			case 'h':
			case '?':
				showHelp();
				exit(0);
		}
	}

	if (optind < argc) fname1 = argv[optind];
	if (optind + 1 < argc) fname2 = argv[optind + 1];

	doc1 = xmlReadFile(fname1, NULL, 0);
	doc2 = xmlReadFile(fname2, NULL, 0);

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

	xmlSaveFile("-", doc1);

	xmlFreeDoc(doc1);
	xmlFreeDoc(doc2);

	return 0;
}
