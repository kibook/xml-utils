NAME
====

xml-trimspace - Trim whitespace in XML elements

SYNOPSIS
========

    xml-trimspace [-N <ns=URL>] [-nh?] <elem>... < <src> > <dst>

DESCRIPTION
===========

The *xml-trimspace* utility trims whitespace around the text contents of
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

-h -?  
Show usage message.

-N &lt;ns=URL&gt;  
Registers an XML namespace handle for URL, which can then be used when
specifying element names as options. Multiple namespaces can be
registered by specifying this option multiple times.

-n  
Normalize space in the specified elements in addition to trimming
whitespace.

--version  
Show version information.

&lt;elem&gt;...  
Elements to trim space on. May include a namespace prefix if the
namespace was registered with -N. &lt;elem&gt; may be either a simple
element name (e.g., "para") which matches all elements with the same
name at any position, or an XPath expression (e.g., "//section/para")
for finer control.

&lt;src&gt;  
The source XML file containing the elements to trim.

&lt;dst&gt;  
The output file.

EXAMPLES
========

Without namespace
-----------------

    <section>
      <para>
        Hello world.
      </para>
    </section>

    $ xml-trimspace para < example.xml

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

    $ xml-trimspace -N d=http://docbook.org/ns/docbook d:para < example.xml

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

    $ xml-trimspace -n para < example.xml

    <section>
      <para>This is a <emphasis>long</emphasis> paragraph with both extra whitespace before
    and after the text, and line breaks entered by the author to wrap the
    text on a certain column.</para>
    </section>
