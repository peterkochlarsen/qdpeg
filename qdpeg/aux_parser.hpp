#pragma once
#include <qdpeg/err_code.hpp>
#include <qdpeg/whitespace.hpp>
#include <qdpeg/qdbase.hpp>
#include <optional>

namespace qdpeg
{
    template<class P>
    constexpr auto opt(P p)
    {
        using opt_res = std::conditional_t<is_skipper<P>(),
            Nothing,
            std::optional<Parsed_type<P>>>;

        using result_type = Parse_result<opt_res>;
        return [p](Iter b,Iter e) -> result_type
        {
            auto parsed = p(b,e);
            if (parsed)
                return { parsed.iter,opt_res { std::move(parsed.value())}};
            return { b, opt_res {} };
        };
    }

    template<class P,class BF>
    constexpr auto check(P p,BF f)
    {
        static_assert(is_parser<P>(),"Parser accepted");

        return [p,f](Iter b,Iter e) mutable -> Parsed_return<P>
        {
            auto res = p(b,e);
            if (res)
            {
                bool ok = f(res.value());
                if (!ok)
                    return { res.iter, Error_code::expected_type };
            }
            return res;
        };
    }


    template<class MustP,class P>
    constexpr auto and_p(MustP must,P p)
    {
        static_assert(is_parser<MustP>() && is_parser<P>(),"Only parsers accepted");

        return [must,p](Iter b,Iter e) mutable -> Parsed_return<P>
        {
            auto temp = must(b,e);
            if (temp)
                return p(b,e);
            return { temp.iter, Error_code::symbol_not_found };
        };
    }

    template<class P>
    constexpr auto and_p(P ps)
    {
        return and_p(ps,empty);
    }

    //  Parse p only if you could not parse NonP
    template<class NonP,class P>
    constexpr auto not_p(NonP nonp, P p)
    {
        static_assert(is_parser<NonP>() && is_parser<P>(),"Only parsers accepted");

        return [nonp,p](Iter b,Iter e) mutable -> Parsed_return<P>
        {
            auto temp = nonp(b,e);
            if (temp)
            {
                return { b,Error_code::symbol_not_expected };
            }
            return p(b,e);
        };
    }

    template<class NonP>
    constexpr auto not_p(NonP ps)
    {
        return not_p(ps,empty);
    }

    //  Always emit a T with value t
    template<class T>
    constexpr auto emit(T t)
    {
        return [t](Iter b,Iter ) -> Parse_result<T>
        {
            return { b , t };
        };
    }

    template<class P>
    constexpr auto when(bool allow,P ps)
    {
        return [allow,ps](Iter b,Iter e) -> Parsed_return<P>
        {
            if (allow)
                return ps(b,e);
            return { b, Error_code::always_fail };
        };
    }

    template<bool B,class PTrue,class PFalse=Empty_skipper>
    constexpr auto conditional(PTrue ps)
    {
        if constexpr (B)
        {
            return ps;
        }
        else
        {
            return PFalse();
        }
    }

    template<class P>
    constexpr auto raw(P ps)
    {
        return [ps](Iter b,Iter e) -> Parse_result<Raw>
        {
            auto res = skip(ps)(b,e);
            if (res)
                return { res.iter, make_raw(b,res.iter) };
            return { res.iter, res.error() };
        };
    }

    template<class T,class P>
    constexpr auto as(P ps)
    {
        return [ps](Iter b,Iter e) mutable -> Parse_result<T>
        {
            auto res = ps(b,e);
            if (res)
                return { res.iter, static_cast<T>(std::move(res.value())) };
            return { res.iter, res.error() };
        };
    }

    template<class P,class T>
    constexpr auto as(P ps,T t)
    {
        using org_return = Parsed_type<P>;
        if constexpr (std::is_invocable_v<T,org_return>)
        {
            using ret_type = std::invoke_result_t<T,org_return>;
            return [ps,t](Iter b,Iter e) mutable -> Parse_result<ret_type>
            {
                auto res = ps(b,e);
                if (res)
                    return { res.iter, t(res.value()) };
                return { res.iter, res.error() };
            };
        }
        else
        {
            static_assert(is_skipper<P>(),"Converting to value should be from white space");
            return [ps,t](Iter b,Iter e) mutable -> Parse_result<T>
            {
                auto res = ps(b,e);
                if (res)
                    return { res.iter, t };
                return { res.iter, res.error() };
            };
        }
    }
}   //  qdpeg
