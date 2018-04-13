NAME
====

xml-highlight - Highlight syntax in XML documents

SYNOPSIS
========

    xml-highlight [options] [<document>...]

DESCRIPTION
===========

The *xml-highlight* tool adds syntax highlighting to program listings in an XML document. By default this is accomplished by wrapping detected syntax in XSL-FO inline elements with the proper namespace to allow a stylesheet to pass them through to the final FO processor. The actual elements used can be customized, however.

To enable highlighting on text in an element, include the processing instruction `<?language ...?>` in the element, where `...` is the name of the language.

OPTIONS
=======

-c &lt;classes&gt;  
Use a custom class definitions file.

-f  
Overwrite input data module(s) instead of outputting to stdout.

-h -?  
Show help/usage message.

-s &lt;syntax&gt;  
Use a custom syntax definitions file.

CLASS FILE FORMAT
=================

The following describes the format of the custom class file specified with -c.

Classes
-------

*Markup element:* &lt;`classes`&gt;

*Attributes:*

-   None

*Child elements:*

-   &lt;`class`&gt;

Class
-----

Represents a type of syntax and how it should be highlighted. This element can also occur within the `syntax` element or within a particular `language` element, in which case it is specific to that language.

*Markup element:* &lt;`class`&gt;

*Attributes:*

-   `id`, the identifier of the class.

*Child elements:*

The element `class` contains one child element of any kind, which any matching syntax will be wrapped in to.

Example custom classes file
---------------------------

    <classes xmlns:fo="http://www.w3.org/1999/XSL/Format">
    <class id="type">
    <fo:inline color="green"/>
    </class>
    <class id="control">
    <fo:inline color="blue"/>
    </class>
    <class id="string">
    <fo:inline color="red"/>
    </class>
    <class id="comment">
    <fo:inline color="pink"/>
    </class>
    </classes>

SYNTAX FILE FORMAT
==================

The following describes the format of the custom syntax file specified with -s.

Syntax
------

*Markup element:* &lt;`syntax`&gt;

*Attributes:*

-   None

*Child elements:*

-   `class`

-   `language`

Language
--------

Describes the syntax of a particular language.

*Markup element:* &lt;`language`&gt;

*Attributes:*

-   `name`, the identifier of the language.

-   `caseInsensitive`, indicates whether keywords are case-insensitive in this language, with one of the following values:

    -   `"no"` - Keywords are case-sensitive (default)

    -   `"yes"` - Keywords are case-insensitive

*Child elements:*

-   &lt;`class`&gt;

-   &lt;`area`&gt;

-   &lt;`keyword`&gt;

Area
----

Matches a section of delimited text, such as strings, comments, etc.

*Markup element:* &lt;`area`&gt;

*Attributes:*

-   `start`, the opening delimiter.

-   `end`, the closing delimiter.

-   `class`, reference to the `class` element to use for this area.

*Child elements:*

If attribute `class` is not used, this element can contain one element of any kind, in which the text matching the area will be wrapped.

Keyword
-------

Matches a particular keyword.

*Markup element:* &lt;`keyword`&gt;

*Attributes:*

-   `match`, the keyword to match.

-   `class`, reference to the `class` element to use for this keyword.

*Child elements:*

If attribute `class` is not used, this element can contain one element of any kind, in which the text matching the keyword will be wrapped.

Example custom syntax file
--------------------------

    <syntax>
    <language name="c">
    <area start="&quot;" end="&quot;" class="string"/>
    <area start="/*" end="*/" class="comment"/>
    <keyword match="if" class="control"/>
    <keyword match="else" class="control"/>
    <keyword match="int" class="type"/>
    <keyword match="char" class="type"/>
    </language>
    </syntax>

BUILT-IN LANGUAGES
==================

The following is a list of language syntaxes currently built-in to the tool:

-   c

-   csharp

-   go

-   java

-   javascript

-   pascal

-   python

-   ruby

-   rust

-   sh

-   sql

-   xml

-   xsl
