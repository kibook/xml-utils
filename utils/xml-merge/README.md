NAME
====

xml-merge - Merge XML files on a common element

SYNOPSIS
========

    xml-merge [-fh?] <dst> <src>

DESCRIPTION
===========

The *xml-merge* utility merges two XML files together, based on a common
element.

OPTIONS
=======

-f  
Overwrite the &lt;dst&gt; file with the merged result, instead of
writing to stdout.

-h -?  
Show usage message.

--version  
Show version information.

&lt;dst&gt;  
The XML file which &lt;src&gt; will be merged in to.

&lt;src&gt;  
The XML file which will me merged in to &lt;dst&gt;. The first occurence
of an element in &lt;dst&gt; which matches the root element of this file
is where the merge will occur.

EXAMPLE
=======

Given the following two XML files:

*`file1.xml`:*

    <root>
    <metadata>...</metadata>
    <content/>
    </root>

*`file2.xml`:*

    <content>
    <text>Hello world.</text>
    </content>

the xml-merge utility will merge them on the element `content`:

    $ xml-merge file1.xml file2.xml > merged.xml

to produce the merged file:

*`merged.xml`:*

    <root>
    <metadata>...</metadata>
    <content>
    <text>Hello world.</text>
    </content>
    </root>
