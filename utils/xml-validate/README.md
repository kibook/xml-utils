NAME
====

xml-validate - Validate XML documents against their schemas

SYNOPSIS
========

    xml-validate [-s <path>] [-F|-f] [-loqvh?] [<file>...]

DESCRIPTION
===========

The *xml-validate* utility validates XML documents, checking whether
they are valid XML files and if they are valid against a referenced or
specified schema.

OPTIONS
=======

-F, --valid-filenames  
List valid files.

-f, --filenames  
List invalid files.

-h, -?, --help  
Show help/usage message.

-l, --list  
Treat input as a list of XML documents names to validate, rather than an
XML document itself.

-q, --quiet  
Quiet mode. The utility will not output anything to stdout or stderr.
Success/failure will only be indicated through the exit status.

-s, --schema &lt;path&gt;  
Validate the XML documents against the specified schema, rather than the
one that they reference.

-v, --verbose  
Verbose mode. Success/failure will be explicitly reported on top of any
errors.

--version  
Show version information.

&lt;file&gt;...  
Any number of XML documents to validate. If none are specified, input is
read from stdin.

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

EXIT STATUS
===========

0  
No errors.

1  
Some XML documents are not well-formed or valid.

2  
The number of schemas cached exceeded the available memory.

3  
A specified schema could not be read.

EXAMPLE
=======

    $ xml-validate doc.xml
