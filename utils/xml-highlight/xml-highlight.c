#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "xml-utils.h"
#include "languages.h"

#define PROG_NAME "xml-highlight"
#define VERSION "1.3.2"

#define PRE_KEYWORD_DELIM BAD_CAST " (\n"
#define POST_KEYWORD_DELIM BAD_CAST " .,);\n"

#define ELEM_XPATH "//*[processing-instruction('language')='%s']/text()"

static xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *expr)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(expr, ctx);

	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

static bool is_keyword(xmlChar *content, int content_len, int i, const xmlChar *keyword, int keyword_len, bool ignorecase)
{
	bool is;
	xmlChar s, e;
	int (*cmp)(const xmlChar *, const xmlChar *, int);

	cmp = ignorecase ? xmlStrncasecmp : xmlStrncmp;

	s = i == 0 ? ' ' : content[i - 1];
	e = i + keyword_len >= content_len - 1 ? ' ' : content[i + keyword_len];

	is = xmlStrchr(PRE_KEYWORD_DELIM, s) &&
	     cmp(content + i, keyword, keyword_len) == 0 &&
	     xmlStrchr(POST_KEYWORD_DELIM, e);

	return is;
}

static void highlight_keyword_in_node(xmlNodePtr node, const xmlChar *keyword, xmlNodePtr tag, bool ignorecase)
{
	xmlChar *content;
	int keyword_len, content_len;
	int i;

	content = xmlNodeGetContent(node);
	content_len = xmlStrlen(content);

	keyword_len = xmlStrlen(keyword);

	i = 0;
	while (i + keyword_len <= content_len) {
		if (is_keyword(content, content_len, i, keyword, keyword_len, ignorecase)) {
			xmlChar *s1 = xmlStrndup(content, i);
			xmlChar *s2 = xmlStrsub(content, i + keyword_len, content_len - (i + keyword_len));
			xmlChar *s3 = xmlStrsub(content, i, keyword_len);
			xmlNodePtr elem;

			xmlFree(content);

			xmlNodeSetContent(node, s1);
			xmlFree(s1);

			elem = xmlAddNextSibling(node, xmlCopyNode(tag, 1));
			xmlNodeSetContent(elem, s3);
			xmlFree(s3);

			node = xmlAddNextSibling(elem, xmlNewText(s2));

			content = s2;
			content_len = xmlStrlen(s2);
			i = 0;
		} else {
			++i;
		}
	}

	xmlFree(content);
}

static void highlight_area_in_node(xmlNodePtr node, const xmlChar *start, const xmlChar *end, xmlNodePtr tag, bool ignorecase)
{
	xmlChar *content;
	int i, slen, elen;
	int (*cmp)(const xmlChar *, const xmlChar *, int);
	const xmlChar *(*sub)(const xmlChar *, const xmlChar *);

	content = xmlNodeGetContent(node);
	slen = xmlStrlen(start);
	elen = xmlStrlen(end);

	cmp = ignorecase ? xmlStrncasecmp : xmlStrncmp;
	sub = ignorecase ? xmlStrcasestr : xmlStrstr;

	for (i = 0; content[i]; ++i) {
		if (cmp(content + i, start, slen) == 0) {
			const xmlChar *e;
			int len;
			xmlChar *s1, *s2, *s3;
			xmlNodePtr elem;

			e = sub(content + i + 1, end);
			if (!e) {
				e = content + (xmlStrlen(content) - 1);
			}

			len = e - (content + i) + elen;

			s1 = xmlStrndup(content, i);
			s2 = xmlStrndup(content + i, len);
			s3 = xmlStrdup(content + i + len);

			xmlFree(content);

			xmlNodeSetContent(node, s1);
			xmlFree(s1);

			elem = xmlAddNextSibling(node, xmlCopyNode(tag, 1));
			xmlNodeSetContent(elem, s2);
			xmlFree(s2);

			node = xmlAddNextSibling(elem, xmlNewText(s3));

			content = s3;

			i = 0;
		}
	}

	xmlFree(content);
}

static void highlight_keyword_in_nodes(xmlNodeSetPtr nodes, const xmlChar *keyword, xmlNodePtr tag, bool ignorecase)
{
	int i;
	for (i = 0; i < nodes->nodeNr; ++i) {
		highlight_keyword_in_node(nodes->nodeTab[i], keyword, tag, ignorecase);
	}
}

static void highlight_area_in_nodes(xmlNodeSetPtr nodes, const xmlChar *start, const xmlChar *end, xmlNodePtr tag, bool ignorecase)
{
	int i;
	for (i = 0; i < nodes->nodeNr; ++i) {
		highlight_area_in_node(nodes->nodeTab[i], start, end, tag, ignorecase);
	}
}

static void highlight_keyword_in_doc(xmlDocPtr doc, const xmlChar *lang, const xmlChar *keyword, xmlNodePtr tag, bool ignorecase)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	
	xmlChar xpath[256];

	xmlStrPrintf(xpath, 256, ELEM_XPATH, lang);

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		highlight_keyword_in_nodes(obj->nodesetval, keyword, tag, ignorecase);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

static void highlight_area_in_doc(xmlDocPtr doc, const xmlChar *lang, const xmlChar *start, const xmlChar *end, xmlNodePtr tag, bool ignorecase)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	xmlChar xpath[256];

	xmlStrPrintf(xpath, 256, ELEM_XPATH, lang);

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		highlight_area_in_nodes(obj->nodesetval, start, end, tag, ignorecase);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

