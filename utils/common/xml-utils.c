#include "xml-utils.h"

/* Default global XML parsing options.
 *
 * XML_PARSE_NOERROR / XML_PARSE_NOWARNING
 *   Suppress the error logging built-in to the parser. The tools will handle
 *   reporting errors.
 *
 * XML_PARSE_NONET
 *   Disable network access as a safety precaution.
 */
int DEFAULT_PARSE_OPTS = XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET;
