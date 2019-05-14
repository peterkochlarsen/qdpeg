#pragma once
#include <qdpeg/err_code.hpp>
#include <stdexcept>
#include <string_view>
#include <qdpeg/cpp20.hpp>
#include <qdpeg/err_code.hpp>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>


struct Ascii_parse
{
    struct Bad_parse_access: std::logic_error
    {
        Bad_parse_access()
            : std::logic_error("Bad Parse_result access")
        {}
    };

    using Iter = std::string_view::iterator;
    [[noreturn]] static inline void bad_parse_access()  
    {
        throw Bad_parse_access {};
    }
    using Raw = std::string_view;
    static constexpr inline Raw make_raw(Iter b,Iter e) 
    { 
        if (b == e) return std::string_view();
        return std::string_view(&*b,static_cast<size_t>(e - b)); 
    }
};

namespace qdpeg
{
    [[noreturn]] inline void bad_parse_access() { Ascii_parse::bad_parse_access(); };
    using Iter = Ascii_parse::Iter;
    using Raw = Ascii_parse::Raw;
    inline Raw make_raw(Iter begin,Iter end) { return Ascii_parse::make_raw(begin,end); }

    constexpr Iter take(Iter b,Iter e,std::ptrdiff_t N) noexcept
    {
        return N <= 0
            ? b
            : (e - b > N
                ? b + N
                : e
            );
    }

    template<class T>
    struct Parse_result;
}   //  qdpeg


namespace qdpeg
{
    struct Nothing {};
    struct Strongly_typed {};
    struct Weakly_typed {};
}   //  qdpeg


namespace qdpeg
{
    template<class T>
    struct Parse_result;

    using Skipper = Parse_result<Nothing>;

    template<class T>
    struct Parse_result
    {
        using result_type = T;
    public:
        ~Parse_result() noexcept { disengage(); }
        constexpr Parse_result()
            : Parse_result(std::string_view("").begin(),Error_code::unknown_error)
        {}
        constexpr Parse_result(Parse_result const& rhs);
        constexpr Parse_result(Parse_result&& rhs) noexcept;
        constexpr Parse_result& operator=(Parse_result const& rhs);
        constexpr Parse_result& operator=(Parse_result&& rhs) noexcept;
        constexpr Parse_result(Iter i,T v)
            : iter(i)
        {
            new (&data.val) T(std::move(v));
            engaged = true;
        }
        constexpr Parse_result(Iter i,Error_code ec)
            : iter(i)
            
        {
            engaged = false;
            new (&data.err_code) Error_code(ec);
        }
    
        constexpr Parse_result(Iter i) noexcept
            : iter(i)
        {
            if constexpr (std::is_same_v<T,Nothing>)
            {
                new (&data.val) Nothing;
                engaged = true;
            }
            else
            {
                Error_code ec {Error_code::unknown_error};
                data.err_code = ec;
                engaged = false;
            }
        }
    public:
        operator Skipper() const noexcept   
        { 
            return engaged 
                ? Skipper { iter, Nothing {} }
                : Skipper { iter, error() };
        }
        constexpr operator Iter() const  noexcept     { return iter; }
        constexpr operator bool() const  noexcept     { return engaged; }
        constexpr bool operator!() const noexcept     { return !engaged; }
        constexpr auto error() const noexcept         { return engaged ? Error_code::no_error : data.err_code; }
        constexpr T const& value() const noexcept     { if (!engaged) bad_parse_access(); return data.val; }
        constexpr T& value()
        { 
            if (!engaged) bad_parse_access();
            return data.val;
        }
    private:
        constexpr void disengage() noexcept;
        constexpr void assign(Parse_result&& rhs);
    public:
        bool engaged = false;
        Iter iter;
        union Data
        {
            Error_code  err_code;
            T           val;
            Data(): err_code(Error_code::no_error) {}
            ~Data() {}
        };
        Data data;
    };  //  Parse_result

    using parse_status = Parse_result<Nothing>;

    template<class T>
    constexpr void Parse_result<T>::disengage() noexcept
    {
        if (engaged)
            data.val.~T();
    }

    template<class T>
    constexpr void Parse_result<T>::assign(Parse_result&& rhs)
    {
        bool rhs_engaged = rhs;
        iter = rhs.iter;
        if (rhs_engaged)
        {
            new(&data.val) T(std::move(rhs.data.val)); 
        }
        else
        {
            data.err_code = rhs.data.err_code;
        }
        engaged = rhs_engaged;
    }

    template<class T>
    constexpr Parse_result<T>::Parse_result(Parse_result&& rhs)   noexcept
    {
        assign(std::move(rhs));
    }

    template<class T>
    constexpr Parse_result<T>::Parse_result(Parse_result const& rhs)
    {
        iter = rhs.iter;
        if (rhs.engaged)
        {
            new(&data.val) T(rhs.data.val); 
        }
        else
        {
            new(&data.err_code) Error_code (rhs.data.err_code); 
        }
        engaged = rhs.engaged;
    }

    template<class T>
    constexpr Parse_result<T>& Parse_result<T>::operator=(Parse_result&& rhs) noexcept
    {
        disengage();
        assign(std::move(rhs));
        return *this;
    }

