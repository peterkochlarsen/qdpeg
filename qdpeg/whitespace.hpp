#pragma once
#include <qdpeg/cpp20.hpp>
#include <qdpeg/qdbase.hpp>
#include <qdpeg/parse_char.hpp>
#include <qdpeg/repeat.hpp>
#include <qdpeg/strlit.hpp>
#include <qdpeg/utility.hpp>
#include <algorithm>
#include <cctype>

namespace qdpeg
{
    template<class P>
    auto skip(P parser)
    {
        static_assert(is_parser<P>());
        return [parser](Iter b,Iter e) mutable -> Skipper
        {
            auto res = parser(b,e);
            return res;
        };
    }

    inline auto lit(char ch)
    {
        return [ch](Iter b,Iter e) mutable -> Skipper
        {
            if (b == e)
                return { b,Error_code::unexpected_eof};
            char res {*b};
            if (res != ch)
                return { b,Error_code::unexpected_char };
            return { ++b };
        };
    }

    template<class T>
    auto make_literal(T s) 
    {
        return [s = std::move(s)](Iter b,Iter e) mutable -> Skipper
        {
            auto avail = e - b;
            if (avail < static_cast<signed long>(s.size()))
                return { b,Error_code::unexpected_eof };

            auto new_b = b + cpp20::ssize(s);
            if (s != make_raw(b,new_b))
                return parse_status { b,Error_code::expected_string };
            return { new_b };
        };
    }

    inline auto lit(str_lit s)
    {
        return make_literal(s);
    }

    inline auto ci_lit(ci_strlit s)
    {
        return make_literal(s);
    }

    inline auto textspace(Iter begin,Iter end) -> Skipper
    {
        auto parser = repeat<Nothing>(space());
        return parser(begin,end);
    }

    inline auto linespace(Iter begin,Iter end) -> Skipper
    {
        auto parser = repeat<Nothing>(lspace());
        return parser(begin,end);
    }

    inline auto spaced_lit(char ch)
    {
        return [ch](Iter b,Iter e) mutable -> Skipper
        {
            b  = textspace(b,e);
            if (std::isspace(ch))
                return parse_status {b};
            auto res = lit(ch)(b,e);
            if (!res) return res;
            b = textspace(res,e);
            return parse_status {b};
        };
    }
}   //  qdpeg
