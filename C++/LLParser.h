#ifndef LLPARSER_H
#define LLPARSER_H

#include "BaseParserGenerator.h"
#include "ParseTable.h"
#include <unordered_set>


class LLParser: public BaseParserGenerator{
private:
    //Parse table will be queried via rule/nonterminal count and token/char #. Value points to corresponding production in the grammar
    ParseTable table{toRuleCount(ruleNum), tokenNum, -1};
    //Determines whether a rule can derive epsilon or not. 
    //Value of -1 means the symbol hasn't been considered (only valid during table construction), 0 means no, 1 means yes 
    short *derivesEpsilon = new short[toRuleCount(ruleNum)];
    //Parse stacks for symbols and values. Values set by users will need to be deleted by users.
    std::vector<ParseValue> valueStack;
    std::vector<int> symbolStack;
    //Current token being parsed and the token expected by the parser
    int curTokenNum = -1;
    int expectedTokenNum = -1;
    //Current lhs, production number, and number of rhs symbols to reduce
    int curLhs = -1;
    int curProdNum = -1;
    int curSymbolCount = -1;
    //Builds parse table based on first sets
    void populateTable(int lhs, std::unordered_set<int>& firstSet);
    //Shift and expand tokens onto the parse stack until the next reduction happens
    ParseStatus shiftHelper();
    //Determine LHS symbol (incremented), symbol count, and production number of a production given its position in the grammar
    void updateReductionInfo(int prodPos);
    //Returns ParseValue of current token, which is the token string on the heap
    ParseValue tokenValue();

public:
    friend std::ostream& operator<<(std::ostream& os, LLParser& parser);
    LLParser(char*, Lexer*);
    ~LLParser();
    //Reset all internal variables and initiate parse on a new input. Begin the first reduction
    ParseStatus parse(char *input);
    //Finish a pending reduction and associate the produced lhs symbol with the reduced value. Begin the next reduction
    //Primary means of advancing the parsing
    ParseStatus reduce(void *reducedValue);
    //Return number of the lhs symbol being reduced
    int lhsNum();
    //Return which production of a rule is being reduced
    int prodNum();
    //Return pointer to value of a specific rhs value being reduced
    void *rhsVal(int pos);
    //Returns number of current token, the expected token (returns -1 for non-shift errors), and the column/line numbers in case parse fails
    int curToken();
    int expectedToken();
};
#endif