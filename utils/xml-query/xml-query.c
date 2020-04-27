#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xml-utils.h"

#define PROG_NAME "xml-query"
#define VERSION "0.1.1"

struct opts {
	xmlNodePtr queries;
	xmlNodePtr conditions;
	xmlNodePtr namespaces;
	char delim;
	bool modify;
	bool overwrite;
};

/* Print an XPath string result to console. */
static int print_string(const xmlChar *s)
{
	return printf("%s", (char *) s);
}

/* Print an XPath Boolean result to console. */
static int print_boolean(const int b)
{
	return printf("%s", b ? "true" : "false");
}

/* Print an XPath number result to console. */
static int print_number(const float f)
{
	return printf("%g", f);
}

/* Print an XPath nodeset result to console. */
static int print_nodeset(const xmlNodeSetPtr nodeset, struct opts *opts)
{
	int i;

	if (xmlXPathNodeSetIsEmpty(nodeset)) {
		return 0;
	}

	for (i = 0; i < nodeset->nodeNr; ++i) {
		xmlChar *s;

		s = xmlNodeGetContent(nodeset->nodeTab[i]);
		printf("%s", (char *) s);
		xmlFree(s);

		if (i < nodeset->nodeNr - 1) {
			putchar(opts->delim);
		}
	}

	return 0;
}

/* Modify a nodeset by setting the content of each node to a given value. */
static int modify_nodeset(xmlNodeSetPtr nodeset, const xmlChar *value, struct opts *opts)
{
	int i;

	if (xmlXPathNodeSetIsEmpty(nodeset)) {
		return 0;
	}

	for (i = 0; i < nodeset->nodeNr; ++i) {
		xmlNodeSetContent(nodeset->nodeTab[i], value);
	}

	return 0;
}

/* Print the results of queries for a single doc. */
static int query_doc(xmlXPathContextPtr ctx, const char *path, struct opts *opts)
{
	xmlNodePtr cur;

	for (cur = opts->queries->children; cur; cur = cur->next) {
		xmlChar *type;

		type = xmlGetProp(cur, BAD_CAST "type");

		if (xmlStrcmp(type, BAD_CAST "xpath") == 0) {
			xmlChar *xpath, *value;
			xmlXPathObjectPtr obj;

			xpath = xmlNodeGetContent(cur);
			value = xmlGetProp(cur, BAD_CAST "val");
			obj = xmlXPathEvalExpression(xpath, ctx);

			switch (obj->type) {
				case XPATH_NODESET:
					if (value) {
						modify_nodeset(obj->nodesetval, value, opts);
					} else {
						print_nodeset(obj->nodesetval, opts);
					}
					break;
				case XPATH_BOOLEAN:
					print_boolean(obj->boolval);
					break;
				case XPATH_NUMBER:
					print_number(obj->floatval);
					break;
				case XPATH_STRING:
					print_string(obj->stringval);
					break;
				default:
					break;
			}

			xmlXPathFreeObject(obj);
			xmlFree(xpath);
			xmlFree(value);
		} else if (xmlStrcmp(type, BAD_CAST "filename") == 0) {
			printf("%s", path);
		}

		xmlFree(type);

		if (cur->next) {
			putchar(opts->delim);
		}
	}

	if (!opts->modify) {
		putchar('\n');
	}

	return 0;
}

/* Check if a doc matches specified conditions. */
static bool check_conditions(const xmlXPathContextPtr ctx, const xmlNodePtr conditions)
{
	xmlNodePtr cur;
	bool res = true;

	for (cur = conditions->children; cur; cur = cur->next) {
		xmlChar *xpath;
		xmlXPathObjectPtr obj;

		xpath = xmlNodeGetContent(cur);
		obj = xmlXPathEvalExpression(xpath, ctx);

		switch (obj->type) {
			case XPATH_NODESET:
				res = !xmlXPathNodeSetIsEmpty(obj->nodesetval);
				break;
			case XPATH_BOOLEAN:
				res = obj->boolval;
				break;
			default:
				break;
		}

		xmlXPathFreeObject(obj);
		xmlFree(xpath);
	}

	return res;
}

/* Register a namespace prefix within an XPath context. */
static void register_ns(xmlXPathContextPtr ctx, char *optarg)
{
	char *prefix, *uri;

	prefix = strtok(optarg, "=");
	uri    = strtok(NULL, "");

	xmlXPathRegisterNs(ctx, BAD_CAST prefix, BAD_CAST uri);
}

