#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "languages.h"

#define PROG_NAME "s1kd-highlight"

#define PRE_KEYWORD_DELIM " (\n"
#define POST_KEYWORD_DELIM " .,);\n"

xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *expr)
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

bool is_keyword(xmlChar *content, int content_len, int i, xmlChar *keyword, int keyword_len)
{
	xmlChar *sub;
	bool is;
	char s, e;

	sub = xmlStrsub(content, i, keyword_len);

	s = i == 0 ? ' ' : (char) content[i - 1];
	e = i + keyword_len == content_len - 1 ? ' ' : (char) content[i + keyword_len];

	is = strchr(PRE_KEYWORD_DELIM, s) &&
	     xmlStrcmp(sub, keyword) == 0 &&
	     strchr(POST_KEYWORD_DELIM, e);

	xmlFree(sub);

	return is;
}

void highlight_keyword_in_node(xmlNodePtr node, const xmlChar *keyword, xmlNodePtr tag)
{
	xmlChar *content;
	xmlChar *term;
	int term_len, content_len;
	int i;

	content = xmlNodeGetContent(node);
	content_len = xmlStrlen(content);

	term = xmlStrdup(BAD_CAST keyword);
	term_len = xmlStrlen(term);

	i = 0;
	while (i + term_len < content_len) {
		if (is_keyword(content, content_len, i, term, term_len)) {
			xmlChar *s1 = xmlStrndup(content, i);
			xmlChar *s2 = xmlStrsub(content, i + term_len, xmlStrlen(content));
			xmlNodePtr elem;

			xmlFree(content);

			xmlNodeSetContent(node, s1);
			xmlFree(s1);

			elem = xmlAddNextSibling(node, xmlCopyNode(tag, 1));
			xmlNodeSetContent(elem, term);

			node = xmlAddNextSibling(elem, xmlNewText(s2));

			content = s2;
			content_len = xmlStrlen(s2);
			i = 0;
		} else {
			++i;
		}
	}

	xmlFree(term);
	xmlFree(content);
}

void highlight_area_in_node(xmlNodePtr node, const xmlChar *start, const xmlChar *end, xmlNodePtr tag)
{
	xmlChar *content;
	int i, n;

	content = xmlNodeGetContent(node);
	n = xmlStrlen(start);

	for (i = 0; content[i]; ++i) {
		if (xmlStrncmp(content + i, start, n) == 0) {
			const xmlChar *e;
			int len;

			if ((e = xmlStrstr(content + i + 1, end))) {
				xmlChar *s1, *s2, *s3;
				xmlNodePtr elem;

				len = e - (content + i) + n;

				s1 = xmlStrndup(content, i);
				s2 = xmlStrndup(content + i, len);
				s3 = xmlStrdup(content + i + len);

				xmlFree(content);

				xmlNodeSetContent(node, s1);
				xmlFree(s1);

				elem = xmlAddNextSibling(node, xmlCopyNode(tag, 1));
				xmlNodeSetContent(elem, s2);

				node = xmlAddNextSibling(elem, xmlNewText(s3));

				content = s3;

				i = 0;
			}
		}
	}
}

void highlight_keyword_in_nodes(xmlNodeSetPtr nodes, const xmlChar *keyword, xmlNodePtr tag)
{
	int i;
	for (i = 0; i < nodes->nodeNr; ++i) {
		highlight_keyword_in_node(nodes->nodeTab[i], keyword, tag);
	}
}

void highlight_area_in_nodes(xmlNodeSetPtr nodes, const xmlChar *start, const xmlChar *end, xmlNodePtr tag)
{
	int i;
	for (i = 0; i < nodes->nodeNr; ++i) {
		highlight_area_in_node(nodes->nodeTab[i], start, end, tag);
	}
}

void highlight_keyword_in_doc(xmlDocPtr doc, const xmlChar *vs, const xmlChar *keyword, xmlNodePtr tag)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	
	xmlChar xpath[256];

	xmlStrPrintf(xpath, 256, "//verbatimText[@verbatimStyle='%s']/text()", vs);

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		highlight_keyword_in_nodes(obj->nodesetval, keyword, tag);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

void highlight_area_in_doc(xmlDocPtr doc, const xmlChar *vs, const xmlChar *start, const xmlChar *end, xmlNodePtr tag)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	xmlChar xpath[256];

	xmlStrPrintf(xpath, 256, "//verbatimText[@verbatimStyle='%s']/text()", vs);

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		highlight_area_in_nodes(obj->nodesetval, start, end, tag);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

xmlNodePtr get_class(const xmlChar *class, xmlDocPtr classes, xmlDocPtr syntax)
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

void highlight_keyword_node_in_doc(xmlDocPtr doc, xmlDocPtr syntax, xmlDocPtr classes, const xmlChar *vs, xmlNodePtr node)
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
		highlight_keyword_in_doc(doc, vs, keyword, tag);
	}

	xmlFree(keyword);
	xmlFree(class);
}

