#include "LRParser.h"

int LRParser::curSymbol(const LRItem &item){
    //Length of item's production
    int len = grammar[item.prodPos];
    //If dot is at the end of the production's length, return -1
    if (item.dotPos >= len){
        return -1;
    }
    //Return symbol that the dot is in front of
    else return grammar[item.prodPos + item.dotPos + 1];
}

void LRParser::closure(LRStateSet &stateSet, int stateNum){
    //Array tracking whether a specific symbol has been subject to closure. Closure happens once per symbol
    bool *closed = new bool[ruleNum];
    for (int i=0; i<ruleNum; i++){
        closed[i] = false;
    }
    //Loop thru each item in the kernel set
    for (const LRItem &item : stateSet.kernelState(stateNum)){
        int symbol = curSymbol(item);
        //If the symbol is nonterminal and hasnt been closed yet
        if (!isTerminal(symbol) && !closed[symbol]){
            closed[symbol] = true;
            //For every production the symbol derives. Add a starting LR item for the production into the closure set
            for (int r=ruleStart(symbol); r<ruleStart(symbol+1); r=nextProduction(r)){
                LRItem item = {r, 0, symbol};
                stateSet.closureState(stateNum).push_back(item);
            }
        }
    }
    delete[] closed;
}

//Shifts the items in a given state for one symbol. Makes new state if it doesn't exist yet 
int LRParser::shiftSymbol(LRStateSet &stateSet, int stateNum, int symbol){
    //Add new state to the set and track its index
    stateSet.newState();
    int back = stateSet.size()-1;
    //For every item in the input state, advance any that have the given symbol as its dot symbol
    //Add them into the kernel of the new state
    for (const LRItem &item : stateSet.closureState(stateNum)){
        if (curSymbol(item) == symbol){
            stateSet.kernelState(back).insert(item.advance());
        }
    }
    for (const LRItem &item : stateSet.kernelState(stateNum)){
        if (curSymbol(item) == symbol){
            stateSet.kernelState(back).insert(item.advance());
        }
    }

    //Check if the newest kernel state is a repeat of any old kernel state
    for (int i=0; i<back; i++){
        if (stateSet.kernelState(i) == stateSet.kernelState(back)){
            //If so, delete it and return the old one
            stateSet.pop();
            return i;
        }
    }
    //Otherwise, return the new state
    return back;
}

//Builds the parse table using item state sets
void LRParser::makeTable(){
    //Create a new state set and add a first state
    LRStateSet stateSet;
    stateSet.newState();
    //Initialize the first kernel state with items corresponding to every production of the starting state (0)
    for (int i=0; i<ruleStart(1); i=nextProduction(i)){
        LRItem startItem = {i, 0, toRuleNum(0)};
        stateSet.kernelState(0).insert(startItem);
    }

    //Go thru the states in the state set until there is no more
    for (int curState=0; curState<stateSet.size(); curState++){
        //For each state, create a corresponding row in the parse table
        table.newRow();
        //Run closure operation on the state
        closure(stateSet, curState);
        //Array tracking whether a symbol has been reduced while processing this state
        bool *shifted = new bool[ruleNum];
        for (int i=0; i<ruleNum; i++){
            shifted[i] = false;
        }
        //Run the helper function for every item in the current state
        for (const LRItem &item : stateSet.kernelState(curState)){
            makeTableHelper(stateSet, item, curState, shifted);
        }
        for (const LRItem &item : stateSet.closureState(curState)){
            makeTableHelper(stateSet, item, curState, shifted);
        }
        delete[] shifted;
    }
}

//Helper function that runs for every item when looping thru an item state.
//Responsible for performing either reduce or shift on the state for the given item. 
void LRParser::makeTableHelper(LRStateSet &stateSet, const LRItem &item, int curState, bool* shifted){
    int symbol = curSymbol(item);
    //If the item's dot has reached the end of the production, reduce the state for that item.
    if (symbol == -1){
        table.reduceState(item.prodPos, item.lhs, findProdNum(item));
    }
    //Otherwise, if the dot symbol hasn't been shifted yet, then update the table with the shifted state for that symbol.
    else if (!shifted[symbol]){
        shifted[symbol] = true;
        int transition = shiftSymbol(stateSet, curState, symbol);
        table(curState, symbol) = transition;
    }
}

int LRParser::findProdNum(const LRItem &item){
    //Loop thru each production of the item's lhs, while keeping track of the production number
    int prodNum = 0;
    for (int i=ruleStart(toRuleCount(item.lhs)); i<ruleStart(toRuleCount(item.lhs+1)); i=nextProduction(i)){
        //If the right production is found, return it
        if (i == item.prodPos){
            return prodNum;
        }
        prodNum++;
    }
    return -1;
}

LRParser::LRParser(char* grammarConfig, Lexer *lexptr) : BaseParserGenerator(grammarConfig, lexptr){
    makeTable();
}