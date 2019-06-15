# qdpeg
Welcome to qdpeg, yet another parsing expression grammar. I propose a linear read/skim of this document, but if you wish, you can skip directly to the relevant sections:
* [Introduction](#introduction) which has a short motivation for our library.
* [Examples and reference](#Examples-and-reference) to find code snippets and more information.
* [Guide-lines and evolution](#Examples-and-reference) where you will find some thoughts about the evolution and guide-lines for how you should use qdpeg.

# Introduction
qdpeg is a simple (quick and dirty) [Parsing Expression Grammer](https://en.wikipedia.org/wiki/Parsing_expression_grammar) parsing library.

If you need code that helps you convert text to a native C++ type, this library might be for you. qdpeg contains:
  - [built-in parsers](##built-in-parsers) for stuff such as characters, integers and whitespace
  - [parser-generators](##parser-generators) which combine individual parsers into new, more powerful ones
  - [adapters](##adapters) that manipulate the result of a parser to eg. perform further checks of the parsed value and/or transform the type of the parsed value.
## Requirements
qdpeg is header-only, requires C++17 and a version of boost containing [mp11](https://github.com/boostorg/mp11). Tested with boost 1.67, recent clang and gcc compilers and MSVC 15.9.7. It is rather fragile on MSVC, but should work if you take care not to have to complex expressions.
## Design goals
 - Debuggability. If you have a problem with your code, it should be reasonably easy to solve using a debugger. 
 - Have a syntax that faciliates automatic code generation. One current usage of qdpeg is as a part of a module for automatic conversion of native data to or from text.
 - Have a value-centric view of the parsing. All our parsers return a value on success (could be an integer, a syntax tree or the special whitespace value). It is possible to validate data as it is parsed.
 - No surprises: A parser is a regular type. You can copy and move them as you wish.

Performance is not a goal, so do test performance if you intend to parse e.g. multi-megabyte xml or json files. That said, qdpeg is inherently easy to optimize giving good performance in the scenarios where it is currently used. There are no virtual functions, type-erasure, dynamic memory allocation or other constructs that might prevent an optimizer from doing its job.
## Anatomy of parsers and generators
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
Also assume a function parse that takes a string and a parser and returns a result and a string. `parse` parses the string using the supplied parser.
The expected output is written as a comment after the call, perhaps with an explanation in parenthesis as here:
```c++
parse("- 1734",int_parser<int>); // failure/" 1734" (space not allowed)
```
So parsing "- 1734" with the int parser results in a failure where " 1734" was not parsed. As the comment indicates, this is because a space can not be part of an integer.
The example-code given here can be found (with small modifications for checking of results) in the file example.cpp.
Finally, in the description of functions I have weighted readability over conciseness. As an example, the function `<char... Chars> char_from` is actually declared as 
```c++
template<char... Chars> 
auto char_from(Iter b,Iter e) -> Parse_result<char>;
```
but cutting a few corners does, I believe, increase readability in tables.

## Built-in parsers
The built-in parsers parse a given type. There is support for standard C++ data-types such as char, integrals and floating-point types.
* [character parsers](##character-parsers)
* [integer parsers](##Integer-parsers)
* [floating-point parsers](#floating-point-parsers)
* [Symbol parsers](#Integer-parsers)
* [emit](#emit)
* [skip parsers](##skip-parsers)

## Parser generators
Parser generators are generic parsers that combine other parsers to generate new and often more complex parsers and do as such form the soul of a real parser. You can use this type of generator to create e.g. XML and JSON parsers.
* [Repeat parsers](#Repeat-parsers)
* [Sequence parsers](#Sequence-parsers)
* [Choice parsers](#Choice-parsers)
* [The and_p and not_p parsers](#The-and_p-and-not_p-parsers)

## Character parsers
All character parsers fail at end of file
| Signature | Succeeds when |
| --- | --- |
|`char_any`| any character is present
|`<char... Chars> char_from`| Succceds if char is in the parameter list.
|`<char... Chars> char_not_from`| Succceds if char is in the parameter list.
|`char_if(F true_cond)`| returns a parser that parses any character c for which true_cond(c) holds.
|`lspace`| Helper: parses space within a line (\t,' ')
|`space`| Helper: parses whitespace
|`digit`| Helper: parses a digit
|`lower`| Helper: parses a lowercase letter
|`upper`| Helper: parses an uppercase letter
|`alpha`| Helper: parses a letter
|`alnum`| Helper: parses a letter or a digit
|`print`| Helper: parses a printable character
    
Examples
```c++
parse("123",digit()); // '1'/"23"
parse("123",alpha()); // Failure/"123"
parse("qdpeg",char_from<'q','e','d'>);      // 'q'/"dpeg"
parse("qdpeg",char_not_from<'q','e','d'>);  // Failure/"qdpeg"
bool is_vowel(char ch) 
{ 
    return ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' 
    || ch == 'o' || ch == 'y' || ch == 'u';
}

parse("qdpeg",is_vowel);  // Failure/"qdpeg"
```
## Integer parsers
int_parser is a templated function that supports parsing of integral signed and unsigned types. The text is interpreted as an integer in the given radix, failing also if the value can't be represented by the internal type.

Synopsis:
```c++
template <class T,
    unsigned Radix  = 10,
    sign_policy sp  = std::is_signed<T>() 
            ? sign_policy::allowed
            : sign_policy::none,
    int MinDigits   = 1,
    int MaxDigits   = MinDigits < 2
                ? std::numeric_limits<int>::max()
                : MinDigits>
auto int_parser(Iter b,Iter e)
        -> Parse_result<T>;
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
|`minus`| optional - is allowed|
|`allowed`| optional - or + is allowed|
|`required`| - or + is required |

### Examples
```c++
parse("17.34%",int_parser<int>); // 17/".34%"
parse("-1734",int_parser<int>); // -1734/""
parse("- 1734",int_parser<int>); // failure/" 1734" (space not allowed)
parse("999999999999999 balloons",int_parser<int>); // failure/" balloons" (overflow assumed)
parse("999999999999999 balloons",int_parser<int,10,sign_policy::allowed,5,5>); // 99999/"9999999999 balloons"
parse("a42f",int_parser<unsigned char,16,sign_policy::allowed,2,2>);  // 0xa4/"2f"
parse("17.34%",int_parser<long long,10,sign_policy::required>); // failure/"17.34%"
```
## Floating point parser
Parses real types (float, double and long double). Synopsis:
```c++
template<class Real,
    sign_policy sign_p      = sign_policy::allowed,
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
|`sign_p` | sign format - as for the integer types |
|`dec_p` | decimal point format |
|`inf_nan` | format for inf and nan | 
|`exp_p `| exponent format | 
|`decimals_min` |least numbers in decimal part|
|`digits_min` | least numbers in whole part |
|`decimals_max` | most numbers in decimal part|
|`digits_max` | most numbers in whole part |

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
    { "Mae"         , m_names::mae},
    { "Mary"        , m_names::mary},
    { "Mary-Lou"    , m_names::mary_lou},   // Key can contain any character
    { "Mary Ellen"  , m_names::mary_ellen}  // ... including space
};

parse("Mary-Jane",m_name_parser);     // mary/""            (Greedy match)
parse("MARY-LOU",m_name_parser);      // mary_lou/""        (m_name parser is Case-insensitive) 
parse("Mae",m_name_parser);           // mae/""
parse("Maes",m_name_parser);          // mae/"s"            (Greedy match)
parse("Mary Ellen",m_name_parser);    // mary_ellen/""
```
## emit
The `emit` parser generator is a simple parser that always succeeds, returning a value of some type. 

### Example
```c++
//  An int parser, that returns 42 without consuming any input
auto fortytwo = emit(42);
parse("whatever",fortytwo);   // 42/"whatever"
//  Parse nothing, returning the four smallest primes
parse("",emit(std::vector<int>{2,3,5,7}));  
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
parse("   ,  ",spaced_lit(','));    // Ok/""
```
`eol` parses new-line characters (\n, \r, \n\r or \r\n). It also succeeds at end of file.
`eof` only succeeds when all input is consumed.
```c++
parse("\nHello",eol);       //  Ok/"Hello"
parse("\r\nHello",eol);     //  Ok/"Hello" (Also supports Windows)
parse("",eol);              //  Ok/"" (eof is also eol)
parse("",eof);              //  Ok/""
parse("\n",eof);            //  Failure/"\n" (eof does not skip white space)
```
`skip` is a generator which takes a parser as input. It parses using its argument, discards any value and returns the result as a Skipper.
```c++
parse("123 is a number",skip(int_parser<int>));  //  Ok/"is a number"
```
`empty` and `fail` are two special skippers. `empty` always succeeds ands `fail` always fails. They can be useful when constructing other parsers.
`fail_as` also fails but returns a typed value. Technically, this makes `fail_as` a real parser, not a skipper
```c++
parse("Hello",empty);   // Ok/"Hello"
parse("Hello",fail);    // Failure/"Hello"
parse("Hello",fail_as<int>);    // Failure/"Hello"  (fail_as<int> is an int parser)
```

# Parser generators
Parser generators are generic parsers that combine other parsers to generate new and often more complex parsers and do as such form the soul of a real parser.
Some of these parsers come in two shapes: they can be weakly typed or strongly typed. 
A weakly typed parser is a parser that has not been told to behave as a particular type. In this case, their type is determined by qd_peg. As an example, a sequence parser could return a std::tuple and a choice parser a std::variant.
For a strongly typed parser, the user supplies the type and the type of the parser then becomes the type supplied. If the actual object can not be created from its weak type, the result is a compile-time error.
## Repeat parsers
The repeat-group of parsers parses a sequence of elements of the same type with the option to restrict the number of elements to parse and the possibility to add a skipper to parse white space between elements.
| Synopsis | Function |
| --- | --- |
|`repeat(Parser e,Skip ws,Length_checker lchk)`| parses N to M white space seperated elements|
|`repeat<T>(Parser e,Skip ws,Length_checker lchk)`| parses N to M white space seperated elements|

The first function is weakly typed, returning a skipper if given a skipper, a std::string if e is a char-parser and a std::vector of the element e parses to. In both cases, the Skip function must be a skipper.

`Length_checker` is a class checking the lengths allowed for repeated input. Instantiate it using one of these function:
| Synopsis | Function |
| --- | --- |
|`at_least(N)`| lower limit of N items|
|`at_most(N)`| max limit of N items|
|`take(N,M)`| Take from N to M items (inclusive)|
|`take(N)`| Same as `take(N,M)`|

For repeat, you can omit the skipper, in which case the input is parsed without skipping and/or the length_checker in which case there are no restrictions on the number of elements allowed:
| Synopsis | Equivalence  |
| --- | --- |
|`repeat(Parser e,Length_checker lchk)`| `repeat(Parser e,empty,Length_checker lchk)`|
|`repeat<T>(Parser e,Length_checker lchk)`|`repeat<T>(Parser e,empty,Length_checker lchk)`|
|`repeat(Parser e,Skip ws)`| `repeat(Parser e,Skip ws,at_least(0))`| 
|`repeat<T>(Parser e,Skip ws)`|`repeat<T>(Parser e,Skip ws,at_least(0))`| 
|`repeat(Parser e)`| `repeat(Parser e,empty,at_least(0))`| 
|`repeat<T>(Parser e)`|`repeat<T>(Parser e,empty,at_least(0))`| 

### Examples
Here the first parser is weakly typed and the second is strong. Both parse to a std::string.
```c++
parse("John 34",repeat(alpha())); // "John"/" 34"
parse("34 John",repeat<std::string>(alpha())); // ""/"34 John"
```
Parsing with a length restriction (weakly typed to std::string)
```c++
auto digits = repeat(digit(),at_least(1)); //  a weakly typed std::string parser
parse("123 potatoes",digits); // "123"/" potatoes"

auto digit4 = repeat<std::string>(digit(),take(4)); //  a std::string parser
parse("123 potatoes",digit4); // failure/" potatoes"
parse("1234 potatoes",digit4); // "1234"/" potatoes"
parse("12345 potatoes",digit4); // "1234"/"5 potatoes"
```
Parsing with embedded white space
```c++
auto vl = repeat(int_parser<long>,lit(',')); //  A std::vector<long> parser
parse("10,20,30,40,50,hello",vl); // {10,20,30,40,50},",hello"

auto vl3 = repeat(int_parser<long>,spaced_lit(','),take(3)); //  A std::vector<long> parser
parse("10 \t, 20,\n 30,40,50 \n,hello",vl3); // {10,20,30}/",40,50 \n,hello"
```

## Sequence parsers
The seq parsers parse a sequence of items in order. There are four varieties: two strongly typed and two weakly typed, each parsing with or without white space seperators.

| Synopsis | Function |
| --- | --- |
|`seq(Ps... ps)`| Parse p0,...,pn in order without white space. Weakly typed|
|`seq_ws(Skip ws,Ps... ps)`| Parse p0,...,pn in order, seperated by white space. Weakly typed|
|`seq<T>(Ps... ps)`|Parse p0,...,pn in order without white space. Type is T |
|`seq_ws<T>(Skip ws,Ps... ps)`|Parse p0,...,pn in order, seperated by white space. Type is T |

A parser supplied to seq or seq_ws can be a skipper or a real parser. The Skip parameter in seq_ws must be a skipper and will be activated between each ps, also for skippers.

The weakly typed sequence parsers will be of the following type, where all ps that are skippers are ignored:
| When | Resulting type |
| --- | --- |
| ps is empty | a skipper |
| One parser px | Same as px |
| All ps parse to T or a weak repeat of T | As repeat<T> |
| Otherwise | As a std::tuple containing the accumulated results of the parse |

### Examples
A skipper
```c++
auto skip_num_colon = seq(skip(int_parser<int>),lit(':')); //  A skipper
parse("123: a number",skip_num_colon);  //  Ok/" a number"
```
Seq into a single type (weakly typed)
```c++
auto par_int = seq(lit('('),parse_int<int>(),lit(')')); //  
static_assert(std::is_same<Parsed_type<decltype(par_int)>,int>());
parse("(123)",par_int); // 123/""
```
Seq into a std::string (weakly typed)
```c++
auto identifier = seq(alpha(),repeat(alnum()));
static_assert(std::is_same<Parsed_type<decltype(identifier)>,std::string>());
parse("A123 B12",identifier); // "A123"/" B12"
```
Seq into a std::tuple (because the repeat parser is strongly typed)
```c++
auto tup = seq(alpha(),repeat<std::string>(alnum())); 
static_assert(std::is_same<Parsed_type<decltype(tup)>,std::tuple<char,std::string>>());
parse("A123 B12",tup); // {'A',"123"}/" B12"
```
As long as your type is constructible from its constituents, parsing into a user-defined type is straightforward:
```c++
struct S 
{
    char ch;
    std::string s; 
};

auto parse_S = seq<S>(alpha(),repeat(alnum()));
parse("A123 B12",parse_S); // S{'A',"123"}/" B12" - type is S

//  weak/strong types does not matter here
auto parse_Sws = seq_ws<S>(textspace,alpha(),repeat<std::string>(alnum())); 
parse("A B12",parse_Sws); // S{'A',"123"}/" B12" - type is S
parse("A             B12",parse_Sws); // S{'A',"123"}/" B12" - type is S
parse("A  \n\n \r\t \n\n \r\t          B12",parse_Sws); // S{'A',"123"}/" B12" - type is S
```

## Choice parsers
The choice parser generator is created given an ordered list of parsers. It parses input in order, stopping as soon as a matching parser is found. If no parser succeeds, the resulting error marks the location of the longest successful parse.
The choice parser is essential for writing recursive parsers. 

As seq, choice comes in a weakly typed and a strongly typed variant. The parsers must either all be skippers or all be real parsers. Mixing them is a compile-time error.

| Synopsis | Function |
| --- | --- |
|`choice(Ps... parsers)`| weakly typed ordered choice | 
|`choice<T>(Ps... parsers)`| Strongly typed ordered choice | 

The strongly typed choice returns a T-parser. For the weakly typed parser, the return type is deduced as a skipper when all parsers are skippers, as T when all parsers are of type T and as a std::variant<U...>
where U... is the combined unique types returned by the individual parsers.

### Examples

Parse into a user defined struct.
```c++
struct U {
    U(std::string);
    U(int);
};

//  A strongly parsed U-parser
auto U_parser = choice<U>(int_parser<int>,repeat(alnum(),at_least(1)));
parse("45",U_parser);
parse("Str",U_parser);
```
Parse C++ unsigned literals. In practice this would be a template, creating parsers for any unsigned types. Notice how the order is important. If the octal parser was put after the decimal parser, "0144" would be parsed successfully as 144.
```c++
auto cpp_int_lit = choice(
    seq(ci_lit("0x"),int_parser<unsigned,16,sign_policy::none>),
    seq(ci_lit("0b"),int_parser<unsigned,2,sign_policy::none>),
    seq(lit('0'),int_parser<unsigned,8,sign_policy::none>),
    int_parser<unsigned>); 

parse(100u,"100",cpp_int_lit);          //  Decimal
parse(100u,"0144",cpp_int_lit);         //  Octal 
parse(100u,"0x64",cpp_int_lit);         //  Hex
parse(100u,"0b1100100",cpp_int_lit);    //  Binary
```
## The and_p and not_p parsers
The `and_p` and `not_p` parsers come in two varieties. The first type takes a parser and the result is a skipper that for `and_p` succeeds if the parser succeeds and for `not_p` succeeds if p fails. What is special is that none of the parsers consume any input.
The second variety accepts two parsers. These parsers are correspond to a and_p/not_p of the first parameter followed - if this parser succceeded by a call of the second parser
| Synopsis | Generator |
| --- | --- |
|`not_p(Parser p)`| succeeds without consuming input iff p fails  |
|`and_p(Parser p)`| succeeds without consuming input iff p succeeds  |
|`not_p(Parser pf,parser p2)`| same as `seq(not_p(pf),p2)` |
|`and_p(Parser pg,parser p2)`|  same as `seq(and_p(pg),p2)` |

### Examples
Basic example of and_p and not_p functionality
```c++
parse("//",and_p(lit("//")));   // Ok/"//"
parse("//",not_p(lit("//")));   // Failure/"//"

parse("aa",and_p(alnum()));     // Ok/"aa"
parse("aa",not_p(alnum()));     // failure/"aa"
```
An example of parsing a line comment. This code is not fully compliant as line continuation is disregarded.
```c++
auto cpp_line_comment = seq(lit("//"),skip(repeat(not_p(eol,char_any)),eol);
parse("// Hello C++ comment\nint main() {}",cpp_line_comment); //  Ok/"int main() {}"
```

## The opt parser
Synopsis: `opt(Parser p)`

The opt parser generator takes a parser P. If P is a skipper, the resulting parser is a skipper. Otherwise the parser is a std::optional<T> parser, that when P succeeds returns the value of P and otherwise contains nothing (and consumes no input). The opt parser thus always succeeds.

### Example
```c++
parse("42",opt(parse_int<int>)); //  std::optional<int>{42}/""
parse("fortytwo",opt(parse_int<int>)); //  std::optional<int>{}/"fortytwo"
```

# Adapters
An Adapter is based on an existing parser and uses the result from that parser to validate or convert the result. 

##raw
Raw returns the parsed data in a raw text-format. Used internally in our integral parsers and useful in cases where you need to call low-level text conversion functions locally.
Synopsis: `raw(parser)` where parser is a valid parser.
## check
`check` performs additional validation of the parsed result and reports failure in case the pass failed.
Synopsis: 
`check(parser,checker )`
|Parameter|Requirement |
| --- | --- |
| parser | A valid parser of type T |
| checker | checker(t), where t is of type T is a valid expression convertible to bool |

### Example:
Here we only parse even numbers.
```c++
bool is_even(int i) { return i % 2 == 0; }

auto parse_even = check(int_parser<int>,is_even);
parse("46",parse_even);  //  47/""
parse("49",parse_even);  //  failure/""
```
##as
`as` converts the parsed type to another type. 
| Synopsis | Action |
|`as<T>(Parser p)`| Converts the parsed value to a T |
|`as(Skip s,Val v)`| Returns v if the skipper s succeeds |
|`as(Parser p,Func f)`| Converts the parsed value t to a f(t) |

For all `as`-parsers, if the underlying parser fails, the error information is passed unchanged to the resulting value.

### Examples
As part of the parsing of a C++ char constant, you could use the code below to convert a character in a hexadecimal/octal notation:
```c++
auto char_from_hex = as<char>(int_parser<unsigned char,16,sign_policy::none,2,2>);
parse("20",char_from_hex);  // ' '/"" (the space character is 0x20 in hex)
auto char_from_oct = as<char>(int_parser<unsigned char,8,sign_policy::none,1,3>);
parse("40",char_from_oct);  // ' '/"" (the space character is 040 in octal)
```
For a type with special sentinel values, the second form could be used:
```c++
auto adams_value = as(ci_lit("meaning_of_everything"),42);
auto extended_int = choice(adams_value,int_parser<int>);
parse("13",extended_int); //  13/""
parse("Meaning_of_EVERYTHING",extended_int); //  42/""
```
Finally, you can invoke a function on the intermediate result, giving another result-type.
```c++
int twice(char ch) { return 2*ch; };

auto parse_and_conv = as(char_from_oct,twice);
static_assert(std::is_same<Parsed_type<decltype(parse_and_conv)>,int>());
parse("40",parse_and_conv);     64/"" (40 octal is 32, and 2*32 = 64)
```

# Guide-lines and evolution
Warning: qdpeg is an early stage library and change is likely to happen.
## Recursive parsers
You define recursive parsers just like you would define any other recursive C++ function, typically by first declaring the function/callable:
```c++
struct Expression;  //  Holds an arithmetic expression
struct Expr_parser
{
    Parse_result<Expression> operator()(Iter b,Iter e);
};
```
Now you can use your parser like any other parser. But you must be careful with recursive parsers as left-recursion is not supported in qdpeg (and plain PEG-grammars in general).

### Left recursion
Left recursion occurs when a rule references itself as one of the first rules in a choice and is often encountered in formal definitions such as EBNF.

 ```
 S = S A  
    | A
```

Implementing this rule directly in qdpeg would result in infinite recursion at runtime.
Mostly, rewriting left recursive rules is straight-forward. The rule above is simply:

```
S = +A
```

which in qdpeg is ```repeat(A,at_least(1))```.


## Writing your own parsers
Do not access anything in the details namespace. This is subject to change. Other parts of the library might also change, but an attempt is made to avoid that.
If you write your own low-level parser, assume that Iter is a forward iterator.
If your parser is implemented as a call-operator, it should be const.

## Evolution of qdpeg
Following items are on my todo/wishlist:
 - Allowing state to be added to parsers. It would be nice if you could pass state to the parsers - e.g. to hold positions of text and a general symbol table.
 - Better error messages.
 - Unicode support.
 - Check performance and consider possible improvements. 
 - Try to detect left-recursion and work around them if possible.
