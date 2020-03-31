#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xml-utils.h"

#define PROG_NAME "xml-validate"
#define VERSION "0.1.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define SUCCESS_PREFIX PROG_NAME ": SUCCESS: "
#define FAILED_PREFIX PROG_NAME ": FAILED: "

#define E_BAD_LIST ERR_PREFIX "Could not read list file: %s\n"
#define E_MAX_SCHEMA_PARSERS ERR_PREFIX "Maximum number of schemas reached: %d\n"
#define E_BAD_IDREF ERR_PREFIX "No matching ID for '%s' (%s line %ld).\n"

#define EXIT_MAX_SCHEMAS 2
#define EXIT_MISSING_SCHEMA 3

#define XML_SCHEMA_URI BAD_CAST "http://www.w3.org/2001/XMLSchema"
#define XSI_URI BAD_CAST "http://www.w3.org/2001/XMLSchema-instance"

static enum verbosity_level {SILENT, NORMAL, VERBOSE} verbosity = NORMAL;

/* Cache schemas to prevent parsing them twice (mainly needed when accessing
 * the schema over a network)
 */
struct xml_schema_parser {
	char *url;
	xmlDocPtr doc;
	xmlSchemaParserCtxtPtr ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;
};

/* Initial max schema parsers. */
static unsigned SCHEMA_PARSERS_MAX = 1;

static struct xml_schema_parser *schema_parsers;

static int schema_parser_count = 0;

static void print_error(void *userData, xmlErrorPtr error)
{
	if (error->file) {
		fprintf(userData, ERR_PREFIX "%s (%d): %s", error->file, error->line, error->message);
	} else {
		fprintf(userData, ERR_PREFIX "%s", error->message);
	}
}

static void suppress_error(void *userData, xmlErrorPtr error)
{
}

xmlStructuredErrorFunc schema_errfunc = print_error;

static struct xml_schema_parser *get_schema_parser(const char *url)
{
	int i;

	for (i = 0; i < schema_parser_count; ++i) {
		if (strcmp(schema_parsers[i].url, url) == 0) {
			return &(schema_parsers[i]);
		}
	}

	return NULL;
}

static struct xml_schema_parser *add_schema_parser(char *url)
{
	struct xml_schema_parser *parser;

	xmlDocPtr doc;
	xmlSchemaParserCtxtPtr ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;

	doc = read_xml_doc(url);
	ctxt = xmlSchemaNewDocParserCtxt(doc);
	schema = xmlSchemaParse(ctxt);
	valid_ctxt = xmlSchemaNewValidCtxt(schema);

	xmlSchemaSetParserStructuredErrors(ctxt, schema_errfunc, stderr);
	xmlSchemaSetValidStructuredErrors(valid_ctxt, schema_errfunc, stderr);

	schema_parsers[schema_parser_count].url = url;
	schema_parsers[schema_parser_count].doc = doc;
	schema_parsers[schema_parser_count].ctxt = ctxt;
	schema_parsers[schema_parser_count].schema = schema;
	schema_parsers[schema_parser_count].valid_ctxt = valid_ctxt;

	parser = &schema_parsers[schema_parser_count];

	++schema_parser_count;

	return parser;
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-s <path>] [-flqvh?] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -f, --filenames       List invalid files.");
	puts("  -h, -?, --help        Show help/usage message.");
	puts("  -l, --list            Treat input as list of filenames.");
	puts("  -q, --quiet           Silent (no output).");
	puts("  -s, --schema <path>   Validate against the given schema.");
	puts("  -v, --verbose         Verbose output.");
	puts("  --version             Show version information.");
	puts("  <file>                Any number of XML documents to validate.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (xml-utils) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

/* Check if a given ID attribute with a given name exists with a given ID value. */
static int check_id_exists_in_doc(const xmlDocPtr doc, const char *fname, bool attr, const xmlChar *name, const xmlChar *id)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err;

	ctx = xmlXPathNewContext(doc);
	xmlXPathRegisterVariable(ctx, BAD_CAST "name", xmlXPathNewString(name));
	xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewString(id));

	if (attr) {
		obj = xmlXPathEvalExpression(BAD_CAST "//@*[name()=$name and .=$id]", ctx);
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "//*[name()=$name and .=$id]", ctx);
	}

	err = xmlXPathNodeSetIsEmpty(obj->nodesetval);

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return err;
}

