#include "LLParser.h"

//Populates the LL parse table for a given LHS symbol
void LLParser::populateTable(int lhs, std::unordered_set<int> &firstSet){
    //Only run function once for every symbol. By default symbols do not derive epsilon
    if (derivesEpsilon[lhs] > -2){
        return;
    }
    derivesEpsilon[lhs] = -1;
    //For each production of the given rule (i is start of production)
    for (int i = ruleStart(lhs); i < ruleStart(lhs+1); i = nextProduction(i)){
        std::unordered_set<int> descendantSet;
        int j;
        //For each symbol in the production (j is index of symbol)
        for (j = i+1; j <= i+grammar[i]; j++){
            int frontSymbol = grammar[j];
            //If terminal symbol was found, then update the first set and parse table for this production
            if (isTerminal(frontSymbol)){
                table(lhs, frontSymbol) = i;
                firstSet.insert(frontSymbol);
                //Go to next production
                break;
            }
            //Otherwise recursively populate the symbol's first set as well
            else{
                populateTable(toRuleCount(frontSymbol), descendantSet);
                //If the symbol derives epsilon, the following symbol is part of first set as well
                if (derivesEpsilon[toRuleCount(frontSymbol)] < 0){
                    break;
                }
            }
        }
        //Run through the descendant first sets and add them to the current first set and the parse table for current production
        for (auto p: descendantSet){
            firstSet.insert(p);
            table(lhs, p) = i;
        }
        //If entire production has been considered, it means that every symbol derives epsilon, so the lhs does so as well
        if (j > i+grammar[i]){
            derivesEpsilon[lhs] = i;
        }
    }
}

//Constructor populates LL parse table
LLParser::LLParser(char* grammarConfig, Lexer *lexptr) : BaseParserGenerator(grammarConfig, lexptr){
    //Initialize epsilon array to -2 (unseen)
    for (int i=0; i<toRuleCount(ruleNum); i++){
        derivesEpsilon[i] = -2;
    }
    //Populate the parse table
    for (int i=0; i<toRuleCount(ruleNum); i++){
        populateTable(i, std::unordered_set<int>());
    }
}

LLParser::~LLParser(){
    delete[] derivesEpsilon;
    deleteValues(valueStack.size());
}

//Deletes x number of ParserValues on the value stack. Responsible for cleaning the associated memory
void LLParser::deleteValues(int count){
    for (int i=0; i<count; i++){
        if (valueStack.back().toDelete){
            delete valueStack.back().ptr;
        }
        valueStack.pop_back();
    }
}

//Reset all parse variables and shift until next reduction 
ParseStatus LLParser::parse(char* input){
    curpos = input;
    prevpos = input;
    deleteValues(valueStack.size());
    symbolStack.clear();
    //Initialize the symbolStack with starting symbol
    symbolStack.push_back(toRuleNum(0));
    lexptr->reset();
    curTokenNum = next();
    return shiftHelper();
}

//Complete current reduction by replacing the rhs values on the value stack with input lhs value.
//Checks for end of parse then continue shifting.
ParseStatus LLParser::reduce(void *lhsValue, bool toDelete){
    //If the parse stack is empty AND the string has been fully parsed, the parse is done
    if (symbolStack.empty() && curTokenNum==0){
        return DONE;
    }
    //If only the parse stack is empty, then the expected token should be \0, since the parse expects end of input
    else if (symbolStack.empty()){
        expectedSymbol = 0;
        return SYNTAXERROR;
    }
    //Pop off reduction token
    symbolStack.pop_back();
    //Replace appropriate number of values off the value stack with user inputted value
    deleteValues(curSymbolCount);
    ParseValue value;
    value.ptr = lhsValue;
    value.toDelete = toDelete;
    valueStack.push_back(value);
    //Shift until next reduction
    return shiftHelper();
}

