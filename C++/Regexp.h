#ifndef REGEXP_H
#define REGEXP_H

#include <vector>
#include <cstdlib>
#include <bitset>
#include <exception>
#include <iostream>

// Namespace containing definitions exclusive to the Regexp module
namespace regexp{
    // Number of ASCII character possible. Regexp alphabet size.
    const int NumOfChars = 256;
    // Represents states of an NFA. Edge value of -1 means it doesn't exist; -2 means a dangling edge.
    struct State{
        // Bitset deciding the characters used as transition for the state. 0 is no 1 is yes.  
        std::bitset<regexp::NumOfChars> transitions;
        // The destination of the edge that uses characters as transition. Defaults to -2.
        int edge;
        // 2 epsilon edges. Defaults to -1.
        int epsilon1;
        int epsilon2;
    };
}

// Custom exception for regexp parsing syntax errors
class RegexSyntaxError : public std::exception{
    char *str;
public:
    // Error constructed with position in string as input    
    RegexSyntaxError(int pos);
    const char *what();
};

#endif