/* Check if a given ID exists in a document. */
static int check_id_exists(const xmlDocPtr schema, const xmlDocPtr doc, const char *fname, const xmlChar *id)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(schema);
	xmlXPathRegisterNs(ctx, BAD_CAST "xs", XML_SCHEMA_URI);

	obj = xmlXPathEvalExpression(BAD_CAST "//xs:attribute[@type='xs:ID']|//xs:element[@type='xs:ID']", ctx);
	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *name = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "name");
			err += check_id_exists_in_doc(
				doc,
				fname,
				xmlStrcmp(obj->nodesetval->nodeTab[i]->name, BAD_CAST "attribute") == 0,
				name,
				id);
			xmlFree(name);
		}
	}
	xmlXPathFreeObject(obj);

	xmlXPathFreeContext(ctx);

	return err;
}

/* Check if a specific IDREF value is valid. */
static int check_specific_idref(const xmlDocPtr schema, const xmlDocPtr doc, const char *fname, bool attr, const xmlChar *name)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(doc);
	xmlXPathRegisterVariable(ctx, BAD_CAST "name", xmlXPathNewString(name));

	if (attr) {
		obj = xmlXPathEvalExpression(BAD_CAST "//@*[name()=$name]", ctx);
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "//*[name()=$name]", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			int e;
			xmlNodePtr node = obj->nodesetval->nodeTab[i];
			xmlChar *id = xmlNodeGetContent(node);
			e = check_id_exists(schema, doc, fname, id);

			if (e) {
				fprintf(stderr, E_BAD_IDREF, (char *) id, fname, xmlGetLineNo(node));
			}

			err += e;

			xmlFree(id);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return err;
}

/* Check all IDREF values in a document. */
static int check_idref(const xmlDocPtr schema, const xmlDocPtr doc, const char *fname)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(schema);
	xmlXPathRegisterNs(ctx, BAD_CAST "xs", XML_SCHEMA_URI);

	obj = xmlXPathEvalExpression(BAD_CAST "//xs:attribute[@type='xs:IDREF']|//xs:element[@type='xs:IDREF']", ctx);
	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *name = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "name");
			err += check_specific_idref(
				schema,
				doc,
				fname,
				xmlStrcmp(obj->nodesetval->nodeTab[i]->name, BAD_CAST "attribute") == 0,
				name);
			xmlFree(name);
		}
	}
	xmlXPathFreeObject(obj);

	xmlXPathFreeContext(ctx);

	return err;
}

/* Check all IDREFS values in a document. */
static int check_specific_idrefs(const xmlDocPtr schema, const xmlDocPtr doc, const char *fname, bool attr, const xmlChar *name)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(doc);
	xmlXPathRegisterVariable(ctx, BAD_CAST "name", xmlXPathNewString(name));
	
	if (attr) {
		obj = xmlXPathEvalExpression(BAD_CAST "//@*[name()=$name]", ctx);
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "//*[name()=$name]", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char *ids, *id = NULL;
			xmlNodePtr node = obj->nodesetval->nodeTab[i];

			ids = (char *) xmlNodeGetContent(node);

			while ((id = strtok(id ? NULL : ids, " "))) {
				int e;

				e = check_id_exists(schema, doc, fname, BAD_CAST id);

				if (e) {
					fprintf(stderr, E_BAD_IDREF, (char *) id, fname, xmlGetLineNo(node));
				}

				err += e;
			}

			xmlFree(ids);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return err;
}

static int check_idrefs(const xmlDocPtr schema, const xmlDocPtr doc, const char *fname)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(schema);
	xmlXPathRegisterNs(ctx, BAD_CAST "xs", XML_SCHEMA_URI);

	obj = xmlXPathEvalExpression(BAD_CAST "//xs:attribute[@type='xs:IDREFS']|//xs:element[@type='xs:IDREFS']", ctx);
	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *name = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "name");
			err += check_specific_idrefs(
				schema,
				doc,
				fname,
				xmlStrcmp(obj->nodesetval->nodeTab[i]->name, BAD_CAST "attribute") == 0,
				name);
			xmlFree(name);
		}
	}
	xmlXPathFreeObject(obj);

	xmlXPathFreeContext(ctx);

	return err;
}

