#pragma once
#include <vector>
#include <unordered_set>
#include <functional>

//Data structure representing a LR item 
struct LRItem{
    //Position of item's production in the grammar
    int prodPos;
    //The nth symbol behind the "dot". Starts at 0
    int dotPos;
    //LHS of item's production (incremented)
    int lhs;
    //Return copy of item shifted by one
    LRItem advance(); 
};

//For the hashset
bool operator==(const LRItem&, const LRItem&);

//Hash function for LR Items
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
    //Pop off the last state
    void pop();
    //Returns a specified kernel/closure state in the state set
    std::unordered_set<LRItem>& kernelState(int index);
    std::vector<LRItem>& closureState(int index);
};

//Represents LR parse table
class LRTable {
    //Vector of int arrays to represent the transition table of states to symbols
    //-1 is no transition, -2 is reduction, +ve indicates which state to shift to
    std::vector<int*> transitions;
    //Represent the reduction attributes of each state, if any. 
    //[0] is production position, [1] is the lhs, [2] is the production number 
    std::vector<int*> reductions;
    //Length of table
    size_t length = 0;
    //# of symbols in the grammar
    int symbolCount;
public:
    size_t size();
    //Initializes symbol Count
    LRTable(int symbols);
    ~LRTable();
    //Delete copy and assignment constructors
    LRTable(LRTable&) = delete;
    LRTable &operator=(LRTable&) = delete;
    //Add new row to table
    void newRow();
    //Turn the last state into a reduction state
    void reduceState(int prodPos, int lhs, int prodNum);
    //Returns production position in reducing state
    int& prodPos(int state);
    //Return reduction lhs num reference for a state
    int& lhsNum(int state);
    //Return reduction production num reference for state
    int& prodNum(int state);
    //Return the transition of a state for a given symbol
    int& operator()(int state, int symbolNum);
};