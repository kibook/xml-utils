#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xml-utils.h"

#define PROG_NAME "xml-trim"
#define VERSION "3.5.1"

#define is_space(c) isspace((unsigned char) c)

/* Remove whitespace on left end of string. */
static char *strltrm(char *dst, const char *src)
{
	int start;
	for (start = 0; is_space(src[start]); ++start);
	sprintf(dst, "%s", src + start);
	return dst;
}

/* Remove whitespace on right end of string. */
static char *strrtrm(char *dst, const char *src)
{
	int len, end;
	len = strlen(src);
	for (end = len - 1; is_space(src[end]); --end);
	sprintf(dst, "%.*s", end + 1, src);
	return dst;
}

/* Normalize space by replacing all sequences of whitespace characters with a
 * single space.
 */
static char *strnorm(char *dst, const char *src)
{
	int i, j;
	j = 0;
	for (i = 0; src[i]; ++i) {
		if (is_space(src[i])) {
			dst[j] = ' ';
			while (is_space(src[i + 1])) {
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
static void register_ns(xmlXPathContextPtr ctx, char *optarg)
{
	char *prefix, *uri;

	prefix = strtok(optarg, "=");
	uri    = strtok(NULL, "");

	xmlXPathRegisterNs(ctx, BAD_CAST prefix, BAD_CAST uri);
}

/* Trim space in a text node. */
static void trim(xmlNodePtr node, char *(*f)(char *, const char *)) {
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
static void trim_nodes(xmlNodeSetPtr nodes, bool normalize)
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

static void trim_nodes_in_file(const char *path, xmlNodePtr ns, xmlNodePtr elems, bool normalize, bool overwrite)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlNodePtr cur;

	doc = read_xml_doc(path);
	ctx = xmlXPathNewContext(doc);

	for (cur = ns->children; cur; cur = cur->next) {
		xmlChar *n;
		n = xmlNodeGetContent(cur);
		register_ns(ctx, (char *) n);
		xmlFree(n);
	}

	for (cur = elems->children; cur; cur = cur->next) {
		xmlChar *xpath;
		xmlXPathObjectPtr obj;
		xmlChar *e;

		e = xmlNodeGetContent(cur);

		/* If the element specifier contains a /, treat it like a
		 * literal XPath expression.
		 *
		 * Otherwise, match all elements with the same name at any
		 * position.
		 */
		if (xmlStrchr(e, '/')) {
			xpath = xmlStrdup(e);
		} else {
			xpath = xmlStrdup(BAD_CAST "//");
			xpath = xmlStrcat(xpath, e);
		}

		xmlFree(e);

		obj = xmlXPathEvalExpression(xpath, ctx);

		xmlFree(xpath);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			trim_nodes(obj->nodesetval, normalize);
		}

		xmlXPathFreeObject(obj);
	}

	xmlXPathFreeContext(ctx);

	if (overwrite) {
		save_xml_doc(doc, path);
	} else {
		save_xml_doc(doc, "-");
	}

	xmlFreeDoc(doc);
}

/* Show usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-e <elem> ...] [-N <ns=URL> ...] [-fnh?] [<src>...]");
	puts("");
	puts("Options:");
	puts("  -e, --element <elem>      Element to trim space on.");
	puts("  -f, --overwrite           Overwrite input files.");
	puts("  -h, -?, --help            Show usage message.");
	puts("  -N, --namespace <ns=URL>  Register a namespace.");
	puts("  -n, --normalize           Normalize space as well as trim.");
	puts("  --version                 Show version information.");
	puts("  <src>                     XML file to trim.");
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
	xmlNodePtr ns, elems;
	bool normalize = false;
	bool overwrite = false;

	const char *sopts = "e:fN:nh?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"element"  , required_argument, 0, 'e'},
		{"overwrite", no_argument      , 0, 'f'},
		{"namespace", required_argument, 0, 'N'},
		{"normalize", no_argument      , 0, 'n'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	ns = xmlNewNode(NULL, BAD_CAST "namespaces");
	elems = xmlNewNode(NULL, BAD_CAST "elems");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1)
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg);
				break;
			case 'e':
				xmlNewChild(elems, NULL, BAD_CAST "elem", BAD_CAST optarg);
				break;
			case 'f':
				overwrite = true;
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

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			trim_nodes_in_file(argv[i], ns, elems, normalize, overwrite);
		}
	} else {
		trim_nodes_in_file("-", ns, elems, normalize, overwrite);
	}

	xmlFreeNode(ns);
	xmlFreeNode(elems);

	xmlCleanupParser();

	return 0;
}
