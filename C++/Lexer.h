#ifndef LEXER_H
#define LEXER_H

#include "Regexp.h"
#include <iostream>

// Class for storing a sequence of regexps as a large NFA with multiple acceptances in order to perform efficient lexical analysis
// with token stream as output
class Lexer : public BaseRegexp{
private:
    //Acceptances represented by a table indexed by NFA state numbers. 
    //Contents represent the number of the regexp accepted by each state. -1 means no accept. 
    int* acceptTable;
    //Specifies the number of the one regex in the lexer whose output token is to be ignored
    int newlineToken;
    int isAccepting(int state);

public:
    //Constructor takes array of regexps and builds NFA.
    Lexer(char* regexplist[], int len, int newlineToken);
    //Performs lexical analysis by processing the next token in the string and returns pointer to the char after the end of the token
    char* Lexer::lex(char* input);
    ~Lexer();
    //Resets token variables
    void reset();
    //Checks if currently parsed token is valid via tokenID
    bool good();
    friend std::ostream& operator<<(std::ostream& os, const Lexer& regexp);

    //The line number, column number, and the regexp number of the current token
    int tokenLine = 1;
    int tokenCol = 1;
    int tokenID = -1;
};

#endif