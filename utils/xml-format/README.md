NAME
====

xml-format - Format XML files

SYNOPSIS
========

    $ xml-format [-fOwh?] [-i <str>] [-o <path>] [<file>...]

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

-f  
Overwrite input XML files.

-h -?  
Show usage message.

-i &lt;str&gt;  
Use &lt;str&gt; to indent each level of the XML tree. The default is two
spaces.

-O  
Omit the XML declaration from the formatted XML output.

-o &lt;path&gt;  
Output formatted XML to &lt;path&gt; instead of stdout.

-w  
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

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

EXAMPLE
=======

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
