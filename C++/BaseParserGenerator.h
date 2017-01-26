#ifndef BASEPARSERGENERATOR_H
#define BASEPARSERGENERATOR_H

#include "Lexer.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cstring>

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

// Numerical representation: All ASCII values above 1 are reserved for chars. Tokens numbers start from 128 + 1.
// Nonterminal numbers start from 128 + (# of tokens) + 1. 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Represents the tokens used to parse grammar strings
enum Gtoken {NEWLINE=0, SPACES, NTRML, TRML, LBRAC, RBRAC, CHR, COLON, PIPE, SCOLON, STAR};

//Base class for generating parsers. Handles conversion of grammar configuration strings into internal representation of the grammar
//Inherited classes will implement specific parsing algorithms
class BaseParserGenerator{
protected:
    //Total # of tokens used by the grammar. Initialized after parsing finishes
    int tokenNum;
    // Grammar is a list of ints divided into segments representing production rules. 
    // The first int of each segment is the # of rhs symbols in the production, followed by each rhs symbol
    // Ex: a : A B C | B becomes  3, 129, 130, 131, 1, 130 
    std::vector<int> grammar;
    // A on/off array indicating which tokens will be ignored
    std::vector<char> tokenIgnore; 
    // Maps lhs symbol # to the position in the grammar where the production with that lhs symbol starts. Padded at the end to allow looping
    // Allows instant access of productions with any lhs symbol
    std::vector<int> ruleNumStart;
    
private:
    //Internal class handling resources for turning the grammar string into an integral representation
    class GrammarParser{
    public:
        // 0->newline  1->spaces  2->nonterminal  3->terminal  4->lbrac  5->rbrac  6->char  7->colon  8->pipe  9->scolon  10->star
        char *regexps[11] = {"\n", " *", "[a-z]+", "[A-Z]+", "{", "}", "'.'", ":", "\\|", ";", "\\*"};
        // Lexer used for parsing grammar string
        Lexer lexer = Lexer(regexps, 11, 0);
        // Maps nonterminal and terminal symbols to their numerical representations
        std::unordered_map<std::string, int> symbolTable;
        // Tracks current and previous location in the grammar string so that words can be extracted
        char *curpos;
        char *prevpos;
        BaseParserGenerator *parser;
        //Number of rules parsed so far, not productions. Truly initialized once all tokens have been parsed
        int ruleNum = -10000;
        //Number of tokens parsed so far. Number starts from after all ASCII chars
        int tokenNum = 1 + NumOfChars;
        //Placeholder number for nonterminals not yet encountered on the lhs. Decrements as more are found
        int unfoundRuleNum = -1;

        GrammarParser(BaseParserGenerator *p, char *grammarConfig);
        //Parser helper functions
        void error(Gtoken token);
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


public:
    BaseParserGenerator(char * grammarConfig);
    friend std::ostream& operator<<(std::ostream& os, const BaseParserGenerator& parser);

};


#endif