#include <vector>
#include <string>
#include <utility>
#include <exception>
#include <iostream>

class BaseRegexp{
private:
    std::vector<int*> nfa;
    int starting;
    int accepting;

protected:
    class RegexpBuilder{
    public:
        std::string regexp;
        int pos = 0;
        BaseRegexp *rePtr;

        bool notEnd(){
            return pos < regexp.size();
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
            throw std::runtime_error("Regex Syntax Error.");
        }

        void updateFragment(int* transitions, int lower, int upper){
            for (int i=lower; i<=upper; i++){
                transitions[i] = -2;
            }
        }
        int push(){
            int statenum = rePtr->nfa.size();
            rePtr->nfa.push_back(new int[258]);
            for (int i=0; i<258; i++){
                rePtr->nfa[statenum][i] = -1;
            }
            return statenum; 
        }
        void concatenate(int left, int right){
            for (int i=0; i<258; i++){
                if (rePtr->nfa[left][i] == -2){
                    rePtr->nfa[left][i] = right;
                }
            }
        }
        std::pair<int,int> optional(int start, int end){
            int newStart = push();
            int newEnd = push();
            rePtr->nfa[newStart][257] = start;
            rePtr->nfa[newStart][256] = newEnd;
            concatenate(end, newEnd);
            rePtr->nfa[newEnd][256] = -2;
            return std::make_pair(newStart, newEnd);
        }
        std::pair<int,int> repeating(int start, int end){
            int newEnd = push();
            rePtr->nfa[newEnd][257] = start;
            rePtr->nfa[newEnd][256] = -2;
            concatenate(end, newEnd);
            return std::make_pair(start, newEnd);
        }
        std::pair<int,int> kleene(int start, int end){
            std::pair<int,int> se = repeating(start, end);
            return std::make_pair(se.second, se.second);
        }

        std::pair<int,int> parseRegexp(){
            std::pair<int,int> startEndL = parseConcat();
            while (next('|')){
                std::pair<int,int> startEndR = parseConcat();
                int root = push();
                rePtr->nfa[root][256] = startEndL.first;
                rePtr->nfa[root][257] = startEndR.first;
                int closure = push();
                concatenate(startEndL.second, closure);
                concatenate(startEndR.second, closure);
                rePtr->nfa[closure][256] = -2;
                startEndL = std::make_pair(root, closure);
            }
            return startEndL;
        }

        std::pair<int,int> parseConcat(){
            std::pair<int,int> startEndL = parseUnary();
            while (notEnd() && regexp[pos]!='|' && regexp[pos]!=')'){
                std::pair<int,int> startEndR = parseUnary();
                //std::cout << startEndL.second << ' ' << startEndR.first << '\n';
                concatenate(startEndL.second, startEndR.first);
                startEndL.second = startEndR.second;
            }
            return startEndL;
        }

        std::pair<int,int> parseUnary(){
            std::pair<int,int> startEnd = parseValue();
            if (next('*')){
                startEnd = kleene(startEnd.first, startEnd.second);
            }
            else if (next('+')){
                startEnd = repeating(startEnd.first, startEnd.second);
            }
            else if (next('?')){
                startEnd = optional(startEnd.first, startEnd.second);
            }
            return startEnd;
        }

        std::pair<int,int> parseValue(){
            if (next('(')){
                std::pair<int,int> se = parseRegexp();
                if (!next(')')) error();
                return se;
            }
            else{
                int s;
                if (next('['))
                    s = parseSet();
                else
                    s = parseChar();
                return std::make_pair(s, s);
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
            int* transition = rePtr->nfa[start];
            if (next('\\')){
                parseSpecial(transition);
            }
            else if (next('.')){
                updateFragment(transition, 1, '\n');
                updateFragment(transition, '\n'+1, 255);
            }
            else{
                char c = parseC();
                transition[c] = -2;
            }
            return start;
        }

        int parseSet(){
            int start = push();
            int* transition = rePtr->nfa[start];
            bool inverse = false;
            if (next('^'))
                inverse = true;
            parseElem(transition);
            while (!next(']')){
                parseElem(transition);
            }
            if (inverse){
                for (int i=0; i<258; i++){
                    if (transition[i] == -2)
                        transition[i] = -1;
                    else
                        transition[i] = -2;
                }
            }
            return start;
        }

        void parseElem(int* transition){
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
                else transition[c] = -2;
            }
        }

        void parseSpecial(int* transition){
            char c = next(1, 255);
            switch(c){
                case 0:
                    transition['\\'] = -2;
                    break;
                case 'd':
                    updateFragment(transition, '0', '9');
                    break;
                case 'D':
                    updateFragment(transition, 1, '0'-1);
                    updateFragment(transition, '9'+1, 255);
                    break;
                case 's':
                    transition[' '] = -2;
                    break;
                case 'S':
                    updateFragment(transition, 1, ' '-1);
                    updateFragment(transition, ' '+1, 255);
                    break;
                default:
                    transition[c] = -2;
            }
        }

    public:
        void build(std::string re, BaseRegexp *rp){
            regexp = re;
            rePtr = rp;
            std::pair<int,int> startEnd = parseRegexp();
            int accepting = push();
            concatenate(startEnd.second, accepting);
            rePtr->starting = startEnd.first;
            rePtr->accepting = accepting;
        }
    };
    friend RegexpBuilder;

public:
    ~BaseRegexp(){
        for (int i=0; i<nfa.size(); i++){
            delete[] nfa[i];
        }
        starting = -1;
        accepting = -1;
    }

    friend std::ostream& operator<<(std::ostream& os, const BaseRegexp& regexp);
};

std::ostream& operator<<(std::ostream& os, const BaseRegexp& regexp){
        os << regexp.starting << ' ' << regexp.accepting << '\n';
        for (int i=0; i<regexp.nfa.size(); i++){
            os << i << " -> ";
            for (int j=0; j<258; j++){
                if (regexp.nfa[i][j] != -1){
                    if (j==256 || j==257)
                        os << "epsilon";
                    else
                        os << (char)j;
                    os << ':' << regexp.nfa[i][j] << ' ';
                }
            }
            os << '\n';
        }
        return os;
    }

class Regexp: public BaseRegexp{
public:
    Regexp(std::string re){
        RegexpBuilder builder;
        try{
            builder.build(re, this);
        }
        catch(...){
            std::cout << "Error at: " << builder.pos << '\n';
        }

    }
};

int main(int argc, char* argv[]){
    Regexp regex = Regexp(argv[1]);
    std::cout << regex;    
}