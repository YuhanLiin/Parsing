#pragma once
#include "BaseParserGenerator.h"
#include "LRHelper.h"
#include <iostream>

class LRParser : public BaseParserGenerator {
private:
    LRTable table{ruleNum};
    //Returns the symbol number right after the dot for a LR item
    int curSymbol(const LRItem &item);
    //Runs the closure operation on the latest kernel state
    void closure(LRStateSet &stateSetm, int state);
    //Gets called on every LRItem in the parse state during closure
    void closureHelper(LRStateSet &stateSet, LRItem item, bool *closed, int stateNum);
    //Runs the shift operation on the latest kernel state for the given symbol
    int shiftSymbol(LRStateSet &stateSet, int symbolNum, int state);
    //Makes the LR parse table
    void makeTable();
    //Helper function that runs for every item when looping thru an item state.
    //Responsible for performing either reduce or shift on the state for the given item. 
    void makeTableHelper(LRStateSet &stateSet, const LRItem &item, int curState, bool* shifted);
    //Find the production number of the production assiciated with a given item
    int findProdNum(const LRItem &item);

    //Parse stacks for states and parse values
    std::vector<ParseValue> valueStack;
    std::vector<int> stateStack;
    //The # of symbols in the currently reduced production
    int symbolCount;
    //Current token
    int curTokenNum;

    void deleteValues(int count);
    void addParseValue();
    ParseStatus shiftHelper();

public: 
    friend std::ostream &operator<<(std::ostream &os, LRParser &parser);
    //Constructs the parse table
    LRParser(char*, Lexer*);
    ~LRParser();
    //Reset all internal variables and initiate parse on a new input. Begin the first reduction
    ParseStatus parse(char *input);
    //Finish a pending reduction and associate the produced lhs symbol with the reduced value. Begin the next reduction
    //Primary means of advancing the parsing
    ParseStatus reduce(void *reducedValue, bool toDelete);
    //Return unincremented number of the lhs symbol being reduced
    int lhsNum();
    //Return which production of a rule is being reduced
    int prodNum();
    //Return pointer to value of a specific rhs value being reduced (0-indexed)
    void *rhsVal(int pos);
    
    //Returns number of current token, the list of expected tokens (returns -1 for non-shift errors), and the column/line numbers in case parse fails
    int curToken();
    std::vector<int> expectedTokens();
};