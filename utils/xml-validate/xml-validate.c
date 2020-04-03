#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xml-utils.h"

#define PROG_NAME "xml-validate"
#define VERSION "0.3.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define SUCCESS_PREFIX PROG_NAME ": SUCCESS: "
#define FAILED_PREFIX PROG_NAME ": FAILED: "

#define E_BAD_LIST ERR_PREFIX "Could not read list file: %s\n"
#define E_MAX_SCHEMA_PARSERS ERR_PREFIX "Maximum number of schemas reached: %d\n"
#define E_BAD_IDREF ERR_PREFIX "%s (%ld): No matching ID for '%s'.\n"

#define EXIT_MAX_SCHEMAS 2
#define EXIT_MISSING_SCHEMA 3

#define XML_SCHEMA_URI BAD_CAST "http://www.w3.org/2001/XMLSchema"
#define XSI_URI BAD_CAST "http://www.w3.org/2001/XMLSchema-instance"

static enum verbosity_level {SILENT, NORMAL, VERBOSE} verbosity = NORMAL;

enum show_fnames { SHOW_NONE, SHOW_INVALID, SHOW_VALID };

/* Cache schemas to prevent parsing them twice (mainly needed when accessing
 * the schema over a network)
 */
struct xml_schema_parser {
	char *url;
	xmlDocPtr doc;
	xmlSchemaParserCtxtPtr ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr id;
	xmlXPathObjectPtr idref;
	xmlXPathObjectPtr idrefs;
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

/* Find a schema parser by URL. */
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

/* Create a new schema parser from a URL. */
static struct xml_schema_parser *add_schema_parser(char *url)
{
	struct xml_schema_parser *parser;

	xmlDocPtr doc;
	xmlSchemaParserCtxtPtr ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr id, idref, idrefs;

	/* Read the schema document and create a validating context. */
	doc = read_xml_doc(url);
	ctxt = xmlSchemaNewDocParserCtxt(doc);
	schema = xmlSchemaParse(ctxt);
	valid_ctxt = xmlSchemaNewValidCtxt(schema);

	/* Set custom error functions. */
	xmlSchemaSetParserStructuredErrors(ctxt, schema_errfunc, stderr);
	xmlSchemaSetValidStructuredErrors(valid_ctxt, schema_errfunc, stderr);

	/* Locate xs:ID, xs:IDREF and xs:IDREFS types. */
	xpath_ctx = xmlXPathNewContext(doc);
	xmlXPathRegisterNs(xpath_ctx, BAD_CAST "xs", XML_SCHEMA_URI);
	id = xmlXPathEvalExpression(BAD_CAST "//xs:attribute[@type='xs:ID']|//xs:element[@type='xs:ID']", xpath_ctx);
	idref = xmlXPathEvalExpression(BAD_CAST "//xs:attribute[@type='xs:IDREF']|//xs:element[@type='xs:IDREF']", xpath_ctx);
	idrefs = xmlXPathEvalExpression(BAD_CAST "//xs:attribute[@type='xs:IDREFS']|//xs:element[@type='xs:IDREFS']", xpath_ctx);

	/* Initialize the parser. */
	schema_parsers[schema_parser_count].url = url;
	schema_parsers[schema_parser_count].doc = doc;
	schema_parsers[schema_parser_count].ctxt = ctxt;
	schema_parsers[schema_parser_count].schema = schema;
	schema_parsers[schema_parser_count].valid_ctxt = valid_ctxt;
	schema_parsers[schema_parser_count].xpath_ctx = xpath_ctx;
	schema_parsers[schema_parser_count].id = id;
	schema_parsers[schema_parser_count].idref = idref;
	schema_parsers[schema_parser_count].idrefs = idrefs;

	parser = &schema_parsers[schema_parser_count];

	++schema_parser_count;

