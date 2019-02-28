NAME
====

xml-format - Format XML files

SYNOPSIS
========

    $ xml-format [-fh?] [-i <str>] [-o &lt;path&gt;] [<file>...]

DESCRIPTION
===========

The *xml-format* utility pretty prints an XML document by removing
ignorable blank nodes and indenting the XML tree.

A text node is considered ignorable if:

-   its parent does not set or inherit xml:space as "preserve"

-   its contents are entirely whitespace

-   all its sibling text nodes are also ignorable

-   it has at least one sibling node of any kind

OPTIONS
=======

-f  
Overwrite input XML files.

-h -?  
Show usage message.

-i &lt;str&gt;  
Use &lt;str&gt; to indent each level of the XML tree. The default is two
spaces.

-o &lt;path&gt;  
Output formatted XML to &lt;path&gt; instead of stdout.

--version  
Show version information.

&lt;file&gt;...  
XML file(s) to format. If none are specified, the utility will read from
stdin.

EXAMPLE
=======

    $ cat doc.xml
    <section>     <title>Example</title>

    <para><emphasis>A</emphasis> <emphasis>B</emphasis> C</para>

    <empty>    </empty>

    <p><b>A</b> <b>B</b> <b>C</b></p>
    <p xml:space="preserve"><b>A</b> <b>B</b> <b>C</b></p>

    </section>

    $ xml-format test.xml
    <section>
      <title>Example</title>
      <para><emphasis>A</emphasis> <emphasis>B</emphasis> C</para>
      <empty>    </empty>
      <p>
        <b>A</b>
        <b>B</b>
        <b>C</b>
      </p>
      <p xml:space="preserve"><b>A</b> <b>B</b> <b>C</b></p>
    </section>
