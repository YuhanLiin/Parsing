#pragma once
#include <vector>
#include <unordered_set>
#include <functional>

//Data structure representing a LR item 
struct LRItem{
    //Position of item's production in the grammar
    int prodPos;
    //The nth symbol behind the "dot"
    int dotPos;
    //LHS of item's production
    int lhs;
    //Return copy of item shifted by one
    LRItem advance(); 
    //For the hashset
    bool operator==(const LRItem& other);
};

namespace std {
    template <>
        class hash<LRItem>{
        public :
            size_t operator()(const LRItem &item ) const
            {
                return item.dotPos ^ item.lhs;
            }
    };
};

//Data structure representing the entire set of item states. Maintains two vectors of same size to separate kernel and closure sets
class LRStateSet{
private:
    //The kernel states of the set. Used set to make checking for repeat kernel states easier
    std::vector<std::unordered_set<LRItem>> kernelSet;
    //The closure states of the set.
    std::vector<std::vector<LRItem>> closureSet;
    //Length of 2 vectors
    size_t length = 0;
public:
    //Make new state 
    void newState();
    //Return size
    size_t size();
    //Returns a specified kernel/closure state in the state set
    std::unordered_set<LRItem>& kernelState(int index);
    std::vector<LRItem>& closureState(int index);
};