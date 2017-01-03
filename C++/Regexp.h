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

// Base class for all Regexp based objects. Contains the meat of the Regexp engine, including the NFA
class BaseRegexp{
protected:
    // Regexp NFA, which is a list of states indexed by number
    std::vector< regexp::State* > nfa;
    // Index of starting state of NFA
    int starting;

    //Internal class containing the functionality for parsing a regex and constructing a NFA
    class RegexpBuilder{
    public:
        // Regular expression string
        char* regexp;
        // Current parser position in the string
        int pos = 0;
        // Pointer to the BaseRegexp object being constructed
        BaseRegexp *rePtr;

        //Parser helper functions
        bool notEnd();
        char next(char c);
        char next(int low, int high);
        void error();
        //NFA construction helper functions
        void updateFragment(std::bitset<regexp::NumOfChars>& transitions, int lower, int upper);
        int push();
        void alternate(int& startL, int& endL, int startR, int endR);
        void concatenate(int left, int right);
        void optional(int& start, int& end);
        void repeating(int& start, int& end);
        void kleene(int& start, int& end);
        //Recursive descent regexp parser and NFA builder
        void parseRegexp(int& startL, int& endL);
        void parseConcat(int& startL, int& endL);
        void parseUnary(int& start, int& end);
        void parseValue(int& start, int& end);
        char parseC();
        int parseChar();
        int parseSet();
        void parseElem(std::bitset<regexp::NumOfChars>& transition);
        void parseSpecial(std::bitset<regexp::NumOfChars>& transition);
        //Runs parser/builder
        void build(char* re, BaseRegexp *rp, int &start, int &accept);
    };

    //Functions for simulating the NFA through an input
    void addState(int state, std::vector<int>& curStates, int *listids, int id);
    virtual int isAccepting(int state) = 0;
    int simulate(char* &str);
    //Destructor
    ~BaseRegexp();
    friend RegexpBuilder;
    friend std::ostream& operator<<(std::ostream& os, const BaseRegexp& regexp);
};

// Class representing a single regexp. Included functions for matching and searching through strings
class Regexp: public BaseRegexp{
private:
    //Single accepting state of the regexp
    int accepting;
    int isAccepting(int state);

public:
    //Match and search a string for the constructed regexp by simulating NFA
    int match(char* str);
    int search(char* str);
    //Constructor. Builds NFA
    Regexp(char* re);
    friend std::ostream& operator<<(std::ostream& os, const Regexp& regexp);
};

// Representation of a lexical token
struct Token{
    // The numbering of the regex this token was matched to
    int id;
    // Position in the string the token started
    int start;
    // Position right after token ended
    int end;
};
std::ostream& operator<<(std::ostream& os, const Token& token);

// Class for storing a sequence of regexps as a large NFA with multiple acceptances in order to perform efficient lexical analysis
// with token stream as output
class Lexer : public BaseRegexp{
private:
    //Acceptances represented by a table indexed by NFA state numbers. 
    //Contents represent the number of the regexp accepted by each state. -1 means no accept. 
    int* acceptTable;
    //Specifies the number of the one regex in the lexer whose output token is to be ignored
    int ignore;

    int isAccepting(int state);

public:
    //Constructor takes array of regexps and builds NFA.
    Lexer(char* regexplist[], int len, int ignore);
    //Performs lexical analysis and returns success status and token list 
    bool lex(char* &curpos, std::vector<Token> &tokens);
    ~Lexer();
    friend std::ostream& operator<<(std::ostream& os, const Lexer& regexp);
};



#endif