#pragma once
#include <qdpeg/aux_parser.hpp>
#include <qdpeg/choice.hpp>
#include <qdpeg/seq.hpp>
#include <qdpeg/repeat.hpp>
#include <qdpeg/symbol.hpp>
#include <qdpeg/whitespace.hpp>
#include <charconv>
#include <limits>

namespace qdpeg
{
    inline auto parse_bool(Iter b,Iter e)
        -> Parse_result<bool>
    {
        return choice(
            as(lit('0'),false),
            as(lit('1'),true),
            as(lit(ci_strlit("false")),false),
            as(lit(ci_strlit("true")),true)
        )(b,e);
    };

    //  signs for a number
    enum class sign_policy
    {
        none,       //  Do not accept a sign
        minus,      //  Accept a minus
        allowed,    //  accept '-','+'
        required    //  Require '-','+'
    };

    namespace details
    {
        //  Get the sign-parser dependent on the policy
        template<sign_policy sp>
        constexpr auto parse_sign()
        {
            if constexpr (sp == sign_policy::none)
            {
                return empty;
            }
            else if constexpr (sp == sign_policy::minus)
            {
                return choice(lit('-'),empty);
            }
            else if constexpr (sp == sign_policy::allowed)
            {
                return choice(lit('-'),lit('+'),empty);
            }
            else //(sp == sign_policy::required)
            {
                return choice(lit('-'),lit('+'));
            }
        }
    }
    
    template<signed char N>
    bool is_radix_char(char ch)
    {
        static_assert(N >= 2 && N <= 36,"Unsupported radix");

        if constexpr (N <= 10) 
        {
            return ch >= '0' && ch < '0' + N;
        }
        else 
        {
            return (ch >= '0' && ch < '0' + 10)
                || (ch >= 'a' && ch < 'a' + N - 10)
                || (ch >= 'A' && ch < 'A' + N - 10);
        }
    }

    template <class T,
        unsigned Radix = 10,
        sign_policy sp = std::is_signed<T>()
            ? sign_policy::allowed
            : sign_policy::none,
        int MinDigits = 1,
        int MaxDigits = std::numeric_limits<int>::max()>
    auto int_parser(Iter b,Iter e)
            -> Parse_result<T>
    {
        static_assert(MinDigits > 0 && MinDigits <= MaxDigits);
        Iter start_pos = b;
        auto parse_digits = 
            raw(seq(
                parse_sign<sp>(),
                repeat(char_if(is_radix_char<Radix>),
                       take(MinDigits,MaxDigits))
            ));

        auto raw_int = parse_digits(b,e);
        if (!raw_int)
            return {raw_int.iter, raw_int.error() };

        auto digs = raw_int.value();

        if constexpr (sp == sign_policy::allowed 
                || sp == sign_policy::required)
        {
            if (digs[0] == '+') 
                digs.remove_prefix(1);
        }
        
        T val;
        auto conv_res = std::from_chars(digs.data(),
            digs.data() + digs.size(),
            val,
            Radix);
        if (conv_res.ec == std::errc::result_out_of_range 
            || conv_res.ec == std::errc::invalid_argument)
        {
            return { raw_int.iter, Error_code::overflow };
        }

        return { raw_int.iter, val };
    }
}   //  qdpeg

namespace qdpeg
{
    //  Exponent for a floating point number
    enum class exp_policy
    {
        none,       //  Do not accept
        allowed,    //  Allow
        required    //  Require
    };

    enum class decpoint_policy
    {
        allow_point,    //  allow
        require_point,  //  Require
    };

    enum class inf_nan_policy 
    {
        none,
        allowed
    };

    namespace details
    {
        template<decpoint_policy dec_p,
                 int decimals_min,
                 int decimals_max>
            auto parse_decimals()
        {
            if constexpr (dec_p == decpoint_policy::require_point
                          || decimals_min > 0)
            {
                return seq(char_from<'.'>,
                           repeat(digit(),take(decimals_min,decimals_max))
                );
            }
            else 
            {
                return choice(
                    seq(char_from<'.'>,
                        repeat(digit(),take(decimals_min,decimals_max))
                    ),
                    empty);
            }
        }
        
        enum class Inf_nan
        {
            minus_inf,
            inf,
            nan
        };

