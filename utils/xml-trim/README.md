NAME
====

xml-trim - Trim whitespace in XML elements

SYNOPSIS
========

    xml-trim [-e <elem> ...] [-N <ns=URL> ...] [-fnh?] <src>...

DESCRIPTION
===========

The *xml-trim* utility trims whitespace around the text contents of
specified elements.

Whitespace is trimmed according to the following rules:

1.  Whitespace characters at the beginning of the first text node child
    of the specified elements are removed.

2.  Whitespace characters at the end of the last text node child of the
    specified elements are removed.

3.  If the -n option is given, sequences of whitespace characters in all
    text node children of the specified elements are converted to a
    single space.

OPTIONS
=======

-e, --element &lt;elem&gt;  
Elements to trim space on. May include a namespace prefix if the
namespace was registered with -N. &lt;elem&gt; may be either a simple
element name (e.g., "para") which matches all elements with the same
name at any position, or an XPath expression (e.g., "//section/para")
for finer control.

-f, --overwrite  
Overwrite input XML files.

-h, -?, --help  
Show usage message.

-N, --namespace &lt;ns=URL&gt;  
Registers an XML namespace handle for URL, which can then be used when
specifying element names as options. Multiple namespaces can be
registered by specifying this option multiple times.

-n, --normalize  
Normalize space in the specified elements in addition to trimming
whitespace.

--version  
Show version information.

&lt;src&gt;  
The source XML file(s) containing the elements to trim.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--xinclude  
Do XInclude processing.

EXAMPLES
========

Without namespace
-----------------

    <section>
      <para>
        Hello world.
      </para>
    </section>

    $ xml-trim -e para example.xml

    <section>
      <para>Hello world.</para>
    </section>

With namespace
--------------

    <d:section>
      <d:para>
        Hello world.
      </d:para>
    </d:section>

    $ xml-trim -N d=http://docbook.org/ns/docbook -e d:para example.xml

    <d:section>
      <d:para>Hello world.</d:para>
    </d:section>

Normalizing space
-----------------

    <section>
      <para>
        This is a <emphasis>long</emphasis>
        paragraph with both extra whitespace
        before and after the text, and line
        breaks entered by the author to wrap
        the text on a certain column.
      </para>
    </section>

    $ xml-trim -n -e para example.xml

    <section>
      <para>This is a <emphasis>long</emphasis> paragraph with both extra whitespace before
    and after the text, and line breaks entered by the author to wrap the
    text on a certain column.</para>
    </section>
