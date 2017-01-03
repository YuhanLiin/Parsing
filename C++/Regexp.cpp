#include <vector>
#include <cstdlib>
#include <bitset>
#include <exception>
#include <iostream>

namespace regexp{
    const int NumOfChars = 256;
    struct State{
        std::bitset<regexp::NumOfChars> transitions;
        int edge;
        int epsilon1;
        int epsilon2;
    };
}

class RegexSyntaxError : public std::exception{
    char *str;
public:    
    RegexSyntaxError(int pos){
        str = itoa(pos, str, 10);
    }

    const char *what(){
        return str;
    }
};

class BaseRegexp{
protected:
    std::vector< regexp::State* > nfa;
    int starting;

    class RegexpBuilder{
    public:
        char* regexp;
        int pos = 0;
        BaseRegexp *rePtr;

        bool notEnd(){
            return regexp[pos] != 0;
        }
        char next(char c){
            if (notEnd() && regexp[pos] == c){
                pos++;
                return regexp[pos-1];
            }
            return 0;
        }
        char next(int low, int high){
            if (notEnd() && regexp[pos] >= low && regexp[pos] <= high){
                pos++;
                return regexp[pos-1];
            }
            return 0;
        }
        void error(){
            std::cerr << "Regex error at position " << pos;
            throw RegexSyntaxError(pos);
        }

        void updateFragment(std::bitset<regexp::NumOfChars>& transitions, int lower, int upper){
            for (int i=lower; i<=upper; i++){
                transitions[i] = 1;
            }

        }
        int push(){
            int statenum = rePtr->nfa.size();
            rePtr->nfa.push_back(new regexp::State);
            rePtr->nfa[statenum]->transitions = std::bitset<regexp::NumOfChars>();
            rePtr->nfa[statenum]->edge = -2;
            rePtr->nfa[statenum]->epsilon1 = -1;
            rePtr->nfa[statenum]->epsilon2 = -1;
            return statenum; 
        }
        void alternate(int& startL, int& endL, int startR, int endR){
            int root = push();
            rePtr->nfa[root]->epsilon1 = startL;
            rePtr->nfa[root]->epsilon2 = startR;
            int closure = push();
            concatenate(endL, closure);
            concatenate(endR, closure);
            rePtr->nfa[closure]->epsilon1 = -2;
            startL = root;
            endL = closure;
        }
        void concatenate(int left, int right){
            regexp::State* lptr = rePtr->nfa[left];
            if (lptr->edge == -2)
                lptr->edge = right;
            if (lptr->epsilon1 == -2)
                lptr->epsilon1 = right;
            if (lptr->epsilon2 == -2)
                lptr->epsilon2 = right;
        }
        void optional(int& start, int& end){
            int newStart = push();
            int newEnd = push();
            rePtr->nfa[newStart]->epsilon2 = start;
            rePtr->nfa[newStart]->epsilon1 = newEnd;
            concatenate(end, newEnd);
            rePtr->nfa[newEnd]->epsilon1 = -2;
            start = newStart;
            end = newEnd;
        }
        void repeating(int& start, int& end){
            int newEnd = push();
            rePtr->nfa[newEnd]->epsilon2 = start;
            rePtr->nfa[newEnd]->epsilon1 = -2;
            concatenate(end, newEnd);
            end = newEnd;
        }
        void kleene(int& start, int& end){
            repeating(start, end);
            start = end;
        }

        void parseRegexp(int& startL, int& endL){
            parseConcat(startL, endL);
            while (next('|')){
                int startR, endR;
                parseConcat(startR, endR);
                alternate(startL, endL, startR, endR);
            }
        }

        void parseConcat(int& startL, int& endL){
            parseUnary(startL, endL);
            while (notEnd() && regexp[pos]!='|' && regexp[pos]!=')'){
                int startR, endR;
                parseUnary(startR, endR);
                concatenate(endL, startR);
                endL = endR;
            }
        }

