#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/transform.h>
#include "normalize.h"

#define PROG_NAME "xml-trimspace"
#define VERSION "1.0.0"

char *strltrm(char *dst, const char *src)
{
	int start;
	for (start = 0; isspace(src[start]); ++start);
	sprintf(dst, "%s", src + start);
	return dst;
}

char *strrtrm(char *dst, const char *src)
{
	int len, end;
	len = strlen(src);
	for (end = len - 1; isspace(src[end]); --end);
	sprintf(dst, "%.*s", end + 1, src);
	return dst;
}

void registerNs(xmlXPathContextPtr ctx, char *optarg)
{
	char *prefix, *uri;

	prefix = strtok(optarg, "=");
	uri    = strtok(NULL, "");

	xmlXPathRegisterNs(ctx, BAD_CAST prefix, BAD_CAST uri);
}

void trim(xmlNodePtr node, char *(*f)(char *, const char *)) {
	char *content, *trimmed;

	content = (char *) xmlNodeGetContent(node);
	trimmed = calloc(strlen(content) + 1, 1);
	f(trimmed, content);
	xmlFree(content);
	content = (char *) xmlEncodeEntitiesReentrant(node->doc, BAD_CAST trimmed);
	content = strdup(trimmed);
	xmlFree(trimmed);
	xmlNodeSetContent(node, BAD_CAST content);
	xmlFree(content);
}

void trimNodes(xmlNodeSetPtr nodes)
{
	int i;

	for (i = 0; i < nodes->nodeNr; ++i) {
		xmlNodePtr first, last;

		if ((first = nodes->nodeTab[i]->children)->type == XML_TEXT_NODE) {
			trim(first, strltrm);
		}

		if ((last = xmlGetLastChild(nodes->nodeTab[i]))->type == XML_TEXT_NODE) {
			trim(last, strrtrm);
		}
	}
}

xmlDocPtr normalizeElem(xmlDocPtr doc, const char *name)
{
	xmlDocPtr styledoc;
	xsltStylesheetPtr style;
	const char *params[3];
	char *namestr;
	xmlDocPtr res;

	styledoc = xmlReadMemory((const char *) normalize_xsl, normalize_xsl_len, NULL, NULL, 0);

	style = xsltParseStylesheetDoc(styledoc);

	namestr = malloc(strlen(name) + 3);
	sprintf(namestr, "\"%s\"", name);

	params[0] = "name";
	params[1] = namestr;
	params[2] = NULL;

	res = xsltApplyStylesheet(style, doc, params);

	free(namestr);

	xmlFreeDoc(doc);

	xsltFreeStylesheet(style);

	return res;
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-Nh?] [-n <ns=URL>] < <src> > <dst>");
	puts("");
	puts("Options:");
	puts("  -h -?        Show usage message.");
	puts("  -N           Normalize space as well as trim.");
	puts("  -n <ns=URL>  Register a namespace.");
	puts("  <elem>...    Elements to trim space on.");
	puts("  <src>        Source XML file.");
	puts("  <dst>        The output file.");
}

void show_version(void)
{
	printf("%s (xml-utils) %s\n", PROG_NAME, VERSION);
}

int main(int argc, char **argv)
{
	int i;
	bool normalize = false;

	xmlDocPtr doc;
	xmlXPathContextPtr ctx = NULL;

	xmlNodePtr ns, cur;

	const char *sopts = "n:Nh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	ns = xmlNewNode(NULL, BAD_CAST "namespaces");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1)
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'n':
				xmlNewChild(ns, NULL, BAD_CAST "ns", BAD_CAST optarg);
				break;
			case 'N':
				normalize = true;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}

	doc = xmlReadFile("-", NULL, 0);
	ctx = xmlXPathNewContext(doc);

	for (cur = ns->children; cur; cur = cur->next) {
		xmlChar *n;
		n = xmlNodeGetContent(cur);
		registerNs(ctx, (char *) n);
		xmlFree(n);
	}
	
	for (i = optind; i < argc; ++i) {
		char xpath[256];
		xmlXPathObjectPtr obj;

		snprintf(xpath, 256, "//%s", argv[i]);

		obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			trimNodes(obj->nodesetval);
		}
		
		xmlXPathFreeObject(obj);

		if (normalize) {
			doc = normalizeElem(doc, argv[i]);
		}
	}

	xmlXPathFreeContext(ctx);

	xmlSaveFile("-", doc);

	xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;
}