//Return unincremented lhs symbol number
int LLParser::lhsNum(){
    return toRuleCount(curLhs);
}
//Return production number
int LLParser::prodNum(){
    return curProdNum;
}
//Return pointer to value of nth reduced symbol
void *LLParser::rhsVal(int pos){
    //std::cout << curSymbolCount;
    return valueStack[valueStack.size() - curSymbolCount + pos].ptr;
}

int LLParser::curToken(){
    return curTokenNum;
}

//Builds a list of expected tokens/chars and returns them
std::vector<int> LLParser::expectedTokens(){
    std::vector<int> expected;
    //If the expected symbol is a terminal, then that symbol is the only expected token
    if (isTerminal(expectedSymbol)){
        expected.push_back(expectedSymbol);
        return expected;
    }
    //If the symbol is nonterminal, then the expected tokens are all that qualify as lookahead for that symbol
    //Search the parse table for the tokens
    for (int i=0; i<tokenNum; i++){
        if (table(toRuleCount(expectedSymbol), i) >= 0){
            expected.push_back(i);
        }
    }
    return expected;
}

//Shift and expand tokens onto the parse stack until the next reduction happens
//Reductions are represented with a -ve value on the stack, representing the position of the production in the grammar
ParseStatus LLParser::shiftHelper(){
    //If the parse stack is empty AND the string has been fully parsed, the parse is done
    if (symbolStack.empty() && curTokenNum==0){
        return DONE;
    }
    //If only the parse stack is empty, then the expected token should be \0, since the parse expects end of input
    else if (symbolStack.empty()){
        expectedSymbol = 0;
        return SYNTAXERROR;
    }
    //Stop at next reduction
    while (symbolStack.back() > 0){
        //Pop symbol off parse stack
        int symbol = symbolStack.back();
        symbolStack.pop_back();
        //If the symbol is a terminal, try to match it to the current token. Get the next token
        if (isTerminal(symbol)){
            if (symbol == curTokenNum){
                //Push the shifted token string onto the value stack
                addTokenValue();
                curTokenNum = next();
            }
            else{
                expectedSymbol = symbol;
                return SYNTAXERROR;
            }
        }
        //If symbol is nonterminal, insert the reduction symbol and the correct production backwards into the parse stack based on parse table
        else{
            int productionPos = table(toRuleCount(symbol), curTokenNum);
            //If table entry doesn't exist for current lhs and token, then see if the lhs derives epsilon
            if (productionPos < 0){
                if (derivesEpsilon[toRuleCount(symbol)] > -1){
                    //Use the epsilon-deriving production as the reducing production
                    productionPos = derivesEpsilon[toRuleCount(symbol)];
                }
                //If lhs doesn't derive epsilon, then syntax error
                else{
                    expectedSymbol = symbol;
                    return SYNTAXERROR;
                }
            }
            //Insert production
            symbolStack.push_back(-productionPos);
            addProduction(productionPos, symbolStack, true);
        }
    }
    //Extract info about the next reduction based on the next reduction symbol
    updateReductionInfo(-symbolStack.back());
    return GOOD;
}

//Determine LHS symbol (incremented), symbol count, and production number of a production given its position in the grammar
void LLParser::updateReductionInfo(int prodPos){
    curSymbolCount = grammar[prodPos];
    for (int i=0; i<ruleNumStart.size(); i++){
        //Find rule whose starting position exceeds production position and recognize its lhs symbol
        if (ruleStart(i) > prodPos) {
            curLhs = toRuleNum(i-1);
            break;
        }
    }
    //Loop through each production of the rule until the right one is found
    curProdNum = 0;
    for (int j=ruleStart(toRuleCount(curLhs)); j<prodPos; j=nextProduction(j)){
        curProdNum++;
    }
}

//Adds ParseValue of current token, which is the token string on the heap
void LLParser::addTokenValue(){
    std::string *stringPtr = new std::string();
    for (char* c=prevpos; c<curpos; c++){
        stringPtr->push_back(*c);
    }
    ParseValue value;
    value.ptr = stringPtr;
    value.toDelete = true;
    valueStack.push_back(value);
}