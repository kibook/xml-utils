#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define PROG_NAME "xml-diff"

#define DIFF_NS_PREFIX BAD_CAST "diff"
#define DIFF_NS BAD_CAST "http://khzae.net/1/xml/xml-utils/xml-diff"

bool same_node(xmlNodePtr a, xmlNodePtr b)
{
	xmlBufferPtr buf_a, buf_b;
	bool eq;

	buf_a = xmlBufferCreate();
	buf_b = xmlBufferCreate();

	xmlNodeDump(buf_a, a->doc, a, 0, 0);
	xmlNodeDump(buf_b, b->doc, b, 0, 0);

	eq = xmlStrcmp(buf_a->content, buf_b->content) == 0;

	xmlBufferFree(buf_a);
	xmlBufferFree(buf_b);

	return eq;
}

xmlXPathContextPtr new_context(xmlDocPtr doc)
{
	xmlXPathContextPtr ctx;

	ctx = xmlXPathNewContext(doc);
	xmlXPathRegisterNs(ctx, DIFF_NS_PREFIX, DIFF_NS);

	return ctx;
}

xmlNodePtr first_xpath_node(xmlDocPtr doc, const xmlChar *expr)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = new_context(doc);
	obj = xmlXPathEvalExpression(expr, ctx);

	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

/* Assign each element node in a document a unique ID. */
void id_nodes(xmlDocPtr doc, xmlNsPtr diffns)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = new_context(doc);

	if (!xmlXPathNodeSetIsEmpty((obj = xmlXPathEvalExpression(BAD_CAST "//*", ctx))->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar id[256];
			xmlStrPrintf(id, 256, "%d", i);
			xmlSetNsProp(obj->nodesetval->nodeTab[i], diffns, BAD_CAST "id", id);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

void unid_nodes(xmlDocPtr doc, xmlNsPtr diffns)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = new_context(doc);

	if (!xmlXPathNodeSetIsEmpty((obj = xmlXPathEvalExpression(BAD_CAST "//*[@diff:id]", ctx))->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlUnsetNsProp(obj->nodesetval->nodeTab[i], diffns, BAD_CAST "id");
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Indicate all changes. */
void indicate_changes(xmlDocPtr doc_a, xmlDocPtr doc_b, xmlNsPtr diffns)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = new_context(doc_b);

	if (!xmlXPathNodeSetIsEmpty((obj = xmlXPathEvalExpression(BAD_CAST "//*[@diff:id]", ctx))->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlNodePtr node_a, node_b;
			xmlChar xpath[256], *id;

			node_b = obj->nodesetval->nodeTab[i];

			id = xmlGetNsProp(node_b, BAD_CAST "id", DIFF_NS);
			xmlStrPrintf(xpath, 256, "//*[@diff:id='%s']", id);
			xmlFree(id);

			node_a = first_xpath_node(doc_a, xpath);

			if (!node_a) {
				xmlSetNsProp(node_b, diffns, BAD_CAST "change", BAD_CAST "add");
			} else if (!same_node(node_a, node_b)) {
				xmlSetNsProp(node_b, diffns, BAD_CAST "change", BAD_CAST "modify");
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Remove change marks from parents whose children also have change marks. */
void remove_duplicate_marks(xmlDocPtr doc, xmlNsPtr diffns)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = new_context(doc);

	obj = xmlXPathEvalExpression(BAD_CAST "//*[@diff:change and .//*[@diff:change]]", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlUnsetNsProp(obj->nodesetval->nodeTab[i], diffns, BAD_CAST "change");
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* If all children of an element are marked, only mark the parent element. */
void consolidate_marks(xmlDocPtr doc, xmlNsPtr diffns)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = new_context(doc);

	obj = xmlXPathEvalExpression(BAD_CAST "//*[not(@diff:change) and *[@diff:change] and not(*[not(@diff:change)])]", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlNodePtr cur;

			xmlChar *parent_type = BAD_CAST "add";

			for (cur = obj->nodesetval->nodeTab[i]->children; cur; cur = cur->next) {
				xmlChar *child_type;

				if (cur->type != XML_ELEMENT_NODE)
					continue;

				child_type = xmlGetNsProp(cur, BAD_CAST "change", DIFF_NS);

				if (xmlStrcmp(child_type, BAD_CAST "add") != 0) {
					parent_type = BAD_CAST "modify";
				}

				xmlFree(child_type);

				xmlUnsetNsProp(cur, diffns, BAD_CAST "change");
			}

			xmlSetNsProp(obj->nodesetval->nodeTab[i], diffns, BAD_CAST "change", parent_type);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] <OLD> <NEW>");
	puts("");
	puts("Options:");
	puts("  -h -?  Show usage message.");
}

int main(int argc, char **argv)
{
	int i;
	xmlDocPtr doc_a, doc_b;
	xmlNsPtr diffns_a, diffns_b;

	while ((i = getopt(argc, argv, "h?")) != -1) {
		switch (i) {
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (optind > argc - 2) {
		return 1;
	}

	doc_a = xmlReadFile(argv[optind], NULL, 0);
	doc_b = xmlReadFile(argv[optind + 1], NULL, 0);

	diffns_a = xmlNewNs(xmlDocGetRootElement(doc_a), DIFF_NS, BAD_CAST "diff");
	diffns_b = xmlNewNs(xmlDocGetRootElement(doc_b), DIFF_NS, BAD_CAST "diff");

	id_nodes(doc_a, diffns_a);
	id_nodes(doc_b, diffns_b);

	indicate_changes(doc_a, doc_b, diffns_b);
	unid_nodes(doc_b, diffns_b);
	remove_duplicate_marks(doc_b, diffns_b);
	consolidate_marks(doc_b, diffns_b);

	xmlSaveFile("-", doc_b);

	xmlFreeDoc(doc_a);
	xmlFreeDoc(doc_b);

	xmlCleanupParser();

	return 0;
}