    template<class T>
    constexpr Parse_result<T>& Parse_result<T>::operator=(Parse_result const& rhs)
    {
        Parse_result tmp { rhs };
        assign(std::move(tmp));
        return *this;
    }
}   //  qdpeg

namespace qdpeg
{
    template<class Parser,class E = void>
    struct parser_info_impl
        : std::false_type
    {};

    template<class T>
    struct parser_info_impl<T,
        std::void_t<
        decltype(std::declval<T>()
            (std::declval<Iter>(),
                std::declval<Iter>()
                ))>>
        : std::true_type
    {
        using return_type = 
            decltype(std::declval<T>()
                (std::declval<Iter>(),
                std::declval<Iter>())
            );
        using result_type = typename return_type::result_type;
    };

    template<class T>
    constexpr bool is_parser()          { return parser_info_impl<T>{}; }
    
    template<class T>
    constexpr bool is_parser(T const&)  { return parser_info_impl<T>{}; }

    template<class T>
    using Parsed_return = typename parser_info_impl<T>::return_type; 
    template<class T>
    using Parsed_type = typename parser_info_impl<T>::result_type; 
    
    template<class T,class E = void>
    struct is_skipper_t: std::false_type {};
    template<class T>
    struct is_skipper_t<T,
        std::void_t<Parsed_type<T>>>
        : std::is_same<Parsed_type<T>,Nothing>
    {};

    template<class T>
    constexpr bool is_skipper()   { return is_skipper_t<T>(); }
    template<class T>
    constexpr bool is_skipper(T const&) { return is_skipper_t<T>(); }

    template<class T>
    constexpr bool is_real_parser() 
    { 
        return is_parser<T>() && !is_skipper<T>(); 
    }
    
    template<class T>
    constexpr bool is_real_parser(T const&)
    { 
        return is_real_parser<T>(); 
    }

    template<class T>
    constexpr bool is_typed()  
    { 
        return !std::is_base_of<Weakly_typed,T>();
    }

    template<class... Ps>
    constexpr bool all_parsers()
    {
        return std::conjunction<parser_info_impl<Ps>...>();
    }

    template<class... Ps>
    constexpr bool is_tuple_of_parsers()
    {
        return all_parsers<Ps...>();
    }

    template<class PRT,class E = void> 
    struct parse_class_info
    {
        using element_type  = Parsed_type<PRT>;
        using value_type  = Parsed_type<PRT>;
        using single = std::true_type;
    };

    template<class PRT> 
    struct parse_class_info<PRT,
        std::enable_if_t<
            !is_typed<PRT>()
        >>
    {
        using value_type  = Parsed_type<PRT>;
        using element_type = typename PRT::element_type;
        using single = std::false_type;
    };

    template<class T>
    using isolate_element = typename parse_class_info<T>::element_type;

    template<class T>
    using isolate_value = typename parse_class_info<T>::value_type;

    template<class P,class Adder>
    using is_extended_parser_t = decltype(
        std::declval<P&>()
            (std::declval<Iter>(),
             std::declval<Iter>(),
              std::declval<Adder&>()));
   
    template<class P,class Adder>
    inline constexpr bool is_extended_parser = cpp20::is_detected<is_extended_parser_t,P,Adder>();
}   //  namespace qdpeg

namespace qdpeg
{
    template<class Func>
    auto do_parse(std::string_view sv,Func f)
    {
        static_assert(is_parser(f));
        auto begin = std::begin(sv);
        auto end = std::end(sv);
        std::cout << "Parsing [" << sv 
            << "] with object of size " << sizeof (f) << ":\n";

        auto result = f(begin,end);
        std::string_view::iterator new_begin = result.iter;
        if (result)
        {
            if (result.iter == end)
                std::cout << "Parsed fully\n";
            else
                std::cout << "Partial result: missing \""
                          << std::string(new_begin,end)
                          << "\"\n";
        }
        else
        {
            std::cout << "Parse fails with result \"" << static_cast<int>(result.error()) << "\" at \""
                        << std::string(new_begin,end)
                          << "\"\n";
        }
        return result;
    }
}   //  qdpeg

//
//
//
namespace qdpeg
{
    struct Empty_skipper
    {
        inline Skipper operator()(Iter b,Iter ) const { return { b, Nothing{} };}
    };
    static Empty_skipper empty;

    inline Skipper eof(Iter b,Iter e)
    {
        return b == e
            ? Skipper { b, Nothing{} }
        : Skipper { b, Error_code::expected_eof }; 
    }

    inline Skipper eol(Iter b,Iter e)
    {
        if (b == e)
            return b;
        if (*b == '\n')
        {
            if (b + 1 == e || b[1] != '\r')
                return b + 1;
            return b + 2;
        }
        if (*b == '\r')
        {
            if (b + 1 == e || b[1] != '\n')
                return b + 1;
            return b + 2;
        }
        return { b, Error_code::expected_end_of_line };
    }

    inline auto fail(Iter b,Iter ) -> Skipper
    {
        return Skipper { b, Error_code::always_fail }; 
    }
    template<class T>
    inline auto fail_as(Iter b,Iter ) -> Parse_result<T>
    {
        return { b, Error_code::always_fail }; 
    }

}   //  qdpeg