/* Query a single XML file. */
static int query_file(const char *path, struct opts *opts)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlNodePtr ns;

	if (!(doc = read_xml_doc(path))) {
		return 1;
	}

	ctx = xmlXPathNewContext(doc);

	/* Register defined namespace prefixes. */
	for (ns = opts->namespaces->children; ns; ns = ns->next) {
		xmlChar *n;
		n = xmlNodeGetContent(ns);
		register_ns(ctx, (char *) n);
		xmlFree(n);
	}

	if (check_conditions(ctx, opts->conditions)) {
		query_doc(ctx, path, opts);
	}

	if (opts->modify) {
		if (opts->overwrite) {
			xmlSaveFile(path, doc);
		} else {
			xmlSaveFile("-", doc);
		}
	}

	xmlXPathFreeContext(ctx);
	xmlFreeDoc(doc);

	return 0;
}

/* Show usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-d <delim>] [-N <ns>=<URL>] [-s <value>] [-w <xpath>] [-x <xpath>] [-0fnth?] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -0, --null                  Output a null-delimited list of results.");
	puts("  -d, --delimiter <delim>     Output results with a custom delimiter.");
	puts("  -f, --overwrite             Overwrite input files when modifying.");
	puts("  -h, -?, --help              Show usage message.");
	puts("  -N, --namespace <ns>=<URL>  Register a namespace.");
	puts("  -n, --filename              Print the filename of the document.");
	puts("  -s, --set <value>           Set a matched node to the given value.");
	puts("  -w, --where <xpath>         Skip documents where <xpath> is false.");
	puts("  -x, --xpath <xpath>         Print the results of the given XPath.");
	puts("  --version                   Show version information.");
	puts("  <file>                      XML file(s) to query.");
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
	const char *sopts = "0d:fN:ntw:s:x:h?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"null"     , no_argument      , 0, '0'},
		{"delimiter", no_argument      , 0, 'd'},
		{"overwrite", no_argument      , 0, 'f'},
		{"namespace", required_argument, 0, 'N'},
		{"filename" , no_argument      , 0, 'n'},
		{"set"      , required_argument, 0, 's'},
		{"tab"      , no_argument      , 0, 't'},
		{"when"     , required_argument, 0, 'w'},
		{"xpath"    , required_argument, 0, 'x'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	struct opts opts;
	xmlNodePtr cur = NULL;

	opts.queries = xmlNewNode(NULL, BAD_CAST "queries");
	opts.conditions = xmlNewNode(NULL, BAD_CAST "conditions");
	opts.namespaces = xmlNewNode(NULL, BAD_CAST "namespaces");
	opts.delim = '\n';
	opts.modify = false;
	opts.overwrite = false;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					goto cleanup;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case '0':
				opts.delim = '\0';
				break;
			case 'd':
				opts.delim = optarg[0];
				break;
			case 'f':
				opts.overwrite = true;
				break;
			case 'N':
				xmlNewChild(opts.namespaces, NULL, BAD_CAST "ns", BAD_CAST optarg);
				break;
			case 'n':
				cur = xmlNewChild(opts.queries, NULL, BAD_CAST "query", NULL);
				xmlSetProp(cur, BAD_CAST "type", BAD_CAST "filename");
				break;
			case 's':
				if (cur) xmlSetProp(cur, BAD_CAST "val", BAD_CAST optarg);
				opts.modify = true;
				break;
			case 't':
				opts.delim = '\t';
				break;
			case 'w':
				cur = xmlNewChild(opts.conditions, NULL, BAD_CAST "condition", BAD_CAST optarg);
				break;
			case 'x':
				cur = xmlNewChild(opts.queries, NULL, BAD_CAST "query", BAD_CAST optarg);
				xmlSetProp(cur, BAD_CAST "type", BAD_CAST "xpath");
				break;
			case 'h':
			case '?':
				show_help();
				goto cleanup;
		}
	}

	if (opts.queries->children) {
		if (optind < argc) {
			for (i = optind; i < argc; ++i) {
				query_file(argv[i], &opts);
			}
		} else {
			query_file("-", &opts);
		}
	}

cleanup:
	xmlFreeNode(opts.queries);
	xmlFreeNode(opts.conditions);
	xmlFreeNode(opts.namespaces);
	xmlCleanupParser();

	return 0;
}
