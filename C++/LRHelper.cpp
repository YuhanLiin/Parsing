#include "LRHelper.h"

LRItem LRItem::advance() const{
    LRItem item;
    item.prodPos = prodPos;
    item.dotPos = dotPos+1;
    item.lhs = lhs;
    item.isStarting = isStarting;
    return item;
}

bool operator==(const LRItem &x, const LRItem &y){
    return (y.prodPos==x.prodPos && y.dotPos==x.dotPos && y.lhs==x.lhs && x.isStarting == y.isStarting);
}

void LRStateSet::newState(){
    length++;
    kernelSet.push_back(std::unordered_set<LRItem>());
    closureSet.push_back(std::vector<LRItem>());
}

void LRStateSet::pop(){
    kernelSet.pop_back();
    closureSet.pop_back();
    length--;
}

size_t LRStateSet::size(){
    return length;
}

std::unordered_set<LRItem>& LRStateSet::kernelState(int index){
    return kernelSet[index];
}
std::vector<LRItem>& LRStateSet::closureState(int index){
    return closureSet[index];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t LRTable::size(){
    return length;
}

LRTable::LRTable(int symbols){
    symbolCount = symbols;
}

LRTable::~LRTable(){
    for (int* row : transitions)
        delete[] row;
    for (int* reduction : reductions)
        delete[] reduction;
}

//Add new row in parse table
void LRTable::newRow(){
    length++;
    //Initiate new parse table entry
    transitions.push_back(new int[symbolCount]);
    reductions.push_back(new int[3]);
    //Set all values in new entry to -1 as default
    for (int i=0; i<symbolCount; i++){
        transitions.back()[i] = -1;
    }
    reductions.back()[0] = -1;
    reductions.back()[1] = -1;
    reductions.back()[2] = -1;
}

//Make last state reduction state
void LRTable::reduceState(int prodPos, int lhs, int prodNum, bool isAccepting){
    //The number that the row will be set to is -3 for accepting state and -2 otherwise
    int entryNum = (isAccepting) ? -3 : -2;
    //Set entire table row to -2 (signifies a reduction) for symbols that have not yet been shifted. This prioritizes shift over reduce
    for (int i=0; i<symbolCount; i++){
        if (transitions.back()[i] == -1)
            transitions.back()[i] = entryNum;
    }
    //Set reduction parameters
    reductions.back()[0] = prodPos;
    reductions.back()[1] = lhs;
    reductions.back()[2] = prodNum;
}

//Reduction parameter accessors with expressive syntax
int& LRTable::prodPos(int state){
    return reductions[state][0];
}
int& LRTable::lhsNum(int state){
    return reductions[state][1];
}
int& LRTable::prodNum(int state){
    return reductions[state][2];
}
int& LRTable::operator()(int state, int symbolNum){
    return transitions[state][symbolNum];
}