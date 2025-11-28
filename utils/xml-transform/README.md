# NAME

xml-transform - Apply XSL transformations to XML documents

# SYNOPSIS

    xml-transform [-s <stylesheet> [(-P|-p) <name>=<value> ...] ...]
                  [(-E|-e) <xpath> ...]
                  [-o <file>] [-cdfilnqSvh?] [<file> ...]

# DESCRIPTION

Applies one or more XSLT stylesheets to one or more XML documents.

# OPTIONS

  - \-c, --combine  
    Transform the input files as a single, combined XML document, rather
    than as individual documents.

  - \-d, --preserve-dtd  
    Preserve the DTD of the original document when transforming.

  - \-E, --extract-text \<xpath\>  
    Extracts the text value of the first node matching the given XPath
    expression.
    
    Although the extracted text will be passed to any subsequent
    -s/-E/-e options like with -e (--extract), it is more common for
    this option to be used alone or at the end of a pipeline.

  - \-e, --extract \<xpath\>  
    Extracts the node(s) matching the given XPath expression.
    
    The document consisting of the extracted node(s) is passed to any
    subsequent -s (--stylesheet), -E (--extract-text) or -e options.

  - \-f, --overwrite  
    Overwrite the specified files instead of writing to stdout.

  - \-h, -?, --help  
    Show usage message.

  - \-i, --identity  
    Includes an "identity" template in to each specified stylesheet.

  - \-l, --list  
    Treat input (stdin or arguments) as lists of files to transform,
    rather than files themselves.

  - \-n, --null-input  
    Use a minimal XML document as input instead of any files or stdin.
    This allows for executing a transformation script without needing a
    "real" XML document as input.
    
    The "null" document consists of single element named "a".

  - \-o, --out \<file\>  
    Output to \<file\> instead of stdout. This option only makes sense
    when the input is a single XML document.

  - \-P, --string-param \<name\>=\<value\>  
    Pass a string parameter to the last specified stylesheet. If
    specified before any stylesheets, the parameter will be passed to
    all stylesheets. This is essentially equivalent to `-p
    "name='value'"`, though it will auto-detect if the value string
    contains single or double quotes and use the opposite. Strings with
    both single and double quotes cannot be used.

  - \-p, --param \<name\>=\<value\>  
    Pass a parameter to the last specified stylesheet. If specified
    before any stylesheets, the parameter will be passed to all
    stylesheets.

  - \-q, --quiet  
    Quiet mode. Errors are not printed.

  - \-S, --xml-stylesheets  
    Apply stylesheets that are associated to each XML document with the
    `xml-stylesheet` processing instruction. Associated stylesheets are
    applied before any user-specified stylesheets, in the order in which
    they occur within the document.

  - \-s, --stylesheet \<stylesheet\>  
    An XSLT stylesheet file to apply to each XML document. Multiple
    stylesheets can be specified by supplying this argument multiple
    times. The stylesheets will be applied in the order they are listed.

  - \-v, --verbose  
    Verbose output.

  - \--version  
    Show version information.

  - \<file\> ...  
    Any number of XML documents to apply all specified stylesheets to.

In addition, the following options allow configuration of the XML
parser:

  - \--dtdload  
    Load the external DTD.

  - \--html  
    Parse input files as HTML.

  - \--huge  
    Remove any internal arbitrary parser limits.

  - \--net  
    Allow network access to load external DTD and entities.

  - \--noent  
    Resolve entities.

  - \--xinclude  
    Do XInclude processing.

  - \--xml-catalog \<file\>  
    Use an XML catalog when resolving entities. Multiple catalogs may be
    loaded by specifying this option multiple times.

## Identity template

The -i option includes an "identity" template in to each stylesheet
specified with the -s option. The template is equivalent to this XSL:

    <xsl:template match="@*|node()">
    <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
    </xsl:template>

This means that any attributes or nodes which are not matched by a more
specific template in the user-specified stylesheet are copied.

# EXAMPLES

Transform a document using a single stylesheet and output the result:

    $ xml-transform -s style1.xsl doc1.xml

Transform a document using multiple stylesheets and output the result:

    $ xml-transform -s style1.xsl -s style2.xsl doc1.xml

Transform multiple documents using a stylesheet and overwrite with the
result:

    $ xml-transform -s style1.xsl -f doc1.xml doc2.xml doc3.xml

Extract node(s) from a document:

    $ xml-transform -e /foo/bar doc1.xml

Extract a node from a document and then apply a stylesheet to it:

    $ xml-transform -e /foo/bar -s style1.xsl doc1.xsl

Extract the text value of a single node from a document:

    $ xml-transform -E /foo/bar doc1.xml

Get the number of a certain node anywhere in a document:

    $ xml-transform -e 'count(//bar)' doc1.xml
