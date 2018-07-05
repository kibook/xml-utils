General
=======

The *xml-trimspace* utility trims whitespace around the text contents of specified elements.

Usage
=====

    xml-trimspace [-Nh?] [-n <ns=URL>] <elem>... < <src> > <dst>

Options
=======

-h -?  
Show usage message.

-n &lt;ns=URL&gt;  
Registers an XML namespace handle for URL, which can then be used when specifying element names as options. Multiple namespaces can be registered by specifying this option multiple times.

-N  
Normalize space in the specified elements in addition to trimming whitespace.

--version  
Show version information.

&lt;elem&gt;  
Elements to trim space on. May include a namespace prefix if the namespace was registered with -n.

&lt;src&gt;  
The source XML file containing the elements to trim.

&lt;dst&gt;  
The output file.

Example
=======

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

    $ xml-trimspace -n d=http://docbook.org/ns/docbook d:para < example.xml

    <d:section>
      <d:para>Hello world.</d:para>
    </d:section>

Normalizing space
-----------------

    <section>
      <para>
        This is a long paragraph with both
        extra whitespace before and after
        the text, and line breaks entered
        by the author to wrap the text on
        a certain column.
      </para>

    $ xml-trimspace -N para < example.xml

    <section>
      <para>This is a long paragraph with both extra whitespace before
    and after the text, and line breaks entered by the author to wrap the
    text on a certain column.</para>
    </section>