static void resize_schema_parsers(void)
{
	if (!(schema_parsers = realloc(schema_parsers, (SCHEMA_PARSERS_MAX *= 2) * sizeof(struct xml_schema_parser)))) {
		fprintf(stderr, E_MAX_SCHEMA_PARSERS, schema_parser_count);
		exit(EXIT_MAX_SCHEMAS);
	}
}

static int validate_file(const char *fname, const char *schema, int list)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	char *url;
	struct xml_schema_parser *parser;
	int err = 0;

	if (!(doc = read_xml_doc(fname))) {
		return 1;
	}

	root = xmlDocGetRootElement(doc);

	if (schema) {
		url = strdup(schema);
	} else {
		url = (char *) xmlGetNsProp(root, BAD_CAST "noNamespaceSchemaLocation", XSI_URI);
	}

	if (!url) {
		if (verbosity > SILENT) {
			fprintf(stderr, ERR_PREFIX "%s has no schema.\n", fname);
		}
		return 1;
	}

	if ((parser = get_schema_parser(url))) {
		xmlFree(url);
	} else {
		if (schema_parser_count == SCHEMA_PARSERS_MAX) {
			resize_schema_parsers();
		}

		parser = add_schema_parser(url);
	}

	/* libxml2's XML Schema validator currently does not check ID and
	 * IDREF/IDREFS relationships, so these have been implemented
	 * separately. */
	err += check_idref(parser->doc, doc, fname);
	err += check_idrefs(parser->doc, doc, fname);

	if (xmlSchemaValidateDoc(parser->valid_ctxt, doc)) {
		++err;
	}

	if (verbosity >= VERBOSE) {
		if (err) {
			fprintf(stderr, FAILED_PREFIX "%s fails to validate against schema %s\n", fname, parser->url);
		} else {
			fprintf(stderr, SUCCESS_PREFIX "%s validates against schema %s\n", fname, parser->url);
		}
	}

	if (list && err) {
		printf("%s\n", fname);
	}

	xmlFreeDoc(doc);

	return err;
}

static int validate_file_list(const char *fname, const char *schema, int list_invalid)
{
	FILE *f;
	char path[PATH_MAX];
	int err;

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, E_BAD_LIST, fname);
			return 0;
		}
	} else {
		f = stdin;
	}

	err = 0;

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		err += validate_file(path, schema, list_invalid);
	}

	if (fname) {
		fclose(f);
	}

	return err;
}

int main(int argc, char *argv[])
{
	int c, i;
	int err = 0;
	int list_invalid = 0;
	int is_list = 0;
	char *schema = NULL;

	const char *sopts = "flqvs:h?";
	struct option lopts[] = {
		{"version"       , no_argument      , 0, 0},
		{"help"          , no_argument      , 0, 'h'},
		{"filenames"     , no_argument      , 0, 'f'},
		{"list"          , no_argument      , 0, 'l'},
		{"quiet"         , no_argument      , 0, 'q'},
		{"verbose"       , no_argument      , 0, 'v'},
		{"schema"        , required_argument, 0, 's'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	schema_parsers = malloc(SCHEMA_PARSERS_MAX * sizeof(struct xml_schema_parser));

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return EXIT_SUCCESS;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'f': list_invalid = 1; break;
			case 'l': is_list = 1; break;
			case 'q': verbosity = SILENT; break;
			case 'v': verbosity = VERBOSE; break;
			case 's': schema = strdup(optarg); break;
			case 'h': 
			case '?': show_help(); return EXIT_SUCCESS;
		}
	}

	LIBXML2_PARSE_INIT

	if (verbosity == SILENT) {
		schema_errfunc = suppress_error;
	}

	xmlSetStructuredErrorFunc(stderr, schema_errfunc);

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (is_list) {
				err += validate_file_list(argv[i], schema, list_invalid);
			} else {
				err += validate_file(argv[i], schema, list_invalid);
			}
		}
	} else if (is_list) {
		err = validate_file_list(NULL, schema, list_invalid);
	} else {
		err = validate_file("-", schema, list_invalid);
	}

	for (i = 0; i < schema_parser_count; ++i) {
		xmlFree(schema_parsers[i].url);
		xmlSchemaFreeValidCtxt(schema_parsers[i].valid_ctxt);
		xmlSchemaFree(schema_parsers[i].schema);
		xmlSchemaFreeParserCtxt(schema_parsers[i].ctxt);
	}

	free(schema_parsers);
	free(schema);
	xmlCleanupParser();

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