static xmlNodePtr get_class(const xmlChar *class, xmlDocPtr classes, xmlDocPtr syntax)
{
	xmlNodePtr node;
	xmlChar xpath[256];

	xmlStrPrintf(xpath, 256, "//class[@id='%s']", class);

	node = first_xpath_node(classes, NULL, xpath);

	if (!node) {
		node = first_xpath_node(syntax, NULL, xpath);
	}

	return node;
}

static void highlight_keyword_node_in_doc(xmlDocPtr doc, xmlDocPtr syntax, xmlDocPtr classes, const xmlChar *lang, xmlNodePtr node, bool ignorecase)
{
	xmlChar *keyword, *class;
	xmlNodePtr tag;

	keyword = xmlGetProp(node, BAD_CAST "match");
	class   = xmlGetProp(node, BAD_CAST "class");

	if (class) {
		tag = xmlFirstElementChild(get_class(class, classes, syntax));
	} else {
		tag = xmlFirstElementChild(node);
	}

	if (tag) {
		highlight_keyword_in_doc(doc, lang, keyword, tag, ignorecase);
	}

	xmlFree(keyword);
	xmlFree(class);
}

static void highlight_area_node_in_doc(xmlDocPtr doc, xmlDocPtr syntax, xmlDocPtr classes, const xmlChar *lang, xmlNodePtr node, bool ignorecase)
{
	xmlChar *start, *end, *class;
	xmlNodePtr tag;

	start = xmlGetProp(node, BAD_CAST "start");
	end   = xmlGetProp(node, BAD_CAST "end");
	class = xmlGetProp(node, BAD_CAST "class");

	if (class) {
		tag = xmlFirstElementChild(get_class(class, classes, syntax));
	} else {
		tag = xmlFirstElementChild(node);
	}

	if (tag) {
		highlight_area_in_doc(doc, lang, start, end, tag, ignorecase);
	}

	xmlFree(start);
	xmlFree(end);
	xmlFree(class);
}

static void highlight_language_in_doc(xmlDocPtr doc, xmlDocPtr syntax,
	xmlDocPtr classes, xmlNodePtr language)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlChar *lang, *case_insen;
	bool ignorecase = false;

	lang = xmlGetProp(language, BAD_CAST "name");
	case_insen = xmlGetProp(language, BAD_CAST "caseInsensitive");

	if (case_insen) {
		ignorecase = xmlStrcmp(case_insen, BAD_CAST "yes") == 0;
	}

	xmlFree(case_insen);

	ctx = xmlXPathNewContext(syntax);
	ctx->node = language;

	obj = xmlXPathEvalExpression(BAD_CAST "area", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			highlight_area_node_in_doc(doc, syntax, classes, lang, obj->nodesetval->nodeTab[i], ignorecase);
		}
	}

	xmlXPathFreeObject(obj);

	obj = xmlXPathEvalExpression(BAD_CAST "keyword", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			highlight_keyword_node_in_doc(doc, syntax, classes, lang, obj->nodesetval->nodeTab[i], ignorecase);
		}
	}

	xmlFree(lang);

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

static void highlight_syntax_in_doc(xmlDocPtr doc, xmlDocPtr syntax, xmlDocPtr classes)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(syntax);
	obj = xmlXPathEvalExpression(BAD_CAST "//language", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			highlight_language_in_doc(doc, syntax, classes,
				obj->nodesetval->nodeTab[i]);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

static void highlight_syntax_in_file(const char *fname, const char *syntax, const char *classes, bool overwrite)
{
	xmlDocPtr doc;
	xmlDocPtr syndoc;
	xmlDocPtr classdoc;

	if (syntax) {
		syndoc = read_xml_doc(syntax);
	} else {
		syndoc = read_xml_mem((const char *) syntax_xml, syntax_xml_len);
	}

	if (classes) {
		classdoc = read_xml_doc(classes);
	} else {
		classdoc = read_xml_mem((const char *) classes_xml, classes_xml_len);
	}

	doc = read_xml_doc(fname);

	highlight_syntax_in_doc(doc, syndoc, classdoc);

	if (overwrite) {
		save_xml_doc(doc, fname);
	} else {
		save_xml_doc(doc, "-");
	}

	xmlFreeDoc(doc);
	xmlFreeDoc(syndoc);
	xmlFreeDoc(classdoc);
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<document>...]");
	puts("");
	puts("Options:");
	puts("  -c, --classes <XML>  Use a custom classes XML file.");
	puts("  -f, --overwrite      Overwrite input documents.");
	puts("  -h, -?, --help       Show usage message.");
	puts("  -s, --syntax <XML>   Use a custom syntax definitions XML file.");
	puts("  --version            Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (xml-utils) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	char *syntax = NULL;
	char *classes = NULL;

	const char *sopts = "c:fs:h?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"classes"  , required_argument, 0, 'c'},
		{"overwrite", no_argument      , 0, 'f'},
		{"syntax"   , required_argument, 0, 's'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'c':
				if (!classes)
					classes = strdup(optarg);
				break;
			case 'f':
				overwrite = true;
				break;
			case 's':
				if (!syntax)
					syntax = strdup(optarg);
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (optind >= argc) {
		highlight_syntax_in_file("-", syntax, classes, false);
	} else {
		for (i = optind; i < argc; ++i) {
			highlight_syntax_in_file(argv[i], syntax, classes, overwrite);
		}
	}

	free(syntax);
	free(classes);

	xmlCleanupParser();

	return 0;
}
