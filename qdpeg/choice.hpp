#pragma once
#include <qdpeg/qdbase.hpp>
#include <boost/mp11.hpp>
#include <tuple>
#include <variant>

namespace qdpeg::details
{
    template <size_t I,class Result,class ParseTup>
    constexpr auto build_result(Iter b,Iter e,ParseTup& ptup) 
        -> Parse_result<Result> 
    {
        using Parser = std::tuple_element_t<I, ParseTup>;
        auto& p = std::get<I>(ptup);
        using my_result = Parse_result<Parser>;

        auto this_res = p(b, e);
        
        if (this_res)
        {
            if constexpr (std::is_same_v<Result,my_result>)
            {
                return this_res;
            }
            else
            {
                return { this_res.iter, Result{ std::move(this_res.value()) } };
            }
        }
        else
        {
            if constexpr (I == std::tuple_size_v<ParseTup> - 1)
            {
                return { this_res.iter, this_res.error() };
            }
            else
            {
                auto alt_res = build_result<I + 1,Result>(b,e,ptup);
                if (alt_res || alt_res.iter > this_res.iter) 
                    return alt_res;
                return  { this_res.iter, this_res.error() };
            }
        }
    };
}   //  namespace qdpeg::details

namespace qdpeg
{
    //  Parse one of a number of types using Result as the resulttype.
    template<class Result,class... Ps>
    constexpr auto choice(Ps ... ps)
    {
        using namespace boost::mp11;
        using my_types = mp_unique<mp_list<Parsed_type<Ps>...>>;

        //  We do not want to mix skippers and real parsers
        using no_skippers = boost::mp11::mp_none_of<my_types,is_skipper_t>;
        using all_skippers = boost::mp11::mp_all_of<my_types,is_skipper_t>;
        static_assert(no_skippers() || all_skippers(),"Do not mix skippers with non-skippers");

        return [tup = std::tuple(ps...)](Iter b,Iter e) mutable
        {
            return details::build_result<0,Result>(b,e,tup);
        };
    }

    //  Parse one of a number of types using a std::variant as the result.
    template<class... Ps>
    constexpr auto choice(Ps ... ps)
    {
        using namespace boost::mp11;
        static_assert(sizeof...(Ps) > 0,"choice requires at least one parser");

        using my_types = mp_unique<mp_list<Parsed_type<Ps>...>>;
        if constexpr (mp_size<my_types>() == 1)
        {
            //  Only one type => decay variant to its first and only type
            using my_result = mp_front<my_types>;
            return choice<my_result,Ps...>(ps...);
        }
        else
        {
            using my_result = mp_rename<my_types,std::variant>;
            return choice<my_result,Ps...>(ps...);
        }
    }
}   //  namespace qdpeg
