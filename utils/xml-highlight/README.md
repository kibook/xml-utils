# NAME

xml-highlight - Highlight syntax in XML documents

# SYNOPSIS

    xml-highlight [options] [<document>...]

# DESCRIPTION

The *xml-highlight* tool adds syntax highlighting to program listings in
an XML document. By default this is accomplished by wrapping detected
syntax in XSL-FO inline elements with the proper namespace to allow a
stylesheet to pass them through to the final FO processor. The actual
elements used can be customized, however.

To enable highlighting on text in an element, include the processing
instruction `<?language ...?>` in the element, where `...` is the name
of the language.

# OPTIONS

  - \-c, --classes \<classes\>  
    Use a custom class definitions file.

  - \-f, --overwrite  
    Overwrite input XML file(s) instead of outputting to stdout.

  - \-h, -?, --help  
    Show help/usage message.

  - \-s, --syntax \<syntax\>  
    Use a custom syntax definitions file.

  - \--version  
    Show version information.

In addition, the following options allow configuration of the XML
parser:

  - \--dtdload  
    Load the external DTD.

  - \--huge  
    Remove any internal arbitrary parser limits.

  - \--net  
    Allow network access to load external DTD and entities.

  - \--noent  
    Resolve entities.

  - \--xinclude  
    Do XInclude processing.

  - \--xml-catalog \<file\>  
    Use an XML catalog when resolving entities. Multiple catalogs may be
    loaded by specifying this option multiple times.

# CLASS FILE FORMAT

The following describes the format of the custom class file specified
with -c.

## Classes

*Markup element:* \<`classes`\>

*Attributes:*

  - None

*Child elements:*

  - \<`class`\>

## Class

Represents a type of syntax and how it should be highlighted. This
element can also occur within the `syntax` element or within a
particular `language` element, in which case it is specific to that
language.

*Markup element:* \<`class`\>

*Attributes:*

  - `id`, the identifier of the class.

*Child elements:*

The element `class` contains one child element of any kind, which any
matching syntax will be wrapped in to.

## Example custom classes file

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

# SYNTAX FILE FORMAT

The following describes the format of the custom syntax file specified
with -s.

## Syntax

*Markup element:* \<`syntax`\>

*Attributes:*

  - None

*Child elements:*

  - `class`

  - `language`

## Language

Describes the syntax of a particular language.

*Markup element:* \<`language`\>

*Attributes:*

  - `name`, the identifier of the language.

  - `caseInsensitive`, indicates whether keywords are case-insensitive
    in this language, with one of the following values:
    
      - `"no"` - Keywords are case-sensitive (default)
    
      - `"yes"` - Keywords are case-insensitive

*Child elements:*

  - \<`class`\>

  - \<`area`\>

  - \<`keyword`\>

## Area

Matches a section of delimited text, such as strings, comments, etc.

*Markup element:* \<`area`\>

*Attributes:*

  - `start`, the opening delimiter.

  - `end`, the closing delimiter.

  - `class`, reference to the `class` element to use for this area.

*Child elements:*

If attribute `class` is not used, this element can contain one element
of any kind, in which the text matching the area will be wrapped.

## Keyword

Matches a particular keyword.

*Markup element:* \<`keyword`\>

*Attributes:*

  - `match`, the keyword to match.

  - `class`, reference to the `class` element to use for this keyword.

*Child elements:*

If attribute `class` is not used, this element can contain one element
of any kind, in which the text matching the keyword will be wrapped.

## Example custom syntax file

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

# BUILT-IN LANGUAGES

The following is a list of language syntaxes currently built-in to the
tool:

  - c

  - csharp

  - go

  - java

  - javascript

  - pascal

  - python

  - ruby

  - rust

  - sh

  - sql

  - xml

  - xsl
