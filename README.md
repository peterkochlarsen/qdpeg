# qdpeg
Yet another parsing expression grammar
# Introduction
qdpeg is a simple (Quick and Dirty) [Parsing Expression Grammer parsing](https://en.wikipedia.org/wiki/Parsing_expression_grammar) library.

If you need code that helps you convert text to a native C++ type, this library might be for you. qdpeg contains:
  - [built-in parsers](##built-in-parsers) for stuff such as characters, integers and whitespace
  - [parser-generators](##parser-generators) which combine individual parsers into new, more powerful ones
  - adaptors that manipulate the result of a parser to eg. perform further checks of the parsed value and/or transform the type of the parsed value.
## Requirements
qdpeg is header-only, requires C++17 and a version of boost containing [mp11](https://github.com/boostorg/mp11). Tested with boost 1.67, recent clang and gcc compilers and MSVC 15.9.7. It is rather fragile on MSVC, but should work if you take care not to have to complex expressions.
## Design goals
 - Debuggability. If you have a problem with your code, it should be reasonably easy to solve using a debugger. 
 - Have a syntax that faciliates automatic code generation. One current usage of qdpeg is as a part of a module for automatic conversion of native data to or from text.
 - Have a value-centric view of the parsing. All our parsers return a value on success (could be an integer, a syntax tree or the special whitespace value). It is possible to validate data as it is parsed.
 - No surprises: A parser is a regular type. You can copy and move them as you wish.

Performance is not a goal, so do test performance if you intend to parse e.g. multi-megabyte xml or json files. That said, qdpeg is inherently easy to optimize giving good performance in the scenarios where it is currently used. There are no virtual functions, type-erasure or other constructs that might prevent an optimizer from doing its job.
## Evolution of qdpeg
Following items are on my todo/wishlist:
 - Allowing state to be added to parsers. It would be nice if you could pass state to the parsers - e.g. to hold positions of text and a general symbol table.
 - Less generic error messages.
 - Unicode support.
 - Check performance and consider possible improvements.
 - Detection of left-recursion.

# Anatomy of parsers and generators
In qdpeg, a parser is a callable object such as a function or lambda, that when called with a begin and an end to input either succeeds and returns the parsed object and a position in the input, marking where new input can be consumed or an error, containing a position where parsing failed and a generic error-message.
If a parser parses data of a given type T, the parser returns a `Parse_result<T>`. Here we name parsers according to what they return so a string parser is a parser that returns `Parse_result<std::string>`. A whitespace parser is called a skipper. Formally it returns `Parse_result<Nothing>`, where Nothing is a special, empty type.

You can write your own parser from scratch. This simple parser parses an ascii digit:
```c++
Parse_result<char> parse_digit(Iter b, Iter e)
{
    if (b == e) return { b, unexpected_eof };           //  End of file
    auto res = *b;
    if (res >= '0' && res <= '9')
        return { ++b, res };                            //  Found a digit - return it
    return { b, unexpected_char };                      //  Not a digit
}
```
The return value of a parser is convertible to bool. It converts to true if and only if the parsing succeeds. If the parsing succeeds, use value() to get the result of a successful parse, error() to get the error code. 
A high-order parser is a function (or, more formally, a `callable`) that returns a parser. A simple example:
```c++
auto char_range(char lo,char hi)
{
    return [lo,hi](Iter b,Iter e) -> Parse_result<char>
    {
        if (b == e) return { b, unexpected_eof };
        auto res = *b;
        if (res >= lo &&  res <= hi) return { ++b, res };
        return { b, unexpected_char };
     };
}
```
So char_range(('0','9'))(begin,end) has exactly the same effect as calling parse_digit(begin,end).

# Examples and reference
In the examples following, you should expect a using namespace qdpeg directive and that appropriate files have been included.
Also expect a function parse that takes a string and a parser and returns a result and a string. parse parses the string using the supplied parser.
The expected output is written as a comment after the call, perhaps with an explanation in parenthesis as here:
```c++
parse("- 1734",int_parser<int>); // failure/" 1734" (space not allowed)
```
So parsing "- 1734" with the int parser results in a failure where " 1734" was not parsed. As the comment indicates, this is because a space can not be part of an integer.
The example-code given here can be found (with small modifications for checking of results) in the file example.cpp.

## Built-in parsers
The built-in parsers parse a given type. There is support for standard C++ data-types such as char, integrals and floating-point types.
* [character parsers](##character-parsers)
* [integer parsers](##Integer-parsers)
* [floating-point parsers](#floating-point-parsers)
* [Symbol parsers](#Integer-parsers)
* [skip parsers](##skip-parsers)

## Parser generators
Parser generators are generic parsers that combine other parsers to generate new and often more complex parsers and do as such form the soul of a real parser. You can use this type of generator to create e.g. XML and JSON parsers.
** [Repeat parsers](#Repeat-parsers)
** [Sequence parsers](#Sequence-parsers)
** [Choice parsers](#Choice-parsers)
** [The and_p and not_p parsers](#The-and_p-and-not_p-parsers)

## Character parsers

| Signature | Function |
| --- | --- |
|`char_any`| parses any character.
|`template<char... Chars> char_from`| parses any character in the template parameter list.
|`template<char... Chars> char_not_from`| parses any character not in the template parameter list.
|`template<class F> char_if(F true_cond)`| returns a parser that parses any character c for which 
true_cond(c) holds.

There are also helper functions
## Integer parsers
int_parser is a templated function that supports parsing of integral signed and unsigned types. The text is interpreted as an integer in the given radix, failing also if the value can't be represented by the internal type.


Function declaration:
```c++
    template <class T,
        unsigned Radix  = 10,
        sign_policy sp  = std::is_signed<T>() 
                ? sign_policy::allowed
                : sign_policy::none,
        int MinDigits   = 1,
        int MaxDigits   = std::numeric_limits<int>::max()>
    auto int_parser(Iter b,Iter e)
            -> Parse_result<T>
```
Where:
| Parameter | Function |
| --- | --- |
|`T` | integer type to parse| 
|`Radix`| a number from 2 to 36 |
|`sp`| enum as below |
|`MinDigits`| is the least number of integers allowed|
|`MaxDigits`| is the largest number of digits allowed|

Parsing of the sign is determined by the sign_policy enum class. Allowed values are

| Value | Meaning|
|---|---|
|`none`| sign is not allowed|
|`minus`| optional - is allowed
|`allowed`| optional - or + is allowed
|`required`| when an optional - or + is required |none for unsigned types and allowed for signed types|

### Examples
```c++
parse("17.34%",int_parser<int>); // 17/".34%"
parse("-1734",int_parser<int>); // -1734/""
parse("- 1734",int_parser<int>); // failure/" 1734" (space not allowed)
parse("999999999999999 balloons",int_parser<int>); // failure/" balloons" (overflow assumed)
parse("999999999999999 balloons",int_parser<int,10,sign_policy::allowed,5,5>); // 99999/"9999999999 balloons"
parse("a42f",int_parser<unsigned char,16,sign_policy::allowed,2,2>);  // 0xa4/"2f"
parse("17.34%",int_parser<int,10,sign_policy::required>); // failure/"17.34%"
```
## Floating point parser
Parses real types (float, double and long double). Synopsis:
```c++
template<class Real,
    sign_policy sp          = sign_policy::allowed,
    decpoint_policy dec_p   = decpoint_policy::allow_point,
    inf_nan_policy inf_nan  = inf_nan_policy::allowed,
    exp_policy ep           = exp_policy::allowed,
    int  decimals_min       = 0,
    int  digits_min         = 0,
    int  decimals_max       = decimals_min < 2
                            ? std::numeric_limits<int>::max()
                            : decimals_min,
    int  digits_max         = digits_min < 2
                            ? std::numeric_limits<int>::max()
                            : digits_min>
auto real_parser(Iter b,Iter e) -> Parse_result<Real>
```

| Parameter | Function |
| --- | --- |
|`Real` | Resulting type |
|`sp` | sign format |
|`dec_p` | decimal point format |
|`inf_nan` | format for inf and nan | 
|`exp_p `| exponent format | 
|`decimals_min` |least numbers in decimal part|
|`digits_min` | least numbers in whole part |
|`digits_max` | most numbers in whole part |
|`decimals_max` | most numbers in decimal part|

exp_p determines the presence of the C++-style exponent part using the exponent_format enumeration class with these options:
| Value | Meaning|
|---|---|
| none |  Not accepted | 
| allowed | allowed | 
| required | required | 

dec_p determines the presence and format of the decimal seperator using the decpoint_policy enum class giving these options:
| Value | Meaning|
|---|---|
| allow_point | Allow a '.' as separator | 
| require_point | Require a '.' as separator | 


## Symbol
The Symbol templates are generators that parse text from a list of key-value pairs, giving a constant mapping from text to a value of the parsed type. The parse results in the longest key-element found in the current text. There are two variants, one giving a case insensitive mapping.

```c++
template<class V> struct Symbol;       //  symbol
template<class V> struct Symbol_ci;    //  case-insensitive symbols
```
Construct the symbol-table by providing a list of key-value pairs, and the symbol-table is a valid parser.
```c++
enum class m_names
{
    mae,
    mary,
    mary_lou,
    mary_ellen
};

Symbol_ci<m_names> m_name_parser {
    { "Mae"			, m_names::mae},
    { "Mary"		, m_names::mary},
    { "Mary-Lou"	, m_names::mary_lou},   // Key can contain any character
    { "Mary Ellen"  , m_names::mary_ellen}  // ... including space
};

parse("Mary-Jane",m_name_parser);     // mary/""            (Greedy match)
parse("MARY-LOU",m_name_parser);      // mary_lou/""        (m_name parser is Case-insensitive)
parse("Mae",m_name_parser);           // mae/""
parse("Maes",m_name_parser);          // mae/"s"            (Greedy match)
parse("Mary Ellen",m_name_parser);    // mary_ellen/""
```
## Skip parsers
Whitespace is traditionally understood as characters such as space, newlines and tabs. In qdpeg, a skiper is a parser that whitespace is defined as any sequence that does not contain anything that should be returned to the parser.
A skipper parses generic white space - text that may or must be present in the source without directly contributing to a value, but simply act as delimiters or separators. 

| Signature | Function |
| --- | --- |
|`textspace`|Skips standard whitespace such as spaces, tabs, newline
|`linespace`|Whitespace within a line
|`eof`| succeeds at end of file
|`eol`| succeeds at end of line, supporting Unix, Windows and Mac style line-endings
|`lit(char ch)`| parses ch and skips it
|`ci_lit(char ch)`| as lit but ignoring case
|`lit(string)`| Skips a string 
|`ci_lit(string)`|Skips a string ignoring case
|`spaced_lit(char ch)`| Skips ch ignoring text-space around ch
|`skip(Parser P)`| Skips ch ignoring text-space around ch
|`empty`| Always succeeds
|`fail`|  Always fails
|`template<class T> fail_as`|  Always fails, using a non-skipper result

`textspace` skips standard white space and `linespace` skips white space that does not create a new line
```c++
parse("\n\t  \n\r  Hello",textspace);   // OK/"Hello" 
parse("",textspace);        // OK/"" (whitespace match)

parse("\t \t \n\r  Hello",linespace);   // OK/"\n\r  Hello" 
parse("Hello",linespace);   // OK/"Hello" 
parse("",linespace);        // OK/"" (whitespace matches)
```
`lit` and `ci_lit` matches an individual character or string, with `ci_lit` ignoring case.
```c++
parse("e",lit('e'));        // Ok  
parse("E",lit('e'));        // Fails - wrong case

parse("if",lit("if"));      // Ok 
parse("If",lit("if"));     // fails - wrong case

parse("iF",ci_lit("if"));   // OK - case insensitive
```

`spaced_lit` skips a single character and its surrounding white space
```c++
parse("   ,  ",spaced_lit(','));
```

`eol` parses new-line characters (\n, \r, \n\r or \r\n). It also succeeds at end of file.
`eof` only succeeds when all input is consumed.
```c++
parse("\nHello",eol);       //  Ok/"Hello"
parse("\r\nHello",eol);     //  Ok/"Hello" (Also supports Windows)
parse("",eol);              //  Ok/"" (eof is also eol)
parse("",eof);              
parse("\n",eof);            //  Failure/"\n" (eof does not skip white space)
```
`skip` is our first meta-parser, introduced here. It is a generator which takes a parser as input, parser using that parser returning the result as a Skipper, discarding the result.
```c++
parse("123 is a number",skip(int_parser<int>));  //  Ok/"is a number"
```
`empty` and `fail` are two special skippers. `empty` always succeeds ands `fail` always fails. They can be useful when constructing other parsers.
`fail_as` also fails but returns a typed value. Technically, this makes `fail_as` a real parser, not a skipper
```c++
parse("Hello",empty);   // Ok/"Hello"
parse("Hello",fail);    // Failure/"Hello"
parse("Hello",fail);    // Failure/"Hello"
parse("Hello",fail_as<int>);    // Failure/"Hello"  (fail_as<int> is an int parser)
```

#Parser generators
Parser generators are generic parsers that combine other parsers to generate new and often more complex parsers and do as such form the soul of a real parser.
Some of these parsers come in two shapes: they can be weakly typed or strongly typed. 
A weakly typed parser is a parser that has not been told to behave as a particular type. In this case, their type is determined by qd_peg. As an example, a sequence parser could return a std::tuple and a choice parser a std::variant.
For a strongly typed parser, the user supplies the type and the type of the parser then becomes the type supplied. If the actual object can not be created from its weak type, the result is a compile-time error.

##Repeat parsers
The repeat-group of parsers parses a sequence of elements of the same type with the option to restrict the number of elements to parse and the possibility to add a skipper to parse white space between elements.
| Synopsis | Function |
| --- | --- |
|´repeat(Parser e,Sep ws,Countrule cr)`| parses N to M white space seperated elements|
|´repeat<T>(Parser e,Sep ws,Countrule cr)`| parses N to M white space seperated elements|

The first function is weakly typed

##Sequence parsers
##Choice parsers
##The and_p and not_p parsers

