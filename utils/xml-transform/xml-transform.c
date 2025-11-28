#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xmlsave.h>
#include <libxml/xpathInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#include "xml-utils.h"
#include "identity.h"
#include "null-input.h"
#include "extract.h"
#include "extract-text.h"

#define PROG_NAME "xml-transform"
#define VERSION "1.8.0"

#define INF_PREFIX PROG_NAME ": INFO: "
#define ERR_PREFIX PROG_NAME ": ERROR: "

#define I_TRANSFORM INF_PREFIX "Transforming %s...\n"

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s: %s\n"
#define E_FILE_NO_WRITE ERR_PREFIX "Could not open file for writing: %s: %s\n"
#define E_BAD_STRING_PARAM ERR_PREFIX "Cannot add string param %s because it contains both quote and double-quote characters.\n"

#define EXIT_OS_ERROR 1

static enum verbosity { QUIET, NORMAL, VERBOSE } verbosity = NORMAL;
static bool preserve_dtd = false;
static bool use_xml_stylesheets = false;
static bool null_input = false;
static xmlNodePtr global_params;

/* Add identity template to stylesheet. */
static void add_identity(xmlDocPtr style)
{
	xmlDocPtr identity;
	xmlNodePtr stylesheet, first, template;

	identity = read_xml_mem((const char *) identity_xsl, identity_xsl_len);
	template = xmlFirstElementChild(xmlDocGetRootElement(identity));

	stylesheet = xmlDocGetRootElement(style);

	first = xmlFirstElementChild(stylesheet);

	if (first) {
		xmlAddPrevSibling(first, xmlCopyNode(template, 1));
	} else {
		xmlAddChild(stylesheet, xmlCopyNode(template, 1));
	}

	xmlFreeDoc(identity);
}

/* Apply stylesheets to a doc, preserving the original DTD. */
static xmlDocPtr transform_doc_preserve_dtd(xmlDocPtr doc, xmlNodePtr stylesheets)
{
	xmlDocPtr src;
	xmlNodePtr cur, new;

	src = xmlCopyDoc(doc, 1);

	for (cur = stylesheets->children; cur; cur = cur->next) {
		xmlDocPtr res;
		xsltStylesheetPtr style;
		const char **params;

		/* Select cached stylesheet/params. */
		style = (xsltStylesheetPtr) cur->doc;
		params = (const char **) cur->children;

		res = xsltApplyStylesheet(style, doc, params);

		xmlFreeDoc(doc);

		doc = res;
	}

	/* If the result has a root element, copy it in place of the root
	 * element of the original document to preserve the original DTD. */
	if ((new = xmlDocGetRootElement(doc))) {
		xmlNodePtr old;
		old = xmlDocSetRootElement(src, xmlCopyNode(new, 1));
		xmlFreeNode(old);
	/* Otherwise, copy the whole doc to keep non-XML results. */
	} else {
		xmlFreeDoc(src);
		src = xmlCopyDoc(doc, 1);
	}

	xmlFreeDoc(doc);

	return src;
}

/* Apply stylesheets to a doc. */
static xmlDocPtr transform_doc(xmlDocPtr doc, xmlNodePtr stylesheets)
{
	xmlNodePtr cur;

	for (cur = stylesheets->children; cur; cur = cur->next) {
		xmlDocPtr res;
		xsltStylesheetPtr style;
		const char **params;

		/* Select cached stylesheet/params. */
		style = (xsltStylesheetPtr) cur->doc;
		params = (const char **) cur->children;

		res = xsltApplyStylesheet(style, doc, params);

		xmlFreeDoc(doc);

		doc = res;
	}

	return doc;
}

/* Save a document using the output settings of the specified stylesheet. */
static void save_doc(xmlDocPtr doc, const char *path, xsltStylesheetPtr style)
{
	if (xmlStrcmp(style->method, BAD_CAST "text") == 0) {
		FILE *f;

		if (strcmp(path, "-") == 0) {
			f = stdout;
		} else {
			f = fopen(path, "w");
		}

		if (!f) {
			fprintf(stderr, E_FILE_NO_WRITE, path, strerror(errno));
			exit(EXIT_OS_ERROR);
		}

		if (doc && doc->children && doc->children->content) {
			fprintf(f, "%s", (char *) doc->children->content);
		}

		if (f != stdout) {
			fclose(f);
		}
	} else {
		xmlSaveCtxtPtr save;
		int saveopts = 0;

		if (xmlStrcmp(style->method, BAD_CAST "html") == 0) {
			saveopts |= XML_SAVE_AS_HTML;
		}
		if (style->omitXmlDeclaration == 1) {
			saveopts |= XML_SAVE_NO_DECL;
		}
		if (style->indent == 1) {
			saveopts |= XML_SAVE_FORMAT;
		}

		save = xmlSaveToFilename(path, (char *) style->encoding, saveopts);

		xmlSaveDoc(save, doc);
		xmlSaveClose(save);
	}
}

