# qdpeg
Yet another parsing expression grammar

# Introduction
qdpeg is a simple (Quick and Dirty) Parsing Expression Grammer parsing library.

If you need code that helps you convert text to a native C++ type, this library might be for you. qdpeg contains:
  - built-in parsers for stuff such as characters, integers and whitespace
  - parser-generators which combine individual parsers into new, more powerful ones
  - adaptors that manipulate the result of a parser to eg. perform further checks of the parsed value and/or transform the type of the parsed value.
  - some utilities that allow you to query about your code.

# Requirements
qdpeg requires a C++17 compiler and a version of boost containing Mp11. Tested with boost 1.67, recent clang and gcc compilers and MSVC 15.9.7. It is rather fragile on MSVC, but should work if you take care not to have to complex expressions.
# Dependencies
The library is header-only and only requires Peter Dimovs excellent mp11 template library (which is part of boost).
# Design goals
 - Debuggability. If you have a problem with your code, it should be reasonably easy to solve using a debugger. 
 - Have a syntax that faciliates automatic code generation. One current usage of qdpeg is as a part of a module for automatic conversion of native data to or from text.
 - Have a value-centric view of the parsing. All our parsers return a value on success (could be an integer, a syntax tree or the special whitespace value). It is possible to validate data as it is parsed.
 - No surprises: A parser is a regular type. You can copy and move them as you wish.

Performance is not a goal, so do test performance if you intend to parse e.g. multi-megabyte xml or json files. That said, qdpeg is inherently easy to optimize giving good performance in the scenarios where it is currently used. There are no virtual functions, type-erasure or other constructs that might prevent an optimizer from doing its job.

# Anatomy of a parser
In qdpeg, a parser is a callable object such as a function or lambda, that when called with a begin and an end to input either succeeds and returns the parsed object and a position in the input, marking where new input can be consumed or an error, containing a position where parsing failed and a generic error-message.
If a parser parses data of a given type T, the parser returns a Parse_result<T>. Here we name parsers according to what they return so a string parser is a parser that returns Parse_result<std::string>. A whitespace parser is called a skipper. Formally it returns Parse_result<Nothing>, where Nothing is a special, empty type.

You can write your own parser from scratch. This simple parser, that parses the single character '1' is a perfectly valid qdpeg parser:
```c++
Parse_result<char> parse_literal_1(Iter b, Iter e)
{
	if (b == e) return { b, unexpected_eof };
    auto res = *b;
    if (res != '1') return { b, unexpected_char };
    return { ++b, res };
}
```
The return value of a parser is convertible to bool. It converts to true if and only if the parsing succeeds. If the parsing succeeds, use value() to get the result of the parse, if it fails use error() to get the error code.
In qdpeg, white space parsers must be explicitly specified. They are recognised in that their return type is qdpeg::Skipper, which is an alias for Parse_result<qdpeg::Nothing>.

# Examples
In the examples following, you should expect a using namespace qdpeg directive and that appropriate files have been included.
Also expect a function parse that takes a string and a parser and returns a result and a string. parse parses the string using the supplied parser and returns the result of the parse and the part of the input that was not parsed.
The expected output is written as a comment after the call, perhaps with an explanation in parenthesis as here:
```c++
parse("- 1734",int_parser<int>); // failure/" 1734" (space not allowed)
```
So parsing "- 1734" with the int parser results in a failure where " 1734" was not parsed. The comment indicates that a space can not be part of an integer.
