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

#define PROG_NAME "xml-transform"
#define VERSION "1.0.0"

#define INF_PREFIX PROG_NAME ": INFO: "
#define ERR_PREFIX PROG_NAME ": ERROR: "

#define I_TRANSFORM INF_PREFIX "Transforming %s...\n"

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s: %s\n"
#define E_FILE_NO_WRITE ERR_PREFIX "Could not open file for writing: %s: %s\n"

#define EXIT_OS_ERROR 1

static enum verbosity { QUIET, NORMAL, VERBOSE } verbosity = NORMAL;
static bool preserve_dtd = false;

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

/* Apply stylesheets to a file. */
static void transform_file(const char *path, xmlNodePtr stylesheets, const char *out, bool overwrite)
{
	xmlDocPtr doc;
	xsltStylesheetPtr last;

	if (verbosity >= VERBOSE) {
		fprintf(stderr, I_TRANSFORM, path);
	}

	doc = read_xml_doc(path);

	if (preserve_dtd) {
		doc = transform_doc_preserve_dtd(doc, stylesheets);
	} else {
		doc = transform_doc(doc, stylesheets);
	}

	/* Use the output settings of the last stylesheet to determine how to
	 * save the end result. */
	last = (xsltStylesheetPtr) stylesheets->last->doc;

	if (overwrite) {
		save_doc(doc, path, last);
	} else {
		save_doc(doc, out, last);
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

/* Load stylesheets from disk and cache. */
static void load_stylesheets(xmlNodePtr stylesheets, const bool include_identity)
{
	xmlNodePtr cur;

	for (cur = stylesheets->children; cur; cur = cur->next) {
		xmlChar *path;
		xmlDocPtr doc;
		xsltStylesheetPtr style;
		unsigned short nparams;
		const char **params = NULL;

		path = xmlGetProp(cur, BAD_CAST "path");
		doc = read_xml_doc((char *) path);
		xmlFree(path);

		if (include_identity) {
			add_identity(doc);
		}

		style = xsltParseStylesheetDoc(doc);

		cur->doc = (xmlDocPtr) style;

		if ((nparams = xmlChildElementCount(cur)) > 0) {
			xmlNodePtr param;
			int n = 0;

			params = malloc((nparams * 2 + 1) * sizeof(char *));

			param = cur->children;
			while (param) {
				xmlNodePtr next;
				char *name, *value;

				next = param->next;

				name  = (char *) xmlGetProp(param, BAD_CAST "name");
				value = (char *) xmlGetProp(param, BAD_CAST "value");

				params[n++] = name;
				params[n++] = value;

				xmlFreeNode(param);
				param = next;
			}

			params[n] = NULL;
		}

		cur->children = (xmlNodePtr) params;
		cur->line = nparams;
	}
}

/* Free cached stylesheets. */
static void free_stylesheets(xmlNodePtr stylesheets)
{
	xmlNodePtr cur;

	for (cur = stylesheets->children; cur; cur = cur->next) {
		const char **params;
		int i;
		unsigned short nparams;

		xsltFreeStylesheet((xsltStylesheetPtr) cur->doc);
		cur->doc = NULL;

		params = (const char **) cur->children;
		nparams = cur->line;
		for (i = 0; i < nparams; ++i) {
			xmlFree((char *) params[i]);
			xmlFree((char *) params[i + 1]);
		}
		free(params);
		cur->children = NULL;
	}
}

/* Show help/usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-s <stylesheet> [-p <name>=<value> ...] ...] [-o <file>] [-dfilqvh?] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -d, --preserve-dtd             Preserve the original DTD.");
	puts("  -f, --overwrite                Overwrite input files.");
	puts("  -h, -?, --help                 Show usage message.");
	puts("  -i, --identity                 Include identity template in stylesheets.");
	puts("  -l, --list                     Treat input as list of files.");
	puts("  -o, --out <file>               Output result of transformation to <path>.");
	puts("  -p, --param <name>=<value>     Pass parameters to stylesheets.");
	puts("  -q, --quiet                    Quiet mode.");
	puts("  -s, --stylesheet <stylesheet>  Apply XSLT stylesheet to XML documents.");
	puts("  -v, --verbose                  Verbose output.");
	puts("  --version                      Show version information.");
	puts("  <file>                         XML documents to apply transformations to.");
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

	const char *sopts = "ds:ilo:p:qfvh?";
	struct option lopts[] = {
		{"version"     , no_argument      , 0, 0},
		{"preserve-dtd", no_argument      , 0, 'd'},
		{"help"        , no_argument      , 0, 'h'},
		{"identity"    , no_argument      , 0, 'i'},
		{"list"        , no_argument      , 0, 'l'},
		{"out"         , required_argument, 0, 'o'},
		{"param"       , required_argument, 0, 'p'},
		{"quiet"       , no_argument      , 0, 'q'},
		{"stylesheet"  , required_argument, 0, 's'},
		{"verbose"     , no_argument      , 0, 'v'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	exsltRegisterAll();

	stylesheets = xmlNewNode(NULL, BAD_CAST "stylesheets");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'd':
				preserve_dtd = true;
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
			case 'o':
				free(out);
				out = strdup(optarg);
				break;
			case 'p':
				add_param(last_style, optarg);
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

	if (out) {
		free(out);
	}

	free_stylesheets(stylesheets);
	xmlFreeNode(stylesheets);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
