#ifndef REGEXP_H
#define REGEXP_H

#include <vector>
#include <cstdlib>
#include <bitset>
#include <exception>
#include <iostream>

// Number of ASCII character possible. Regexp alphabet size.
const int NumOfChars = 128;

// Namespace containing definitions exclusive to the Regexp module
namespace regexp{    
    // Represents states of an NFA. Edge value of -1 means it doesn't exist; -2 means a dangling edge.
    struct State{
        // Bitset deciding the characters used as transition for the state. 0 is no 1 is yes.  
        std::bitset<NumOfChars> transitions;
        // The destination of the edge that uses characters as transition. Defaults to -2.
        int edge;
        // Epsilon edge dedicated to concatenations
        int epsilon1;
        // Epsilon edge dedicated to unary operations
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
        char next(char low, char high);
        void error();
        //Many of the functions below will "return" a regexp fragment by updating reference arguments
        //NFA construction helper functions
        void updateFragment(std::bitset<NumOfChars>& transitions, char lower, char upper);
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
        void parseElem(std::bitset<NumOfChars>& transition);
        void parseSpecial(std::bitset<NumOfChars>& transition);
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
    int search(char* &str);
    //Constructors. Builds NFA
    Regexp(char* re);
    Regexp();
    friend std::ostream& operator<<(std::ostream& os, const Regexp& regexp);
};

#endif