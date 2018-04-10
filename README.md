NAME
====

s1kd-highlight - Syntax highlighting for S1000D verbatimText

SYNOPSIS
========

    s1kd-highlight [options] [<dmodule>...]

DESCRIPTION
===========

The *s1kd-highlight* tool adds syntax highlighting to program listings in a data module. By default this is accomplished by wrapping detected syntax in XSL-FO inline elements with the proper namespace to allow a stylesheet to pass them through to the final FO processor. The actual elements used can be customized, however.

OPTIONS
=======

-f  
Overwrite input data module(s) instead of outputting to stdout.

-h -?  
Show help/usage message.

-l &lt;language&gt;  
Highlight syntax for &lt;language&gt; only.

-s &lt;syntax&gt;  
Use a custom syntax definitions file.

-v &lt;highlight&gt;  
&lt;highlight&gt; is an XML file which associates specific `verbatimStyle` values with specific language names as defined in the syntax file.

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

Class
-----

Represents a type of syntax and how it should be highlighted. This element can also occur within a particular `language` element, in which case it is specific to that language.

*Markup element:* &lt;`class`&gt;

*Attributes:*

-   `id`, the identifier of the class.

*Child elements:*

The element `class` contains one child element of any kind, which any matching syntax will be wrapped in to.

Language
--------

Describes the syntax of a particular language.

*Markup element:* &lt;`language`&gt;

*Attributes:*

-   `name`, the identifier of the language.

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

    <syntax xmlns:fo="http://www.w3.org/1999/XSL/Format">
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
    <language name="c">
    <area start="&quot;" end="&quot;" class="string"/>
    <area start="/*" end="*/" class="comment"/>
    <keyword match="if" class="control"/>
    <keyword match="else" class="control"/>
    <keyword match="int" class="type"/>
    <keyword match="char" class="type"/>
    </language>
    </syntax>

HIGHLIGHT FILE FORMAT
=====================

The following describes the format of the custom highlight file specified with -v.

Highlight
---------

*Markup element:* &lt;`highlight`&gt;

*Attributes:*

-   None

*Child element:*

-   &lt;`verbatimText`&gt;

Verbatim text
-------------

Maps a style of verbatim text to a particular language.

*Markup element:* &lt;`verbatimText`&gt;

*Attributes:*

-   `verbatimStyle`, the style to match.

-   `language`, the language to use for this style.

Example custom highlight file
-----------------------------

    <highlight>
    <verbatimText verbatimStyle="vs51" language="c"/>
    <verbatimText verbatimStyle="vs52" language="pascal"/>
    </highlight>
