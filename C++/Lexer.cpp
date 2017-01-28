#include "Lexer.h"

//Lexer has multiple accept states, so the accept state table is queried to see which the regexp number the acceptance corresponds with
int Lexer::isAccepting(int state){
    return acceptTable[state];
}

//Lexer builds multiple regexps into a large NFA with multiple accepts
Lexer::Lexer(char* regexplist[], int len, int newlineToken=-1){
    //Invalid construction for empty regexp list
    if (len == 0){
        //Offset destructor
        acceptTable = new int[0];
        this->newlineToken = -100;
        return;
    }

    //Accept list maps the regexp numbers to the accept state numbers. To be transformed into acceptTable 
    int* acceptList = new int[len];
    //Build the first regexp as NFA
    RegexpBuilder builder;
    builder.build(regexplist[0], this, starting, acceptList[0]);
    //For each subsequent regexp, build a new NFA and alternate it to the previous one
    for (int i=1; i<len; i++){
        int startR, acceptR;
        RegexpBuilder builder;
        builder.build(regexplist[i], this, startR, acceptList[i]);
        acceptR = acceptList[i];
        builder.alternate(startR, acceptR, starting, acceptList[i-1]);
        //Pop off the extra end state, which isn't needed since we can allow multiple accept states
        nfa.pop_back();
        starting = startR;
    }

    //Default all acceptTable values to -1 (no accept)
    acceptTable = new int[nfa.size()];
    for (int i=0; i<nfa.size(); i++)
        acceptTable[i] = -1;
    //For each accept state, map it to its regexp number
    for (int i=0; i<len; i++)
        acceptTable[acceptList[i]] = i;

    this->newlineToken = newlineToken;
    delete[] acceptList;
}

//Parses the next token in the input and stores its info. Returns pointer to char after end of token
char* Lexer::lex(char* input){
    //Don't advance input if lexer was created with no regexp
    if (newlineToken == -100){
        return input;
    }
    char* curpos = input;
    //Run simulation, which advances the curpos pointer and produces the token id
    tokenID = simulate(curpos);
    if (tokenID == newlineToken){
        tokenLine++;
        tokenCol = 1;
    }
    else{
        tokenCol += curpos-input;
    }
    return curpos;
}

void Lexer::reset(){
    tokenLine = 1;
    tokenCol = 1;
    tokenID = -1;
}

bool Lexer::good(){
    return (tokenID > -1);
}

Lexer::~Lexer(){
    delete[] acceptTable;
}

//Lexer cout overload adds all the accepting states
std::ostream& operator<<(std::ostream& os, const Lexer& regexp){
    os << (const BaseRegexp&)regexp << "Accepting: ";
    for (int i = 0; i<regexp.nfa.size(); i++){
        if (regexp.acceptTable[i] >= 0)
            os << i << "->t" << regexp.acceptTable[i] << ' ';
    }
    os << '\n';
    return os;
}

// int main(int argc, char* argv[]){
//     Lexer lexer = Lexer(argv+1, 2, 0);
//     std::cout << lexer;
//     int location = lexer.lex(argv[3]) - argv[3];
//     std::cout << location << ' ' << lexer.tokenLine << ' ' << lexer.tokenCol << ' ' << lexer.tokenID;
// }