#ifndef BASEPARSERGENERATOR_H
#define BASEPARSERGENERATOR_H

#include "Lexer.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cstring>

//Represents the tokens used to parse grammar strings
enum Gtoken {NEWLINE=0, SPACES, NTRML, TRML, LBRAC, RBRAC, CHR, COLON, PIPE, SCOLON, STAR};

//Base class for generating parsers. Handles conversion of grammar configuration strings into internal representation of the grammar
//Inherited classes will implement specific parsing algorithms
class BaseParserGenerator{
protected:
    typedef std::vector<int> Rule;
    std::vector<Rule> grammar;
    std::vector<char> tokenIgnore; 
    std::vector<int> ruleNumStart;
    

private:
    class GrammarParser{
    public:
        // 0->newline  1->spaces  2->nonterminal  3->terminal  4->lbrac  5->rbrac  6->char  7->colon  8->pipe  9->scolon  10->star
        char *regexps[11] = {"\n", " *", "[a-z]+", "[A-Z]+", "{", "}", "'.'", ":", "\\|", ";", "\\*"};
        Lexer lexer = Lexer(regexps, 11, 0);
        typedef std::unordered_map<std::string, int> SymbolTable;
        SymbolTable symbolTable;
        char *curpos;
        char *prevpos;
        BaseParserGenerator *parser;
        //Number of rules parsed so far, not productions. Truly initialized once all tokens have been parsed
        int ruleNum = -10000;
        //Number of tokens parsed so far. Number starts from after all ASCII chars
        int tokenNum = 1 + NumOfChars;
        int unfoundRuleNum = -1;

        GrammarParser(BaseParserGenerator *p, char *grammarConfig);
        void error(Gtoken token);
        void next();
        void next(Gtoken gtoken);
        bool tokenIs(Gtoken gtoken);
        void getWord(std::string &word);
        void replaceSymbol(int old, int replacement);
        //Returns the number that maps to the input symbol. Returns 0 if the symbol isn't in the table
        int symbolNumber(std::string &symbol);
        void parseTokens();
        void parseRule();
        void parseProduction(Rule& production);
        void parseGrammar();
    };
    friend GrammarParser;


public:
    BaseParserGenerator(char * grammarConfig);
    friend std::ostream& operator<<(std::ostream& os, const BaseParserGenerator& parser);

};


#endif