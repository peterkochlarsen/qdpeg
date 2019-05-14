#pragma once    
#include <utility>
namespace cpp20
{
    template<class Container>
    constexpr auto ssize(Container const& c)
    {
        using signed_type = std::conditional_t<
            sizeof(std::size_t) == sizeof(long),long,int>;
        return static_cast<signed_type>(size(c));
    }

    struct nonesuch {};

    namespace details 
    {
        template<class Default, class AlwaysVoid,
                 template<class...> class Op, class... Args>
        struct detector 
        {
            using value_t = std::false_type;
            using type = Default;
        };
 
        template <class Default, template<class...> class Op, class... Args>
        struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> 
        {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };
    } // namespace details
 
    template <template<class...> class Op, class... Args>
    using is_detected = typename details::detector
        <nonesuch, void, Op, Args...>::value_t;
    
    template <template<class...> class Op, class... Args>
    using detected_t = typename details::detector
        <nonesuch, void, Op, Args...>::type;
 
    template <template<class...> class Op, class... Args>
    using detected_v = typename details::detector
        <nonesuch, void, Op, Args...>::value_t;
 
    template <class Default, template<class...> class Op, class... Args>
    using detected_or = details::detector<Default, void, Op, Args...>;
}   // cpp20
