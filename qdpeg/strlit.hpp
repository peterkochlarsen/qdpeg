#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

constexpr inline char to_lower(char c)
{
    return c >= 'A' && c <= 'Z'
        ? c + 'a' - 'A'
        : c;
}

struct str_lit
{
    str_lit& operator= (str_lit const& rhs) = default;

    template<size_t NN>
    constexpr str_lit(char const (&d)[NN])
       : data(d)
       , N(NN)
    {}
    constexpr char operator[](int i) const noexcept { return data[i]; }
    constexpr operator std::string_view() const { return {data,size()}; }
    constexpr size_t size() const noexcept { return N - 1;}
    constexpr char const* begin() const noexcept { return data; }
    constexpr char const* end() const noexcept { return data + size(); }
private:
    char const * data;
    size_t  N;
};

constexpr inline size_t size(str_lit const& s) noexcept { return s.size(); }

constexpr bool ic_less(char a,char b) noexcept 
{
    return to_lower(a) < to_lower(b);
}

constexpr bool ic_eq(char a,char b)
{
    return to_lower(a) == to_lower(b);
}

struct ci_strlit
    : str_lit
{
    ci_strlit& operator= (ci_strlit const& rhs) noexcept = default;
    using base = str_lit;
    template<size_t NN>
    constexpr ci_strlit(char const (&d)[NN]) noexcept 
       : base(d)
    {}
    using base::begin;
    using base::end;
    bool lt(std::string_view rhs) const noexcept 
    { 
        return std::lexicographical_compare(begin(),end(),rhs.begin(),rhs.end(),ic_less);
    }
    bool gt(std::string_view rhs) const noexcept 
    { 
        return std::lexicographical_compare(rhs.begin(),rhs.end(),begin(),end(),ic_less);
    }
    bool eq(std::string_view rhs) const noexcept 
    { 
        return std::equal(rhs.begin(),rhs.end(),begin(),end(),ic_eq);
    }
};

inline bool operator < (str_lit const& sl,str_lit const& o) { return std::string_view(sl) <  o; }
inline bool operator >=(str_lit const& sl,str_lit const& o) { return std::string_view(sl) >= o; }
inline bool operator ==(str_lit const& sl,str_lit const& o) { return std::string_view(sl) == o; }
inline bool operator !=(str_lit const& sl,str_lit const& o) { return std::string_view(sl) != o; }
inline bool operator > (str_lit const& sl,str_lit const& o) { return std::string_view(sl) >  o; }
inline bool operator <=(str_lit const& sl,str_lit const& o) { return std::string_view(sl) <= o; }

template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator < (str_lit const& sl,O const& o) { return std::string_view(sl) <  o; }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator >=(str_lit const& sl,O const& o) { return std::string_view(sl) >= o; }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator ==(str_lit const& sl,O const& o) { return std::string_view(sl) == o; }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator !=(str_lit const& sl,O const& o) { return std::string_view(sl) != o; }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator > (str_lit const& sl,O const& o) { return std::string_view(sl) >  o; }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator <=(str_lit const& sl,O const& o) { return std::string_view(sl) <= o; }

template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator < (O const& o,str_lit const& sl) { return o <  std::string_view(sl); }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator >=(O const& o,str_lit const& sl) { return o >= std::string_view(sl); }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator ==(O const& o,str_lit const& sl) { return o == std::string_view(sl); }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator !=(O const& o,str_lit const& sl) { return o != std::string_view(sl); }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator > (O const& o,str_lit const& sl) { return o >  std::string_view(sl); }
template<class O> constexpr std::enable_if_t<!std::is_same_v<O,ci_strlit>,bool> operator <=(O const& o,str_lit const& sl) { return o <= std::string_view(sl); }

inline bool operator < (ci_strlit const& sl,ci_strlit const& o) { return sl.lt(o); }
inline bool operator >=(ci_strlit const& sl,ci_strlit const& o) { return !sl.lt(o); }
inline bool operator ==(ci_strlit const& sl,ci_strlit const& o) { return sl.eq(o); }
inline bool operator !=(ci_strlit const& sl,ci_strlit const& o) { return !sl.eq(o); }
inline bool operator > (ci_strlit const& sl,ci_strlit const& o) { return sl.gt(o); }
inline bool operator <=(ci_strlit const& sl,ci_strlit const& o) { return !sl.gt(o); }

template<class O> constexpr bool operator < (ci_strlit const& sl,O const& o) { return sl.lt(o); }
template<class O> constexpr bool operator >=(ci_strlit const& sl,O const& o) { return !sl.lt(o); }
template<class O> constexpr bool operator ==(ci_strlit const& sl,O const& o) { return sl.eq(o); }
template<class O> constexpr bool operator !=(ci_strlit const& sl,O const& o) { return !sl.eq(o); }
template<class O> constexpr bool operator > (ci_strlit const& sl,O const& o) { return sl.gt(o); }
template<class O> constexpr bool operator <=(ci_strlit const& sl,O const& o) { return !sl.gt(o); }

template<class O> constexpr bool operator < (O const& o,ci_strlit const& sl) { return sl.gt(o); }
template<class O> constexpr bool operator >=(O const& o,ci_strlit const& sl) { return !sl.gt(o); }
template<class O> constexpr bool operator ==(O const& o,ci_strlit const& sl) { return sl.eq(o); }
template<class O> constexpr bool operator !=(O const& o,ci_strlit const& sl) { return !sl.eq(o); }
template<class O> constexpr bool operator > (O const& o,ci_strlit const& sl) { return sl.lt(o); }
template<class O> constexpr bool operator <=(O const& o,ci_strlit const& sl) { return !sl.lt(o); }