void highlight_area_node_in_doc(xmlDocPtr doc, xmlDocPtr syntax, xmlDocPtr classes, const xmlChar *vs, xmlNodePtr node)
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
		highlight_area_in_doc(doc, vs, start, end, tag);
	}

	xmlFree(start);
	xmlFree(end);
	xmlFree(class);
}

void highlight_language_in_doc(xmlDocPtr doc, xmlDocPtr syntax, xmlDocPtr high,
	xmlDocPtr classes, xmlNodePtr language)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlChar *vs;
	xmlChar xpath[256];
	xmlChar *lang;

	lang = xmlGetProp(language, BAD_CAST "name");
	xmlStrPrintf(xpath, 256, "//verbatimText[@language='%s']", lang);
	xmlFree(lang);
	ctx = xmlXPathNewContext(high);
	obj = xmlXPathEvalExpression(xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		vs = NULL;
	} else {
		vs = xmlGetProp(obj->nodesetval->nodeTab[0], BAD_CAST "verbatimStyle");
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (!vs) {
		return;
	}

	ctx = xmlXPathNewContext(syntax);
	ctx->node = language;

	obj = xmlXPathEvalExpression(BAD_CAST "area", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			highlight_area_node_in_doc(doc, syntax, classes, vs, obj->nodesetval->nodeTab[i]);
		}
	}

	xmlXPathFreeObject(obj);

	obj = xmlXPathEvalExpression(BAD_CAST "keyword", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			highlight_keyword_node_in_doc(doc, syntax, classes, vs, obj->nodesetval->nodeTab[i]);
		}
	}

	xmlFree(vs);

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

void highlight_syntax_in_doc(xmlDocPtr doc, xmlDocPtr syntax, xmlDocPtr high, xmlDocPtr classes, const char *lang)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	char xpath[256];

	ctx = xmlXPathNewContext(syntax);

	if (lang) {
		snprintf(xpath, 256, "//language[@name='%s']", lang);
	} else {
		strcpy(xpath, "//language");
	}

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			highlight_language_in_doc(doc, syntax, high, classes, obj->nodesetval->nodeTab[i]);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

void highlight_syntax_in_file(const char *fname, const char *syntax,
	const char *lang, const char *high, const char *classes, bool overwrite)
{
	xmlDocPtr doc;
	xmlDocPtr syndoc;
	xmlDocPtr highdoc;
	xmlDocPtr classdoc;

	if (syntax) {
		syndoc = xmlReadFile(syntax, NULL, 0);
	} else {
		syndoc = xmlReadMemory((const char *) syntax_xml, syntax_xml_len, NULL, NULL, 0);
	}

	if (high) {
		highdoc = xmlReadFile(high, NULL, 0);
	} else {
		highdoc = xmlReadMemory((const char *) highlight_xml,
			highlight_xml_len, NULL, NULL, 0);
	}

	if (classes) {
		classdoc = xmlReadFile(classes, NULL, 0);
	} else {
		classdoc = xmlReadMemory((const char *) classes_xml,
			classes_xml_len, NULL, NULL, 0);
	}

	doc = xmlReadFile(fname, NULL, 0);

	highlight_syntax_in_doc(doc, syndoc, highdoc, classdoc, lang);

	if (overwrite) {
		xmlSaveFile(fname, doc);
	} else {
		xmlSaveFile("-", doc);
	}

	xmlFreeDoc(doc);
	xmlFreeDoc(syndoc);
	xmlFreeDoc(highdoc);
	xmlFreeDoc(classdoc);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<dmodule>...]");
	puts("");
	puts("Options:");
	puts("  -h -?      Show usage message.");
	puts("  -c <XML>   Use a custom classes XML file.");
	puts("  -f         Overwrite input data modules.");
	puts("  -l <lang>  Highlight a specific language only.");
	puts("  -s <XML>   Use a custom syntax definitions XML file.");
	puts("  -v <XML>   Use a custom highlight definitions XML file.");
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	char *syntax = NULL;
	char *lang = NULL;
	char *high = NULL;
	char *classes = NULL;

	while ((i = getopt(argc, argv, "c:fl:s:v:h?")) != -1) {
		switch (i) {
			case 'c':
				if (!classes)
					classes = strdup(optarg);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'l':
				if (!lang)
					lang = strdup(optarg);
				break;
			case 's':
				if (!syntax)
					syntax = strdup(optarg);
				break;
			case 'v':
				if (!high)
					high = strdup(optarg);
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	if (optind >= argc) {
		highlight_syntax_in_file("-", syntax, lang, high, classes, false);
	} else {
		for (i = optind; i < argc; ++i) {
			highlight_syntax_in_file(argv[i], syntax, lang, high, classes, overwrite);
		}
	}

	free(syntax);
	free(lang);
	free(high);
	free(classes);

	return 0;
}
