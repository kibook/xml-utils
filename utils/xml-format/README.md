NAME
====

xml-format - Format XML files

SYNOPSIS
========

    xml-format [-cfIOwh?] [-i <str>] [-o <path>] [<file>...]

DESCRIPTION
===========

The *xml-format* utility pretty prints an XML document by removing
ignorable blank nodes and indenting the XML tree.

A text node is considered ignorable if:

-   its parent does not set or inherit xml:space as "preserve"

-   its contents are entirely whitespace

-   all its sibling text nodes are also ignorable

-   all its ancestors contained only ignorable text nodes

OPTIONS
=======

-c, --compact  
Output in compact form without indenting.

-f, --overwrite  
Overwrite input XML files.

-h, -?, --help  
Show usage message.

-I, --indent-all  
Indent nodes within non-blank nodes. Normally, all whitespace within
non-blank nodes and their descendants is treated as significant.

-i, --indent &lt;str&gt;  
Use &lt;str&gt; to indent each level of the XML tree. The default is two
spaces.

-O, --omit-decl  
Omit the XML declaration from the formatted XML output.

-o, --out &lt;path&gt;  
Output formatted XML to &lt;path&gt; instead of stdout.

-w, --empty  
Treat elements containing only whitespace as empty.

--version  
Show version information.

&lt;file&gt;...  
XML file(s) to format. If none are specified, the utility will read from
stdin.

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

EXAMPLE
=======

Standard formatting:

    $ cat doc.xml
    <section>     <title>Example</title>

    <para><emphasis>A</emphasis> <emphasis>B</emphasis> C</para>

    <empty>    </empty>

    <p><b>A</b> <b>B</b> <b>C</b></p>
    <p xml:space="preserve"><b>A</b> <b>B</b> <b>C</b></p>

    <para>
        See the following list:
        <randomList>
          <listItem><para>A</para></listItem>
          <listItem><para>B</para></listItem>
          <listItem><para>C</para></listItem>
        </randomList>
    </para>

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
      <para>
        See the following list:
        <randomList>
          <listItem><para>A</para></listItem>
          <listItem><para>B</para></listItem>
          <listItem><para>C</para></listItem>
        </randomList>
      </para>
    </section>

Using the -I (--indent-all) option:

    $ cat test1.xml
    <para>
      See the following list:
      <randomList>
        <listItem><para>A</para></listItem>
        <listItem><para>B</para></listItem>
        <listItem><para>C</para></listitem>
      </randomList>
    </para>

    $ xml-format -I test1.xml
    <para>
      See the following list:
      <randomList>
        <listItem>
          <para>A</para>
        </listItem>
        <listItem>
          <para>B</para>
        </listItem>
        <listItem>
          <para>C</para>
        </listItem>
      </randomList>
    </para>
