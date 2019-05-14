#pragma once
#include <qdpeg/cpp20.hpp>
#include <qdpeg/repeat.hpp>
#include <boost/mp11.hpp>
#include <array>
#include <vector>

namespace qdpeg
{
    using namespace boost::mp11;
}

namespace qdpeg::details
{
    template<class T, class... Args>
    using brace_cons_impl = decltype(T{ std::declval<Args&&>()... });

    template<class T, class... Args>
    using is_brace_constructible = cpp20::is_detected<brace_cons_impl, T, Args...>;

    template<class T, class E>
    struct is_brace_constructible_from_tup : std::false_type {};

    template<class T, class... Args>
    struct is_brace_constructible_from_tup<T, std::tuple<Args...>>
        : is_brace_constructible<T, Args...>
    {};

    template<class Result, class ParseResTup, size_t ... I>
    Result make_braced(ParseResTup& tup, std::index_sequence<I...>)
    {
        return Result{ std::move(std::get<I>(tup).value())... };
    }
    template<class Result, class ParseResTup>
    Result make_braced(ParseResTup& tup)
    {
        using idx_seq = std::make_index_sequence<std::tuple_size_v<ParseResTup>>;
        return make_braced<Result> (tup, idx_seq{});
    }

    using namespace boost::mp11;
    template <size_t I, class Adder, class Skip, class ParseTup>
    Skipper build_array_x(Iter b,
        Iter e [[maybe_unused]] ,
        Adder& a,
        Skip& skip [[maybe_unused]] ,
        ParseTup& ptup [[maybe_unused]] )
    {
        if constexpr (I == std::tuple_size_v<ParseTup>)
        {
            return b;
        }
        else
        {
            if constexpr (I > 0 && !std::is_same_v<Skip, decltype(empty)>)
            {
                Skipper res = skip(b, e);
                if (!res) return res;
                b = res.iter;
            }
            using Parser = std::tuple_element_t<I, ParseTup>;
            if constexpr (is_skipper<Parser>())
            {
                auto res = std::get<I>(ptup)(b, e);
                if (!res) return res;
                b = res.iter;
            }
            else
            {
                if constexpr (is_extended_parser<Parser, Adder>)
                {
                    auto res = std::get<I>(ptup)(b, e, a);
                    if (!res) return { res.iter, res.error() };
                    b = res.iter;
                }
                else
                {
                    auto res = std::get<I>(ptup)(b, e);
                    if (!res) return { res.iter, res.error() };

                    if constexpr (I == 0) a.add(std::move(res.value()));

                    b = res.iter;
                }
            }
            return build_array_x<I + 1>(b, e, a, skip, ptup);
        }
    }

    template<class RetType, class Skip, class... Ps>
    struct seq_to_array
    {
        seq_to_array(Skip& s, Ps& ... ps)
            : skip(std::move(s))
            , tup_parse(std::move(ps)...)
        {}
        auto operator()(Iter b, Iter e) -> Parse_result<RetType>
        {
            using Adder = details::element_adder<RetType>;

            Adder adder;
            auto res = build_array_x<0>(b, e, adder, skip, tup_parse);
            if (res)
            {
                return { res.iter, adder.return_value() };
            }
            return { res.iter, res.error() };
        }
        Skip                skip;
        std::tuple<Ps...>   tup_parse;
    };

    template <size_t I, class Skip, class ParseTup>
    auto build_skip_tup(Iter b,
        Iter e [[maybe_unused]] ,
        Skip& skip [[maybe_unused]] ,
        ParseTup& ptup [[maybe_unused]] ) -> Skipper

    {
        //if constexpr (true) { return {b}; } else
        if constexpr (I == std::tuple_size_v<ParseTup>)
        {
            return { b };
        }
        else
        {
            auto res = std::get<I>(ptup)(b,e);
            if (!res) return res;
            if constexpr (I + 1 < std::tuple_size_v<ParseTup>)
            {
                res = skip(res.iter,e);
                if (!res) return res;
            }
            return build_skip_tup<I + 1>(res.iter,e,skip,ptup);
        }
    }

