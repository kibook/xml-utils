NAME
====

xml-format - Format XML files

SYNOPSIS
========

    $ xml-format [-fh?] [-i <str>] [<file>...]

DESCRIPTION
===========

The *xml-format* utility pretty prints an XML document by removing
ignorable blank nodes and indenting the XML tree.

OPTIONS
=======

-f  
Overwrite input XML files.

-h -?  
Show usage message.

-i &lt;str&gt;  
Use &lt;str&gt; to indent each level of the XML tree. The default is two
spaces.

--version  
Show version information.

&lt;file&gt;...  
XML file(s) to format. If none are specified, the utility will read from
stdin.

EXAMPLE
=======

    $ cat doc.xml
    <section>

      <title>Example</title>

      <para><emphasis>A</emphasis> <emphasis>B</emphasis> C</para>

    </section>

    $ xml-format test.xml
    <section>
      <title>Example</title>
      <para><emphasis>A</emphasis> <emphasis>B</emphasis> C</para>
    </section>