        void parseUnary(int& start, int& end){
            parseValue(start, end);
            if (next('*')){
                kleene(start, end);
            }
            else if (next('+')){
                repeating(start, end);
            }
            else if (next('?')){
                optional(start, end);
            }
        }

        void parseValue(int& start, int& end){
            if (next('(')){
                parseRegexp(start, end);
                if (!next(')')) error();
            }
            else{
                if (next('['))
                    start = parseSet();
                else
                    start = parseChar();
                end = start;
            }
        }

        char parseC(){
            char c = next(1,255);
            switch(c){
                case 0:
                    error();
                    break;
                case '\\':
                    error();
                    break;
                case '.':
                    error();
                    break;
                case '[':
                    error();
                    break;
                case ']':
                    error();
                    break;
                 case '(':
                    error();
                    break;
                case ')':
                    error();
                    break;
                default:
                    return c;
            }
        }

        int parseChar(){
            int start = push();
            std::bitset<regexp::NumOfChars>& transition = rePtr->nfa[start]->transitions;
            if (next('\\')){
                parseSpecial(transition);
            }
            else if (next('.')){
                updateFragment(transition, 1, '\n');
                updateFragment(transition, '\n'+1, 255);
            }
            else{
                char c = parseC();
                transition[c] = 1;
            }
            return start;
        }

        int parseSet(){
            int start = push();
            std::bitset<regexp::NumOfChars>& transition = rePtr->nfa[start]->transitions;
            bool inverse = false;
            if (next('^'))
                inverse = true;
            parseElem(transition);
            while (!next(']')){
                parseElem(transition);
            }
            if (inverse){
                transition.flip();
            }
            return start;
        }

        void parseElem(std::bitset<regexp::NumOfChars>& transition){
            if (next('\\')){
                parseSpecial(transition);
            }
            else if (next('.')){
                updateFragment(transition, 1, '\n');
                updateFragment(transition, '\n'+1, 255);
            }
            else{
                char c = parseC();
                if (next('-')){
                    char r = parseC();
                    updateFragment(transition, c, r);
                }
                else transition[c] = 1;
            }
        }

        void parseSpecial(std::bitset<regexp::NumOfChars>& transition){
            char c = next(1, 255);
            switch(c){
                case 0:
                    transition['\\'] = 1;
                    break;
                case 'd':
                    updateFragment(transition, '0', '9');
                    break;
                case 'D':
                    updateFragment(transition, 1, '0'-1);
                    updateFragment(transition, '9'+1, 255);
                    break;
                case 's':
                    transition[' '] = 1;
                    break;
                case 'S':
                    updateFragment(transition, 1, ' '-1);
                    updateFragment(transition, ' '+1, 255);
                    break;
                default:
                    transition[c] = 1;
            }
        }

    public:
        void build(char* re, BaseRegexp *rp, int &start, int &accept){
            regexp = re;
            rePtr = rp;
            int end;
            parseRegexp(start, end);
            accept = push();
            concatenate(end, accept);
        }
    };
    friend RegexpBuilder;

    void addState(int state, std::vector<int>& curStates, int *listids, int id){
        if (listids[state] != id){
            listids[state] = id;
            curStates.push_back(state);
        }
    }

    virtual int isAccepting(int state) = 0;

    int simulate(char* &str){
        std::vector<int> curStates;
        std::vector<int> nextStates;
        int i = 0;
        int lastAcceptPos = 0;
        int lastAcceptState = -1;
        curStates.push_back(starting);

        int *listids = new int[nfa.size()];
        for (int i=0; i<nfa.size(); i++)
            listids[i] = -1;

        for (int i=0; !curStates.empty(); i++){
            char c = str[i];
            for (int j=0; j<curStates.size(); j++){
                int accept = isAccepting(curStates[j]);
                if (accept > -1){
                    lastAcceptPos = i;
                    lastAcceptState = accept;
                }

                regexp::State *stateptr = nfa[curStates[j]];
                if (stateptr->epsilon1 >= 0)
                    addState(stateptr->epsilon1, curStates, listids, i);
                if (stateptr->epsilon2 >= 0)
                    addState(stateptr->epsilon2, curStates, listids, i);
                if (stateptr->transitions[c])
                    addState(stateptr->edge, nextStates, listids, i+1);
            }
            curStates.swap(nextStates);
            nextStates.clear();

            if (str[i] == 0) break;
        }

        delete[] listids;
        str += lastAcceptPos;
        return lastAcceptState;
    }

