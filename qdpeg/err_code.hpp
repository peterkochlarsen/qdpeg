#pragma once

namespace qdpeg 
{
    enum class Error_code
    {
        no_error,

        unknown_error,
        unexpected_eof,
        unexpected_char,
        
        expected_eof,
        expected_char,
        expected_string,
        expected_type,
        expected_end_of_line,
        
        symbol_not_found,
        symbol_not_expected,
        always_fail,
        
        to_few,
        underflow,
        overflow
    };
};
