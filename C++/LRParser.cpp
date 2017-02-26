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
        closureHelper(stateSet, item, closed, stateNum);
    }
    for (const LRItem &item : stateSet.closureState(stateNum)){
        closureHelper(stateSet, item, closed, stateNum);
    }
    delete[] closed;
}

//Gets called on every LRItem in the parse state during closure
void LRParser::closureHelper(LRStateSet &stateSet, LRItem item, bool *closed, int stateNum){
    int symbol = curSymbol(item);
    //If the symbol is nonterminal and hasnt been closed yet
    if (!isTerminal(symbol) && !closed[symbol]){
        closed[symbol] = true;
        //For every production the symbol derives. Add a starting LR item for the production into the closure set
        for (int r=ruleStart(toRuleCount(symbol)); r<ruleStart(toRuleCount(symbol+1)); r=nextProduction(r)){
            LRItem item = {r, 0, symbol};
            stateSet.closureState(stateNum).push_back(item);
        }
    }
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
    //Make it so that when the starting state recieves the completion of the starting rule, the acceptance action is triggered
    table(0, toRuleNum(0)) = -3;
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

ParseStatus LRParser::parse(char *input){
    curpos = input;
    prevpos = input;
    deleteValues(valueStack.size());
    lexptr->reset();
    stateStack.clear();
    stateStack.push_back(0);
    curTokenNum = next();
    return shiftHelper();
}

//Advances the parse until a reduction occurs
ParseStatus LRParser::shiftHelper(){
    //Query the next action in the parse table via the most recent state and the current token
    int action = table(stateStack.back(), curTokenNum);
    std::cout << curTokenNum;
    //Continue while the parse table's next action is a shift action
    while (action >= 0){
        //Push the shifted-to state into the stack and get the next token   
        stateStack.push_back(action);
        curTokenNum = next();
        action = table(stateStack.back(), curTokenNum);
        //Push string value onto value stack
        addParseValue();
    }
    if (action == -2){
        return GOOD;
    }
    else{
        return SYNTAXERROR;
    }
}

//Performs a reduction and appends a user-provided value onto the value stack
ParseStatus LRParser::reduce(void *reducedValue, bool toDelete){
    int lastState = stateStack.back();
    if (lastState != -2) return SYNTAXERROR;
    //The # of states to pop off the state stack depends on the # of symbols in the reduced production
    symbolCount = grammar[table.prodPos(lastState)];
    for (int i=0; i<symbolCount; i++){
        stateStack.pop_back();
    }
    //Replace the popped states with the new state that corresponds to the reduced lhs
    stateStack.push_back(table(stateStack.back(), table.lhsNum(lastState)));
    //Pop off the same # of values off the value stack and replace with the inputted parse value
    deleteValues(symbolCount);
    valueStack.push_back(ParseValue());
    valueStack.back().toDelete = toDelete;
    valueStack.back().ptr = reducedValue;

    //If the acceptance action has been encountered, meaning that the starting rule has been completely reduced
    if (stateStack.back()==-3){
        //The parse is successful if no tokens are left
        if (curTokenNum==0){
            return DONE;
        }
        //Otherwise, \0 is the expected token
        else{
            return SYNTAXERROR;
        }
    }
    return shiftHelper();
}

//Adds a parse value to the value stack corresponding to the current token
void LRParser::addParseValue(){
    valueStack.push_back(ParseValue());
    ParseValue &value = valueStack.back();
    //Allocate a new string onto the ptr and copy the contents of the current token into it
    std::string* sptr = new std::string;
    for (char * c=prevpos; c<curpos; c++){
        sptr->push_back(*c);
    }
    value.ptr = sptr;
    //The parser will clean the memory of the value
    value.toDelete = true;
}

//Deletes x number of ParserValues on the value stack. Responsible for cleaning the associated memory
void LRParser::deleteValues(int count){
    for (int i=0; i<count; i++){
        if (valueStack.back().toDelete){
            delete valueStack.back().ptr;
        }
        valueStack.pop_back();
    }
}

int LRParser::lhsNum(){
    return table.lhsNum(stateStack.back());
}

int LRParser::prodNum(){
    return table.prodNum(stateStack.back());
}

void *LRParser::rhsVal(int pos){
    return valueStack[valueStack.size() - symbolCount + pos].ptr;
}

int LRParser::curToken(){
    return curTokenNum;
}

//Returns list of tokens the parser expects at this point in the parse
std::vector<int> LRParser::expectedTokens(){
    std::vector<int> list;
    //Having the acceptance state means that the input was too long, and \0 was expected
    if (stateStack.back()==-3){
        list.push_back(0);
        return list;
    }
    //Loop thru all look ahead tokens and add the ones that produce a shift action for the current state
    for (int i=0; i<tokenNum; i++){
        if (table(stateStack.back(), i) >= 0){
            list.push_back(i);
        }
    }
    return list;
}