        template<class Real>
        Real inf_nan_to_real(Inf_nan inf_nan)
        {
            using num_lim = std::numeric_limits<Real>;
            if (inf_nan == Inf_nan::nan)
            {
                if constexpr (num_lim::has_quiet_NaN)
                {
                    return num_lim::quiet_NaN();
                }
                else
                {
                    return num_lim::denormal();
                }
            }
            else
            {
                if constexpr (num_lim::has_infinity)
                {
                    return inf_nan == Inf_nan::inf
                        ?  num_lim::infinity()
                        : -num_lim::infinity();
                }
                else
                {
                    return inf_nan == Inf_nan::inf
                        ?  num_lim::max()
                        : -num_lim::max();
                }
            }
        }

        template<class Real>
        inline auto do_parse_inf_nan(Iter b,Iter e)
            -> Parse_result<Real>
        {
            static const Symbol_ci<Inf_nan> inf_nan_symbols
            {
                { "nan", Inf_nan::nan },
                { "inf", Inf_nan::inf },
                {"+nan", Inf_nan::nan },
                {"+inf", Inf_nan::inf },
                {"-nan", Inf_nan::nan },
                {"-inf", Inf_nan::minus_inf },
            };

            return as(inf_nan_symbols,inf_nan_to_real<Real>)(b,e);
        }

        template<class Real,
            sign_policy ,
            inf_nan_policy  inp>
        auto parse_inf_nan()
        {
            if constexpr (inp == inf_nan_policy::none)
            {
                return fail_as<Real>;
            }
            else
            {
                return do_parse_inf_nan<Real>;
            }
        }

        auto parse_full_exponent()
        {
            return seq(ci_lit("e"),
                choice(lit('-'),lit('+'),empty),
                repeat(digit(),at_least(1)));
        }

        template<exp_policy ep>
        auto parse_exponent()
        {
            if constexpr (ep == exp_policy::none)
            {
                return empty;
            }
            else if constexpr (ep == exp_policy::allowed)
            {
                return choice(parse_full_exponent(),empty);
            }
            else // (ep == exp_policy::required)
            {
                return parse_full_exponent();
            }
        }

        //  e.g. "." which is not a good real number
        template<sign_policy sp>
        auto non_digit_real(Iter b,Iter e)
        {
            return seq(parse_sign<sp>(),
                lit('.'),
                not_p(char_if(is_digit))
            )(b,e);
        }

        template<sign_policy sp,
            decpoint_policy dec_p,
            exp_policy exp_p,
            int  decimals_min,
            int  digits_min,
            int  decimals_max,
            int  digits_max>
        auto real_parser_image(Iter b,Iter e) -> Parse_result<Raw>
        {
            //  parse sign,digits,decimals, exponent
            return raw(seq(
                details::parse_sign<sp>(),
                repeat(digit(),take(digits_min,digits_max)),
                details::parse_decimals<dec_p,decimals_min,decimals_max>(),
                details::parse_exponent<exp_p>()
            ))(b,e);
        }

        template<class Real,
            sign_policy sp,
            decpoint_policy dec_p,
            exp_policy ep,
            int  decimals_min,
            int  digits_min,
            int  decimals_max,
            int  digits_max>
        auto parse_actual_real(Iter b,Iter e) -> Parse_result<Real>
        {
            static_assert(digits_min >= 0 && digits_min <= digits_max,
                "Invalid digit specification for real_parser");
            static_assert(decimals_min >= 0 
                          && decimals_min <= decimals_max,
                "Invalid decimals specification for real_parser");

            auto raw_result = not_p(details::non_digit_real<sp>,
                details::real_parser_image<sp,
                dec_p,
                ep,
                decimals_min,
                digits_min,
                decimals_max,
                digits_max>)(b,e);

            if (!raw_result)
                return { raw_result.iter,raw_result.error() };
        
            Real val;
            if constexpr (sp == sign_policy::allowed
                          || sp == sign_policy::required)
            {
                if (*b == '+') ++b;
            }
            auto conv_res = std::from_chars(
                &*b,
                &*b + raw_result.value().size(),
                val);
            if (conv_res.ec == std::errc::result_out_of_range)
                return { raw_result.iter, Error_code::overflow };

            return { raw_result.iter, val };
        }
    }

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
    {
        return choice(
            details::parse_inf_nan<Real,sp,inf_nan>(),
            details::parse_actual_real<
                Real,sp,dec_p,ep,decimals_min,digits_min,
                decimals_max,digits_max>)(b,e);
    }

}
