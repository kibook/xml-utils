#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/transform.h>
#include "normalize.h"

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
	trimmed = calloc(strlen(content) + 1, sizeof(char));
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

int main(int argc, char **argv)
{
	int i;
	bool normalize = false;

	xmlDocPtr doc;
	xmlXPathContextPtr ctx = NULL;

	doc = xmlReadFile("-", NULL, 0);
	ctx = xmlXPathNewContext(doc);

	while ((i = getopt(argc, argv, "n:N")) != -1)
		switch (i) {
			case 'n':
				registerNs(ctx, optarg);
				break;
			case 'N':
				normalize = true;
				break;
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