static xmlNodePtr xml_stylesheet_node(const xmlNodePtr pi)
{
	xmlChar *content;
	xmlChar *xml;
	int n;
	xmlDocPtr d;
	xmlNodePtr root, node;

	content = pi->content;

	n = xmlStrlen(content) + 6;
	xml = malloc(n * sizeof(xmlChar));
	xmlStrPrintf(xml, n, "<x %s/>", content);
	d = xmlParseDoc(xml);
	xmlFree(xml);

	root = xmlDocGetRootElement(d);
	node = xmlCopyNode(root, 1);
	xmlFreeDoc(d);

	return node;
}

/* Read param from XML and encode in params list. */
static void read_param(const char **params, int *n, xmlNodePtr param)
{
	char *name, *value;

	name  = (char *) xmlGetProp(param, BAD_CAST "name");
	value = (char *) xmlGetProp(param, BAD_CAST "value");

	params[(*n)++] = name;
	params[(*n)++] = value;
}

/* Load stylesheet from disk and cache. */
static void load_stylesheet(xmlNodePtr cur, const bool include_identity)
{
	xmlDocPtr doc;
	xsltStylesheetPtr style;
	unsigned short nparams;
	const char **params = NULL;

	if (xmlHasProp(cur, BAD_CAST "path")) {
		xmlChar *path;

		path = xmlGetProp(cur, BAD_CAST "path");
		doc = read_xml_doc((char *) path, false);
		xmlFree(path);

		if (include_identity) {
			add_identity(doc);
		}
	} else {
		xmlChar *method;

		method = xmlGetProp(cur, BAD_CAST "method");

		if (xmlStrcmp(method, BAD_CAST "node") == 0) {
			doc = read_xml_mem((const char *) extract_xsl, extract_xsl_len);
		} else {
			doc = read_xml_mem((const char *) extract_text_xsl, extract_text_xsl_len);
		}

		xmlFree(method);

		xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
		xmlXPathObjectPtr obj = xmlXPathEvalExpression(BAD_CAST "/*/*/*", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			xmlChar *xpath;
			xmlNodePtr valueof;

			valueof = obj->nodesetval->nodeTab[0];

			xpath = xmlGetProp(cur, BAD_CAST "extract");
			xmlSetProp(valueof, BAD_CAST "select", xpath);
			xmlFree(xpath);
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);
	}

	style = xsltParseStylesheetDoc(doc);

	if (style == NULL) {
		xmlFreeDoc(doc);
		xmlUnlinkNode(cur);
		xmlFreeNode(cur);
		return;
	}

	cur->doc = (xmlDocPtr) style;

	if ((nparams = xmlChildElementCount(cur) + xmlChildElementCount(global_params)) > 0) {
		xmlNodePtr param;
		int n = 0;

		params = malloc((nparams * 2 + 1) * sizeof(char *));

		param = cur->children;
		while (param) {
			xmlNodePtr next = param->next;
			read_param(params, &n, param);
			xmlFreeNode(param);
			param = next;
		}

		param = global_params->children;
		while (param) {
			xmlNodePtr next = param->next;
			read_param(params, &n, param);
			param = next;
		}

		params[n] = NULL;
	}

	cur->children = (xmlNodePtr) params;
	cur->line = nparams;
}

/* Load stylesheets from disk and cache. */
static void load_stylesheets(xmlNodePtr stylesheets, const bool include_identity)
{
	xmlNodePtr cur;

	if (stylesheets == NULL) {
		return;
	}

	cur = stylesheets->children;
	while (cur) {
		xmlNodePtr next = cur->next;
		load_stylesheet(cur, include_identity);
		cur = next;
	}
}

