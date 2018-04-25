NAME
====

xml-diff - Indicate changes between versions of an XML document

SYNOPSIS
========

    xml-diff [options] <OLD> <NEW>

DESCRIPTION
===========

The *xml-diff* utility marks changes on elements between two versions of an XML document. The attribute diff:change is placed on changed elements, with a value indicating the presumed type of change, "add" for new elements and "modify" for modified elements. If all child elements of an element are changed, only the parent element will receive the change mark.

The namespace for the diff:change elements is:

`http://khzae.net/1/xml/xml-utils/xml-diff`

OPTIONS
=======

-h -?  
Show help/usage message.
