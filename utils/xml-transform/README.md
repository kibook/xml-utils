NAME
====

xml-transform - Apply XSL transformations to XML documents

SYNOPSIS
========

    xml-transform [-s <stylesheet> [-p <name>=<value> ...] ...]
                  [-o <file>] [-cdfilqvh?] [<file> ...]

DESCRIPTION
===========

Applies one or more XSLT stylesheets to one or more XML documents.

OPTIONS
=======

-c, --combine  
Transform the input files as a single, combined XML document, rather
than as individual documents.

-d, --preserve-dtd  
Preserve the DTD of the original document when transforming.

-f, --overwrite  
Overwrite the specified files instead of writing to stdout.

-h, -?, --help  
Show usage message.

-i, --identity  
Includes an "identity" template in to each specified stylesheet.

-l, --list  
Treat input (stdin or arguments) as lists of files to transform, rather
than files themselves.

-o, --out &lt;file&gt;  
Output to &lt;file&gt; instead of stdout. This option only makes sense
when the input is a single XML document.

-p, --param &lt;name&gt;=&lt;value&gt;  
Pass a parameter to the last specified stylesheet.

-q, --quiet  
Quiet mode. Errors are not printed.

-s, --stylesheet &lt;stylesheet&gt;  
An XSLT stylesheet file to apply to each XML document. Multiple
stylesheets can be specified by supplying this argument multiple times.
The stylesheets will be applied in the order they are listed.

-v, --verbose  
Verbose output.

--version  
Show version information.

&lt;file&gt; ...  
Any number of XML documents to apply all specified stylesheets to.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--huge  
Remove any internal arbitrary parser limits.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--xinclude  
Do XInclude processing.

Identity template
-----------------

The -i option includes an "identity" template in to each stylesheet
specified with the -s option. The template is equivalent to this XSL:

    <xsl:template match="@*|node()">
    <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
    </xsl:template>

This means that any attributes or nodes which are not matched by a more
specific template in the user-specified stylesheet are copied.

EXAMPLE
=======

    $ xml-transform -s <XSL> <doc1> <doc2> ...
