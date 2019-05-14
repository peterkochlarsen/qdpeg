#pragma once

template<class...> struct [[deprecated]] print_type
{};

template<template<class...> class C,class... Ts>
struct print_tup_type
    : print_type<Ts...>
{};

