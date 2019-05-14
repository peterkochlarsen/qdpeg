#pragma once
#include <qdpeg/qdbase.hpp>
#include <algorithm>
#include <vector>

namespace qdpeg
{
    namespace details
    {
        template<class K,class V>
        struct Symbol_element
        {
            friend bool operator<(
                Symbol_element const& lhs,
                std::string_view rhs)
            {
                return lhs.name < rhs;
            }
            friend bool operator<(
                std::string_view lhs,
                Symbol_element const& rhs)
            {
                return lhs < rhs.name;
            }
            bool operator<(Symbol_element const& rhs) const
            {
                return name < rhs;
            }
            K       name;
            V       value;
        };

        struct Compare_one
        {
            Compare_one(int p,char c)
                : pos(p), ch(c)
            {}
            int     pos;
            char    ch;
        };

        template<class K,class V>
        struct Symbol_x
        {
            using Siter = typename std::vector<Symbol_element<K,V>>::const_iterator;
            using Rng = std::pair<Siter,Siter>;

            Symbol_x(std::initializer_list<Symbol_element<K,V>>  elements)
                : sv(elements)
            {
                std::sort(sv.begin(),sv.end(),[](auto& a,auto& b)
                {
                    return a.name < b.name;
                });
            }

            auto operator()(Iter b,Iter e) const -> Parse_result<V>
            {
                auto res = find_elem(b,e);
                if (res == sv.end())
                {
                    return { b, Error_code::symbol_not_found };
                }
                else
                {
                    return { b + cpp20::ssize(res->name), res->value };
                }
            }
        private:
            auto find_elem(Iter b,Iter e) const
            {
                Siter end = sv.end(); 
                if (b == e) 
                    return end;

                Siter s_begin = std::lower_bound(sv.begin(),end,make_raw(b,b + 1));
                std::string_view ttp { make_raw(b,e) };
                Siter s_end = std::upper_bound(s_begin,end,ttp);

                for (;;)
                {
                    if (s_end == s_begin) return end;
                    --s_end;
                    if (s_end->name == ttp.substr(0,s_end->name.size()))
                        return s_end;
                };
            }
        private:
            std::vector<Symbol_element<K,V>> sv;
        };
    }   //  namespace details

    template<class V>
    using Symbol_ci = details::Symbol_x<ci_strlit,V>;
    template<class V>
    using Symbol    = details::Symbol_x<str_lit,V>;
}   //  namespace qdpeg
