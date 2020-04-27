NAME
====

xml-query - Query XML documents with XPath

SYNOPSIS
========

    xml-query [-d <delim>] [-N <ns>=<URL>] [-s <value>] [-w <xpath>]
              [-x <xpath>] [-0fnth?] [<file>...]

DESCRIPTION
===========

The *xml-query* utility queries XML documents using XPath and prints the
results. It can also modify nodes matched by an XPath expression using
the -s (--set) option.

OPTIONS
=======

-0, --null  
Output the results of the query in a null-delimited list.

-d, --delimiter &lt;delim&gt;  
Output the results of the query in a list with a custom delimiter.

-f, --overwrite  
When modifying documents with -s (--set), overwrite the input files
rather than printing to stdout.

-h, -?, --help  
Show usage message.

-N, --namespace &lt;ns&gt;=&lt;URL&gt;  
Register an XML namespace handle for &lt;URL&gt;, which can then be used
when specifying element/attribute names in XPath expressions. Multiple
namespaces can be registered by specifiny this option multiple times.

-n, --filename  
Print the filename of each document.

-s, --set &lt;value&gt;  
Set the contents of the nodes matched by the previous XPath expression
(-x) to the given string value.

-w, --where &lt;xpath&gt;  
Skip documents where &lt;xpath&gt; does not evaluate to true. Multiple
conditions can be set by specifying this option multiple times.

-x, --xpath &lt;xpath&gt;  
Print the nodes matched by the given XPath expression. Multiple XPath
expressions can be matched by specifying this option multiple times.

--version  
Show version information.

&lt;file&gt;  
XML file(s) to query or modify.

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

EXAMPLES
========

Input XML document (`test.xml`):

    <root xmlns:ex="http://www.example.com">
    <a>1</a>
    <b>foobar</b>
    <ex:c>barfoo</ex:c>
    </root>

Example queries:

    $ xml-query -x //a test.xml
    1

    $ xml-query -x //b test.xml
    foobar

    $ xml-query -N ex=http://www.example.com -x //ex:c test.xml
    barfoo