/* Get stylesheets from xml-stylesheet instructions. */
static xmlNodePtr get_xml_stylesheets(xmlDocPtr doc)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr stylesheets;

	stylesheets = xmlNewNode(NULL, BAD_CAST "stylesheets");

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEval(BAD_CAST "//processing-instruction('xml-stylesheet')", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlNodePtr xml_stylesheet, style;
			xmlChar *href;

			xml_stylesheet = xml_stylesheet_node(obj->nodesetval->nodeTab[i]);
			href = xmlGetProp(xml_stylesheet, BAD_CAST "href");

			style = xmlNewChild(stylesheets, NULL, BAD_CAST "stylesheet", NULL);
			xmlSetProp(style, BAD_CAST "path", href);

			xmlFree(href);
			xmlFreeNode(xml_stylesheet);
		}

		load_stylesheets(stylesheets, false);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return stylesheets;
}

/* Free a cached stylesheet. */
static void free_stylesheet(xmlNodePtr cur)
{
	const char **params;
	int i;
	unsigned short nparams;

	xsltFreeStylesheet((xsltStylesheetPtr) cur->doc);
	cur->doc = NULL;

	params = (const char **) cur->children;
	nparams = cur->line;
	for (i = 0; i < nparams * 2; ++i) {
		xmlFree((char *) params[i]);
	}
	free(params);
	cur->children = NULL;
}

/* Free cached stylesheets. */
static void free_stylesheets(xmlNodePtr stylesheets)
{
	xmlNodePtr cur;

	if (stylesheets == NULL) {
		return;
	}

	cur = stylesheets->children;
	while (cur) {
		xmlNodePtr next = cur->next;
		free_stylesheet(cur);
		cur = next;
	}

	xmlFreeNode(stylesheets);
}

/* Apply stylesheets to a file. */
static void transform_file(const char *path, xmlNodePtr stylesheets, const char *out, bool overwrite)
{
	xmlDocPtr doc;
	xsltStylesheetPtr last = NULL;
	xmlNodePtr xml_stylesheets = NULL;

	if (verbosity >= VERBOSE) {
		fprintf(stderr, I_TRANSFORM, path);
	}

	if (path == NULL) {
		doc = read_xml_mem((const char *) null_input_xml, null_input_xml_len);
	} else {
		doc = read_xml_doc(path, PARSE_AS_HTML);
	}

	/* Transform using associated xml-stylesheets. */
	if (use_xml_stylesheets) {
		xml_stylesheets = get_xml_stylesheets(doc);

		if (xml_stylesheets->children != NULL) {
			if (preserve_dtd) {
				doc = transform_doc_preserve_dtd(doc, xml_stylesheets);
			} else {
				doc = transform_doc(doc, xml_stylesheets);
			}
		}
	}

	/* Transform using user-specified stylesheets. */
	if (preserve_dtd) {
		doc = transform_doc_preserve_dtd(doc, stylesheets);
	} else {
		doc = transform_doc(doc, stylesheets);
	}

	/* Use the output settings of the last stylesheet to determine how to
	 * save the end result. */
	if (stylesheets != NULL && stylesheets->last != NULL) {
		last = (xsltStylesheetPtr) stylesheets->last->doc;
	} else if (xml_stylesheets != NULL && xml_stylesheets->last != NULL) {
		last = (xsltStylesheetPtr) xml_stylesheets->last->doc;
	}

	if (last != NULL) {
		if (overwrite) {
			save_doc(doc, path, last);
		} else {
			save_doc(doc, out, last);
		}
	/* If no stylesheets are specified, save as-is. */
	} else {
		if (overwrite) {
			save_xml_doc(doc, path);
		} else {
			save_xml_doc(doc, out);
		}
	}

	if (use_xml_stylesheets) {
		free_stylesheets(xml_stylesheets);
	}

	xmlFreeDoc(doc);
}

/* Apply stylesheets to a list of files. */
static void transform_list(const char *path, xmlNodePtr stylesheets, const char *out, bool overwrite)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		if (!(f = fopen(path, "r"))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_LIST, path, strerror(errno));
			}
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		transform_file(line, stylesheets, out, overwrite);
	}

	if (path) {
		fclose(f);
	}
}

