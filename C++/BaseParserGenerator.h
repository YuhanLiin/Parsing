#ifndef BASEPARSERGENERATOR_H
#define BASEPARSERGENERATOR_H

#include "Lexer.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <exception>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Parser Generator converts grammar configuration strings into a concise internal representation on which a parser can be built.
// Configuration consists of an optional Token Declaration section and a Grammar section. Newlines and spaces are ignored.

// Token Declaration is a list of space separated token names inside {} brackets. 
// Names are mapped to the tokens produced by the regexp parameters in order.
// Use * to map to a regexp whose tokens will be ignored by the parser.
// { SPACE INT STR ID * } maps to an array of 5 regexps, the last of which will produce ignored tokens.

// Grammar is in YACC form, consisting of Rules formed by an lhs symbol, colon, Productions, and semicolon.
// Productions are one or more lists of space-separated symbols. Each production is separated by a |
// Example Rule with 2 productions:   exp : exp '+' INT | INT ;
// Multiple rules with same lhs symbol cannot exist.

// Tokens/terminals are all UPPERCASE. Nonterminals are lowercase. 
// Individual characters can appear in rhs of rules in the form of 'x'.

// Numerical representation: All ASCII values above 1 are reserved for chars. Tokens numbers start from 128.
// Nonterminal numbers start from 128 + (# of tokens) + 1. 
// Start symbol will always be the lhs of the first rule in the grammar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Represents the tokens used to parse grammar strings
enum Gtoken {INVALID=-1, NEWLINE=0, SPACES, NTRML, TRML, LBRAC, RBRAC, CHR, COLON, PIPE, SCOLON, STAR};
//Represents current state of a given parse
enum ParseStatus {SYNTAXERROR, GOOD, DONE};

//Error raised when grammar configuration is syntactically wrong
class GrammarConfigError : public std::exception{
    char *str;
public:
    // Error constructed with position in string as input    
    GrammarConfigError(char *msg);
    const char *what();
};

//Base class for generating parsers. Handles conversion of grammar configuration strings into internal representation of the grammar
//Inherited classes will implement specific parsing algorithms
class BaseParserGenerator{
private:
    //Internal class handling resources for turning the grammar string into an integral representation
    class GrammarParser{
    public:
        // 0->newline  1->spaces  2->nonterminal  3->terminal  4->lbrac  5->rbrac  6->char  7->colon  8->pipe  9->scolon  10->star
        char *regexps[11] = {"\n", " +", "[a-z]+", "[A-Z]+", "{", "}", "'.'", ":", "\\|", ";", "\\*"};
        // Lexer used for parsing grammar string
        Lexer lexer{regexps, 11, 0};
        // Maps nonterminal and terminal symbols to their numerical representations
        std::unordered_map<std::string, int> symbolTable;
        // Tracks current and previous location in the grammar string so that words can be extracted
        char *curpos;
        char *prevpos;
        BaseParserGenerator *parser;
        //Icremented number of rules parsed so far, not productions. Truly initialized once all tokens have been parsed
        int ruleNum = -10000;
        //Incremented number of tokens parsed so far. Number starts from after all ASCII chars
        int tokenNum = NumOfChars;
        //Placeholder number for nonterminals not yet encountered on the lhs. Decrements as more are found
        int unfoundRuleNum = -1;

        GrammarParser(BaseParserGenerator *p, char *grammarConfig);
        //Parser helper functions
        const char* gtokenName(Gtoken gtoken);
        void error(Gtoken token);
        void error(char * message);
        void next();
        void next(Gtoken gtoken);
        bool tokenIs(Gtoken gtoken);
        void getWord(std::string &word);
        //Replaces all placeholders in the grammar with a specific number
        void replaceSymbol(int old, int replacement);
        //Returns the number that maps to the input symbol. Returns 0 if the symbol isn't in the table
        int symbolNumber(std::string &symbol);
        //Parsing functions
        void parseTokens();
        void parseRule();
        void parseProduction();
        void parseGrammar();
    };
    friend GrammarParser;

protected:
    //Total incremented # of tokens  used by the grammar. Initialized after parsing finishes. Max token index + 1
    int tokenNum;
    //Rule num is token num + the number of nonterminals. Max rule index + 1
    int ruleNum;
    // Grammar is a list of ints divided into segments representing production rules. 
    // The first int of each segment is the # of rhs symbols in the production, followed by each rhs symbol
    // Ex: a : A B C | B becomes  3, 129, 130, 131, 1, 130 
    std::vector<int> grammar;
    // A on/off array indicating which tokens will be ignored
    std::vector<char> tokenIgnore; 
    // Maps lhs symbol # to the position in the grammar where the production with that lhs symbol starts. Padded at the end to allow looping
    // Allows instant access of productions with any lhs symbol
    std::vector<int> ruleNumStart;

    char * curpos;
    char * prevpos;
    Lexer * lexptr;

    //Add the rhs symbols of a production to a stack
    void addProduction(int ruleStart, std::vector<int>& stack, bool reverse);
    //Go to starting pos of next production in grammar
    int nextProduction(int ruleStart);
    //Gets next token number/char. 0 means end of input
    int next();
    //Starting position in grammar of a given lhs symbol
    int ruleStart(int symbol);
    //Get contents of current token
    void getWord(std::string &word);
    //Switch between the non-incremented and incremented rule/nonterminal numbers
    int toRuleCount(int ruleNum);
    int toRuleNum (int ruleCount);
    //Checks if symbol is terminal
    bool isTerminal(int symbol);

public:
    BaseParserGenerator(char * grammarConfig, Lexer * lex);
    friend std::ostream& operator<<(std::ostream& os, BaseParserGenerator& parser);
    //Disable copying and assigning
    BaseParserGenerator(BaseParserGenerator&) = delete;
    BaseParserGenerator& operator=(BaseParserGenerator&) = delete;

    //These virtual methods, when implemented, will allow a parse to be conducted on a reduction-by-reduction basis and
    // allow semantic actions on the values associated with reduced symbols. Value for tokens will be std::string

    //Reset all internal variables and initiate parse on a new input. Begin the first reduction
    virtual ParseStatus parse(char *input) = 0;
    //Finish a pending reduction and associate the produced lhs symbol with the reduced value. Begin the next reduction
    //Primary means of advancing the parsing
    virtual ParseStatus reduce(void *reducedValue, bool toDelete) = 0;
    //Return unincremented number of the lhs symbol being reduced
    virtual int lhsNum() = 0;
    //Return which production of a rule is being reduced
    virtual int prodNum() = 0;
    //Return pointer to value of a specific rhs value being reduced (0-indexed)
    virtual void *rhsVal(int pos) = 0;
    
    //Returns number of current token, the expected token (returns -1 for non-shift errors), and the column/line numbers in case parse fails
    virtual int curToken() = 0;
    virtual int expectedToken() = 0;
    int lineNum();
    int colNum();
};

struct ParseValue{
public:
    void * ptr = NULL;
    bool toDelete = false;
};


#endif