#include "LRHelper.h"

LRItem LRItem::advance(){
    LRItem item;
    item.prodPos = prodPos;
    item.dotPos = dotPos+1;
    item.lhs = lhs;
    return item;
}

bool LRItem::operator==(const LRItem &other){
    return (prodPos==other.prodPos && dotPos==other.dotPos && lhs==other.lhs);
}

// size_t LRItemHash::operator()(const LRItem &item){
//     return item.dotPos ^ item.prodPos;
// }

void LRStateSet::newState(){
    length++;
    kernelSet.push_back(std::unordered_set<LRItem>());
    closureSet.push_back(std::vector<LRItem>());
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