	return parser;
}

/* Show help/usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-s <path>] [-F|-f] [-lqvh?] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -F, --valid-filenames  List valid files.");
	puts("  -f, --filenames        List invalid files.");
	puts("  -h, -?, --help         Show help/usage message.");
	puts("  -l, --list             Treat input as list of filenames.");
	puts("  -q, --quiet            Silent (no output).");
	puts("  -s, --schema <path>    Validate against the given schema.");
	puts("  -v, --verbose          Verbose output.");
	puts("  --version              Show version information.");
	puts("  <file>                 Any number of XML documents to validate.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
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
static int check_id_exists(const struct xml_schema_parser *parser, const xmlDocPtr doc, const char *fname, const xmlChar *id)
{
	int err = 0;

	if (!xmlXPathNodeSetIsEmpty(parser->id->nodesetval)) {
		int i;

		for (i = 0; i < parser->id->nodesetval->nodeNr; ++i) {
			xmlNodePtr node = parser->id->nodesetval->nodeTab[i];
			xmlChar *name = xmlGetProp(node, BAD_CAST "name");
			err += check_id_exists_in_doc(
				doc,
				fname,
				xmlStrcmp(node->name, BAD_CAST "attribute") == 0,
				name,
				id);
			xmlFree(name);
		}
	}

	return err;
}

/* Check if a specific IDREF value is valid. */
static int check_specific_idref(const struct xml_schema_parser *parser, const xmlDocPtr doc, const char *fname, bool attr, const xmlChar *name)
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

			e = check_id_exists(parser, doc, fname, id);

			if (e) {
				fprintf(stderr, E_BAD_IDREF, fname, xmlGetLineNo(node), (char *) id);
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
static int check_idref(const struct xml_schema_parser *parser, const xmlDocPtr doc, const char *fname)
{
	int err = 0;

	if (!xmlXPathNodeSetIsEmpty(parser->idref->nodesetval)) {
		int i;
		for (i = 0; i < parser->idref->nodesetval->nodeNr; ++i) {
			xmlNodePtr node = parser->idref->nodesetval->nodeTab[i];
			xmlChar *name = xmlGetProp(node, BAD_CAST "name");
			err += check_specific_idref(
				parser,
				doc,
				fname,
				xmlStrcmp(node->name, BAD_CAST "attribute") == 0,
				name);
			xmlFree(name);
		}
	}

	return err;
}

/* Check all IDREFS values in a document. */
static int check_specific_idrefs(const struct xml_schema_parser *parser, const xmlDocPtr doc, const char *fname, bool attr, const xmlChar *name)
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

				e = check_id_exists(parser, doc, fname, BAD_CAST id);

				if (e) {
					fprintf(stderr, E_BAD_IDREF, fname, xmlGetLineNo(node), (char *) id);
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

static int check_idrefs(const struct xml_schema_parser *parser, const xmlDocPtr doc, const char *fname)
{
	int err = 0;

	if (!xmlXPathNodeSetIsEmpty(parser->idrefs->nodesetval)) {
		int i;
		for (i = 0; i < parser->idrefs->nodesetval->nodeNr; ++i) {
			xmlNodePtr node = parser->idrefs->nodesetval->nodeTab[i];
			xmlChar *name = xmlGetProp(node, BAD_CAST "name");
			err += check_specific_idrefs(
				parser,
				doc,
				fname,
				xmlStrcmp(node->name, BAD_CAST "attribute") == 0,
				name);
			xmlFree(name);
		}
	}

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
	err += check_idref(parser, doc, fname);
	err += check_idrefs(parser, doc, fname);

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

static int validate_file_list(const char *fname, const char *schema, enum show_fnames show_fnames)
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
		err += validate_file(path, schema, show_fnames);
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
	enum show_fnames show_fnames = SHOW_NONE;
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
			case 'F': show_fnames = SHOW_VALID; break;
			case 'f': show_fnames = SHOW_INVALID; break;
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
				err += validate_file_list(argv[i], schema, show_fnames);
			} else {
				err += validate_file(argv[i], schema, show_fnames);
			}
		}
	} else if (is_list) {
		err = validate_file_list(NULL, schema, show_fnames);
	} else {
		err = validate_file("-", schema, show_fnames);
	}

	for (i = 0; i < schema_parser_count; ++i) {
		xmlFree(schema_parsers[i].url);
		xmlSchemaFreeValidCtxt(schema_parsers[i].valid_ctxt);
		xmlSchemaFree(schema_parsers[i].schema);
		xmlSchemaFreeParserCtxt(schema_parsers[i].ctxt);
		xmlXPathFreeObject(schema_parsers[i].id);
		xmlXPathFreeObject(schema_parsers[i].idref);
		xmlXPathFreeObject(schema_parsers[i].idrefs);
		xmlXPathFreeContext(schema_parsers[i].xpath_ctx);
		xmlFreeDoc(schema_parsers[i].doc);
	}

	free(schema_parsers);
	free(schema);
	xmlCleanupParser();

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
