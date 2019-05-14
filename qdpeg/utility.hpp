#pragma once

namespace qdpeg
{
    template<class... Pred>
    auto when_any(Pred... pred)
    {
        return [=](auto&& v)
        {
            return (pred(v) || ...);
        };
    }

    template<class... Pred>
    auto when_all(Pred... pred)
    {
        return [=](auto&& v)
        {
            return (pred(v) && ...);
        };
    }

    template<class... T>
    auto equals(T... t)
    {
        return [=](auto const& v)
        {
            return ((v == t) || ...);
        };
    }

    template<class T>
    auto negate(T t)
    {
        return [=](auto const& v)
        {
            return !(t(v));
        };
    }

    template<class... T>
    auto not_equals(T... t)
    {
        return [=](auto const& v)
        {
            return ((v != t) && ...);
        };
    }

    template<class T>
    auto in_range(T lo,T hi)
    {
        return [=](auto const& v)
        {
            return (v >= lo && v <= hi);
        };
    }
}   //  namespace qdpeg