/* Add a parameter to a stylesheet. */
static void add_param(xmlNodePtr stylesheet, char *s)
{
	char *n, *v;
	xmlNodePtr p;

	n = strtok(s, "=");
	v = strtok(NULL, "");

	p = xmlNewChild(stylesheet, NULL, BAD_CAST "param", NULL);
	xmlSetProp(p, BAD_CAST "name", BAD_CAST n);
	xmlSetProp(p, BAD_CAST "value", BAD_CAST v);
}

static void add_str_param(xmlNodePtr stylesheet, char *s)
{
	xmlChar *name, *v, *value;
	xmlNodePtr p;

	name = BAD_CAST strtok(s, "=");
	v = BAD_CAST strtok(NULL, "");

	if (xmlStrchr(v, '"')) {
		if (xmlStrchr(v, '\'')) {
			fprintf(stderr, E_BAD_STRING_PARAM, name);
			return;
		}
		value = xmlStrdup((const xmlChar *)"'");
		value = xmlStrcat(value, v);
		value = xmlStrcat(value, (const xmlChar *)"'");
	} else {
		value = xmlStrdup((const xmlChar *)"\"");
		value = xmlStrcat(value, v);
		value = xmlStrcat(value, (const xmlChar *)"\"");
	}

	p = xmlNewChild(stylesheet, NULL, BAD_CAST "param", NULL);
	xmlSetProp(p, BAD_CAST "name", name);
	xmlSetProp(p, BAD_CAST "value", value);

	xmlFree(value);
}

/* Combine a single file into the combined document. */
static void combine_file(xmlNodePtr combined, const char *path)
{
	xmlDocPtr doc = read_xml_doc(path, PARSE_AS_HTML);
	xmlAddChild(combined, xmlCopyNode(xmlDocGetRootElement(doc), 1));
	xmlFreeDoc(doc);
}

/* Combine a list of files into the combined document. */
static void combine_file_list(xmlNodePtr combined, const char *path)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		if (!(f = fopen(path, "r"))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_LIST, path, strerror(errno));
			}
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		combine_file(combined, line);
	}

	if (path) {
		fclose(f);
	}
}

/* Transform input files as as combined document. */
static void transform_combined(int argc, char **argv, bool islist, const char *out, xmlNodePtr stylesheets)
{
	xmlDocPtr doc;
	xmlNodePtr combined;

	doc = xmlNewDoc(BAD_CAST "1.0");
	combined = xmlNewNode(NULL, BAD_CAST "combined");
	xmlDocSetRootElement(doc, combined);

	/* Combine all input files into a single document. */
	if (optind < argc) {
		int i;

		for (i = optind; i < argc; ++i) {
			if (islist) {
				combine_file_list(combined, argv[i]);
			} else {
				combine_file(combined, argv[i]);
			}
		}
	} else if (islist) {
		combine_file_list(combined, NULL);
	} else {
		combine_file(combined, "-");
	}

	doc = transform_doc(doc, stylesheets);

	/* Use the output settings of the last stylesheet to determine how to
	 * save the end result. */
	if (stylesheets->last) {
		xsltStylesheetPtr last;
		last = (xsltStylesheetPtr) stylesheets->last->doc;
		save_doc(doc, out, last);
	/* If no stylesheets were specified, save as-is. */
	} else {
		save_xml_doc(doc, out);
	}

	xmlFreeDoc(doc);
}