    ~BaseRegexp(){
        for (int i=0; i<nfa.size(); i++){
            delete[] nfa[i];
        }
        starting = -1;
    }

    friend std::ostream& operator<<(std::ostream& os, const BaseRegexp& regexp);
};

std::ostream& operator<<(std::ostream& os, const BaseRegexp& regexp){
    for (int i=0; i<regexp.nfa.size(); i++){
        os << i << " -> ";
        regexp::State *stateptr = regexp.nfa[i];
        if (stateptr->transitions.any()){
            os << stateptr->edge << " : {";
            for (int j=0; j<regexp::NumOfChars; j++){
                if (stateptr->transitions[j] == 1)
                    os << (char)j;
            }
            os << "} ";
        }
        if (stateptr->epsilon1 >= 0)
            os << "epsilon:" << stateptr->epsilon1 << ' ';
        if (stateptr->epsilon2 >= 0)
            os << "epsilon:" << stateptr->epsilon2 << ' ';
        os << '\n';
    }
    os << "Starting: " << regexp.starting << '\n';
    return os;
}

class Regexp: public BaseRegexp{
private:
    int accepting;

    int isAccepting(int state){
        if (state == accepting)
            return accepting;
        return -1;
    }

public:
    int match(char* str){
        char *end = str;
        int success = simulate(end);
        if (success < 0)
            return -1;
        return end-str;
    }

    int search(char* str){
        for (int i=0; str[i]!=0; i++){
            char *end = str+i;
            int success = simulate(end);
            if (success >= 0){
                return i;
            }
        }
        return -1;
    }

    Regexp(char* re){
        RegexpBuilder builder;
        builder.build(re, this, starting, accepting);

    }
    friend std::ostream& operator<<(std::ostream& os, const Regexp& regexp);
};

std::ostream& operator<<(std::ostream& os, const Regexp& regexp){
    os << (const BaseRegexp&)regexp << "Accepting: " << regexp.accepting << '\n';
    return os;
}

class Lexer : public BaseRegexp{
private:
    int* acceptTable;

    int isAccepting(int state){
        return acceptTable[state];
    }

public:
    Lexer(char* regexplist[], int len){
        if (len == 0){
            acceptTable = new int[0];
            return;
        }

        int* acceptList = new int[len];
        RegexpBuilder builder;
        builder.build(regexplist[0], this, starting, acceptList[0]);
        for (int i=1; i<len; i++){
            int startR, acceptR;
            RegexpBuilder builder;
            builder.build(regexplist[i], this, startR, acceptList[i]);
            acceptR = acceptList[i];
            builder.alternate(startR, acceptR, starting, acceptList[i-1]);
            nfa.pop_back();
            starting = startR;
        }

        acceptTable = new int[nfa.size()];
        for (int i=0; i<nfa.size(); i++)
            acceptTable[i] = -1;
        for (int i=0; i<len; i++)
            acceptTable[acceptList[i]] = i;

        delete[] acceptList;
    }

    ~Lexer(){
        delete[] acceptTable;
    }
    friend std::ostream& operator<<(std::ostream& os, const Lexer& regexp);
};

std::ostream& operator<<(std::ostream& os, const Lexer& regexp){
    os << (const BaseRegexp&)regexp << "Accepting: ";
    for (int i = 0; i<regexp.nfa.size(); i++){
        if (regexp.acceptTable[i] >= 0)
            os << i << "->t" << regexp.acceptTable[i] << ' ';
    }
    os << '\n';
    return os;
}

int main(int argc, char* argv[]){
    Lexer lexer = Lexer(argv+1, 5);
    std::cout << lexer;
}