    template <size_t I, size_t R, class Result, class Skip, class ParseTup>
    Skipper build_tup_x(Iter b,
        Iter e [[maybe_unused]] ,
        Result & r [[maybe_unused]] ,
        Skip & skip [[maybe_unused]] ,
        ParseTup& ptup [[maybe_unused]] )
    {
        if constexpr (I == std::tuple_size_v<ParseTup>)
        {
            return b;
        }
        else
        {
            using Parser = std::tuple_element_t<I, ParseTup>;
            auto& p = std::get<I>(ptup);

            if constexpr (I > 0 && !std::is_same_v<Skip, decltype(empty)>)
            {
                Skipper res = skip(b, e);
                if (!res) return res;
                b = res.iter;
            }
            if constexpr (is_skipper<Parser>())
            {
                auto res = p(b, e);
                if (!res) return res;
                return build_tup_x<I + 1, R>(res.iter, e, r, skip, ptup);
            }
            else
            {
                std::get<R>(r) = p(b, e);

                if (!std::get<R>(r)) return { std::get<R>(r).iter, std::get<R>(r).error() };

                return build_tup_x<I + 1, R + 1>(std::get<R>(r).iter, e, r, skip, ptup);
            }
        }
    };

    template<class RawResult, class Skip, class ParseTup>
    auto build_tup(Iter b,
        Iter e,
        Skip & skip,
        ParseTup& ptup)
    {
        Skipper dummy;
        Parse_result<char> data;
        RawResult  rresult;
        std::pair<Skipper, RawResult> result;
        result.first = build_tup_x<0, 0>(b, e, result.second, skip, ptup);
        return result;
    }

    template<class RetType, class Skip, class... Ps>
    struct seq_to_tuple
    {
        using real_parsers = mp_remove_if<std::tuple<Ps...>, is_skipper_t>;
        using parse_results = mp_transform<Parsed_return, real_parsers>;

        seq_to_tuple(Skip& s, Ps& ... ps)
            : skip(std::move(s))
            , tup_parse(std::move(ps)...)
        {}

        auto operator()(Iter b, Iter e) -> Parse_result<RetType>
        {
            auto raw_result = build_tup<parse_results>(b, e, skip, tup_parse);
            if (!raw_result.first)
            {
                return { raw_result.first.iter, raw_result.first.error() };
            }
            return {
                raw_result.first.iter,
                details::make_braced<RetType>(raw_result.second)
            };
        }
    private:
        Skip                skip;
        std::tuple<Ps...>   tup_parse;
    };

    //  Impl when all parsers are in the same category
    template<size_t real_ps, class RPList>
    struct seq_cat_same
    {
        using isolate_elem = mp_transform<isolate_element, RPList>;
        using result_elem = mp_first<isolate_elem>;
        using std_tuple = mp_rename<mp_transform<Parsed_type, RPList>, std::tuple>;
        using std_type  = container_for<result_elem>;

        template <class Ret, class Skip, class... Ps>
        static auto typed_parse(Skip& s, Ps& ... ps)
        {
            if constexpr (is_brace_constructible_from_tup<Ret, std_tuple>())
            {
                return seq_to_tuple<Ret, Skip, Ps...> { s, ps... };
            }
            else
            {
                return seq_to_array<Ret, Skip, Ps...> { s, ps... };
            }
        }

        template <class Skip, class... Ps>
        static auto untyped_parse(Skip& s, Ps& ... ps)
        {
            return typed_parse<std_type>(s, ps...);
        }
    };  //  seq_cat_same

     //  Implementation when there is only one real parser
    template<class RPList>
    struct seq_cat_same<1, RPList>
    {
        using parsed_result = Parsed_return<mp_first<RPList>>;
        using std_result = typename parsed_result::result_type;

        template <class Result, class Skip, class... Ps>
        static auto typed_parse(Skip& s, Ps& ... ps)
        {
            return[skip = std::move(s), 
                tup_parse = std::tuple{ std::move(ps)... }] (Iter b, Iter e) mutable 
                ->Parse_result<Result>
            {
                auto raw_result = build_tup<std::tuple<parsed_result>>(b, e, skip, tup_parse);
                if (raw_result.first)
                {
                    if constexpr (std::is_same_v<Result, std_result>)
                    {
                        std::get<0>(raw_result.second).iter = raw_result.first.iter;
                        return std::move(std::get<0>(raw_result.second));
                    }
                    else
                    {
                        return
                        {
                            raw_result.first.iter,
                            Result{ std::get<0>(raw_result.second).value() }
                        };
                    }
                }
                return { raw_result.first.iter, raw_result.first.error() };
            };
        }