/* Show help/usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-s <stylesheet> [-[Pp] <name>=<value> ...] ...] [-o <file>] [-cdfilnqSvh?] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -c, --combine                      Combine input files into a single document.");
	puts("  -d, --preserve-dtd                 Preserve the original DTD.");
	puts("  -E, --extract-text <xpath>         Extracts the text value of the given XPath expression.");
	puts("  -e, --extract <xpath>              Extracts the node(s) matching the given XPath expression.");
	puts("  -f, --overwrite                    Overwrite input files.");
	puts("  -h, -?, --help                     Show usage message.");
	puts("  -i, --identity                     Include identity template in stylesheets.");
	puts("  -l, --list                         Treat input as list of files.");
	puts("  -n, --null-input                   Use a minimal XML document as the input");
	puts("  -o, --out <file>                   Output result of transformation to <path>.");
	puts("  -P, --string-param <name>=<value>  Pass parameters as strings to stylesheets.");
	puts("  -p, --param <name>=<value>         Pass parameters to stylesheets.");
	puts("  -q, --quiet                        Quiet mode.");
	puts("  -S, --xml-stylesheets              Apply associated stylesheets.");
	puts("  -s, --stylesheet <stylesheet>      Apply XSLT stylesheet to XML documents.");
	puts("  -v, --verbose                      Verbose output.");
	puts("  --version                          Show version information.");
	puts("  <file>                             XML documents to apply transformations to.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
static void show_version(void)
{
	printf("%s (xml-utils) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s, libxslt %s and libexslt %s\n",
		xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
}

int main(int argc, char **argv)
{
	int i;

	xmlNodePtr stylesheets, last_style = NULL;

	char *out = strdup("-");
	bool overwrite = false;
	bool islist = false;
	bool include_identity = false;
	bool combine = false;

	const char *sopts = "cdE:e:Ss:ilno:P:p:qfvh?";
	struct option lopts[] = {
		{"version"        , no_argument      , 0, 0},
		{"combine"        , no_argument      , 0, 'c'},
		{"preserve-dtd"   , no_argument      , 0, 'd'},
		{"extract-text"   , required_argument, 0, 'E'},
		{"extract"        , required_argument, 0, 'e'},
		{"overwrite"      , no_argument      , 0, 'f'},
		{"help"           , no_argument      , 0, 'h'},
		{"identity"       , no_argument      , 0, 'i'},
		{"list"           , no_argument      , 0, 'l'},
		{"null-input"     , no_argument      , 0, 'n'},
		{"out"            , required_argument, 0, 'o'},
		{"string-param"   , required_argument, 0, 'P'},
		{"param"          , required_argument, 0, 'p'},
		{"quiet"          , no_argument      , 0, 'q'},
		{"xml-stylesheets", no_argument      , 0, 'S'},
		{"stylesheet"     , required_argument, 0, 's'},
		{"verbose"        , no_argument      , 0, 'v'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	exsltRegisterAll();

	stylesheets = xmlNewNode(NULL, BAD_CAST "stylesheets");
	global_params = xmlNewNode(NULL, BAD_CAST "params");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case 'c':
				combine = true;
				break;
			case 'd':
				preserve_dtd = true;
				break;
			case 'E':
				last_style = xmlNewChild(stylesheets, NULL, BAD_CAST "stylesheet", NULL);
				xmlSetProp(last_style, BAD_CAST "extract", BAD_CAST optarg);
				xmlSetProp(last_style, BAD_CAST "method", BAD_CAST "text");
				break;
			case 'e':
				last_style = xmlNewChild(stylesheets, NULL, BAD_CAST "stylesheet", NULL);
				xmlSetProp(last_style, BAD_CAST "extract", BAD_CAST optarg);
				xmlSetProp(last_style, BAD_CAST "method", BAD_CAST "node");
				break;
			case 'S':
				use_xml_stylesheets = true;
				break;
			case 's':
				last_style = xmlNewChild(stylesheets, NULL, BAD_CAST "stylesheet", NULL);
				xmlSetProp(last_style, BAD_CAST "path", BAD_CAST optarg);
				break;
			case 'i':
				include_identity = true;
				break;
			case 'l':
				islist = true;
				break;
			case 'n':
				null_input = true;
				break;
			case 'o':
				free(out);
				out = strdup(optarg);
				break;
			case 'p':
				if (last_style == NULL) {
					add_param(global_params, optarg);
				} else {
					add_param(last_style, optarg);
				}
				break;
			case 'P':
				if (last_style == NULL) {
					add_str_param(global_params, optarg);
				} else {
					add_str_param(last_style, optarg);
				}
				break;
			case 'q':
				--verbosity;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'v':
				++verbosity;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	load_stylesheets(stylesheets, include_identity);

	if (null_input) {
		transform_file(NULL, stylesheets, out, false);
	} else if (combine) {
		transform_combined(argc, argv, islist, out, stylesheets);
	} else {
		if (optind < argc) {
			for (i = optind; i < argc; ++i) {
				if (islist) {
					transform_list(argv[i], stylesheets, out, overwrite);
				} else {
					transform_file(argv[i], stylesheets, out, overwrite);
				}
			}
		} else if (islist) {
			transform_list(NULL, stylesheets, out, overwrite);
		} else {
			transform_file("-", stylesheets, out, false);
		}
	}

	if (out) {
		free(out);
	}

	free_stylesheets(stylesheets);
	xmlFreeNode(global_params);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
