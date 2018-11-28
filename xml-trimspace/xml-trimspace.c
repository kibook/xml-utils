#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define PROG_NAME "xml-trimspace"
#define VERSION "2.1.2"

/* Remove whitespace on left end of string. */
char *strltrm(char *dst, const char *src)
{
	int start;
	for (start = 0; isspace(src[start]); ++start);
	sprintf(dst, "%s", src + start);
	return dst;
}

/* Remove whitespace on right end of string. */
char *strrtrm(char *dst, const char *src)
{
	int len, end;
	len = strlen(src);
	for (end = len - 1; isspace(src[end]); --end);
	sprintf(dst, "%.*s", end + 1, src);
	return dst;
}

/* Normalize space by replacing all sequences of whitespace characters with a
 * single space.
 */
char *strnorm(char *dst, const char *src)
{
	int i, j;
	j = 0;
	for (i = 0; src[i]; ++i) {
		if (isspace(src[i])) {
			dst[j] = ' ';
			while (isspace(src[i + 1])) {
				++i;
			}
		} else {
			dst[j] = src[i];
		}
		++j;
	}
	return dst;
}

/* Register an XML namespace with the XPath context. */
void register_ns(xmlXPathContextPtr ctx, char *optarg)
{
	char *prefix, *uri;

	prefix = strtok(optarg, "=");
	uri    = strtok(NULL, "");

	xmlXPathRegisterNs(ctx, BAD_CAST prefix, BAD_CAST uri);
}

/* Trim space in a text node. */
void trim(xmlNodePtr node, char *(*f)(char *, const char *)) {
	char *content, *trimmed;

	content = (char *) xmlNodeGetContent(node);

	trimmed = calloc(strlen(content) + 1, 1);
	f(trimmed, content);
	xmlFree(content);

	content = strdup(trimmed);
	xmlFree(trimmed);

	xmlNodeSetContent(node, BAD_CAST content);
	xmlFree(content);
}

/* Trim all text nodes in a given set of elements. */
void trim_nodes(xmlNodeSetPtr nodes, bool normalize)
{
	int i;

	for (i = 0; i < nodes->nodeNr; ++i) {
		xmlNodePtr first, last;

		/* If node has no children, no trimming is necessary. */
		if (!nodes->nodeTab[i]->children) {
			continue;
		}

		if ((first = nodes->nodeTab[i]->children)->type == XML_TEXT_NODE) {
			trim(first, strltrm);
		}

		if ((last = xmlGetLastChild(nodes->nodeTab[i]))->type == XML_TEXT_NODE) {
			trim(last, strrtrm);
		}

		if (normalize) {
			for (first = nodes->nodeTab[i]->children; first; first = first->next) {
				if (first->type == XML_TEXT_NODE) {
					trim(first, strnorm);
				}
			}
		}
	}
}

/* Show usage message. */
void show_help(void)
{
	puts("Usage: " PROG_NAME " [-N <ns=URL>] [-nh?] <elem>... < <src> > <dst>");
	puts("");
	puts("Options:");
	puts("  -h -?        Show usage message.");
	puts("  -N <ns=URL>  Register a namespace.");
	puts("  -n           Normalize space as well as trim.");
	puts("  --version    Show version information.");
	puts("  <elem>...    Elements to trim space on.");
	puts("  <src>        Source XML file.");
	puts("  <dst>        The output file.");
}

/* Show version information. */
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

	const char *sopts = "N:nh?";
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
			case 'N':
				xmlNewChild(ns, NULL, BAD_CAST "ns", BAD_CAST optarg);
				break;
			case 'n':
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
		register_ns(ctx, (char *) n);
		xmlFree(n);
	}
	
	for (i = optind; i < argc; ++i) {
		xmlChar *xpath;
		xmlXPathObjectPtr obj;

		/* If the element specifier contains a /, treat it like a
		 * literal XPath expression.
		 *
		 * Otherwise, match all elements with the same name at any
		 * position.
		 */
		if (strchr(argv[i], '/')) {
			xpath = xmlStrdup(BAD_CAST argv[i]);
		} else {
			xpath = xmlStrdup(BAD_CAST "//");
			xpath = xmlStrcat(xpath, BAD_CAST argv[i]);
		}

		obj = xmlXPathEvalExpression(xpath, ctx);

		xmlFree(xpath);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			trim_nodes(obj->nodesetval, normalize);
		}
		
		xmlXPathFreeObject(obj);
	}

	xmlXPathFreeContext(ctx);

	xmlSaveFile("-", doc);

	xmlFreeDoc(doc);

	xmlFreeNode(ns);

	xmlCleanupParser();

	return 0;
}