        template <class Skip, class... Ps>
        static auto untyped_parse(Skip& s, Ps& ... ps)
        {
            return typed_parse<std_result>(s, ps...);
        }
    };  //  seq_cat_same

    template<size_t unique_elem, size_t real_ps, class RPList>
    struct seq_category
    {
        using std_result = mp_rename<mp_transform<Parsed_type, RPList>, std::tuple>;
        template <class Skip, class... Ps>
        static auto untyped_parse(Skip& s, Ps& ... ps)
        {
            return seq_to_tuple<std_result, Skip, Ps...> { s, ps... };
        }
        template <class Ret, class Skip, class... Ps>
        static auto typed_parse(Skip& s, Ps& ... ps)
        {
            return seq_to_tuple<Ret, Skip, Ps...> { s, ps... };
        }
    };

    //  There are no real parsers: seq are all skippers.
    template<size_t real_ps, class RPList>
    struct seq_category<0, real_ps, RPList>
    {
        using result = Nothing;
        static_assert(real_ps == 0);

        template <class Skip, class... Ps>
        static auto untyped_parse(Skip& s, Ps& ... ps)
        {
            return[skip = std::move(s), tup_parse = std::tuple{ std::move(ps)... }](Iter b, Iter e) mutable
            {
                return build_skip_tup<0>(b, e, skip, tup_parse);
            };
        }
        template <class Ret, class Skip, class... Ps>
        static auto typed_parse(Skip&, Ps& ... ps)
        {
            static_assert(sizeof...(ps) < 0, "Parser only contains skippers. To force a type use emit");
        }
    };

    //  All real parsers are in the same category
    template<size_t real_ps, class RPList>
    struct seq_category<1, real_ps, RPList>
        : seq_cat_same<real_ps, RPList>
    {};
}   //  namespace qdpeg::details

namespace qdpeg
{
    template<class Skip, class... Ps>
    auto seq_ws(Skip s, Ps... ps)
    {
        static_assert(is_skipper<Skip>(), "First parameter must be a skipper");
        static_assert(all_parsers<Ps...>(), "Non-parser passed as argument");

        //  Remove all skippers 
        using real_parsers = mp_remove_if<mp_list<Ps...>, is_skipper_t>;

        using isolate_elem = mp_transform<isolate_element, real_parsers>;
        using isolate_uniq  = mp_unique<isolate_elem>;

        using seq_cat = details::seq_category<
            mp_size<isolate_uniq>::value,
            mp_size<real_parsers>::value,
            real_parsers >;

        return seq_cat::untyped_parse(s, ps...);
    }   // seq_ws

    template<class RetType, class Skip, class... Ps>
    auto seq_ws(Skip s, Ps... ps)
    {
        static_assert(is_skipper<Skip>(), "First parameter must be a skipper");
        static_assert(all_parsers<Ps...>(), "Non-parser passed as argument");

        //  Remove all skippers 
        using real_parsers = mp_remove_if<mp_list<Ps...>, is_skipper_t>;

        using isolate_elem = mp_transform<isolate_element, real_parsers>;
        using isolate_uniq  = mp_unique<isolate_elem>;

        using seq_cat = details::seq_category<
            mp_size<isolate_uniq>::value,
            mp_size<real_parsers>::value,
            real_parsers >;

        return seq_cat::template typed_parse<RetType>(s, ps...);
    }   // seq_ws

    template<class... Ps>
    auto seq(Ps... ps)
    {
        static_assert(all_parsers<Ps...>(), "Non-parser passed as argument");
        return seq_ws(empty, std::move(ps)...);
    }

    template<class RetType, class... Ps>
    auto seq(Ps... ps)
    {
        return seq_ws<RetType>(empty, std::move(ps)...);
    }
}   //  namespace qdpeg
