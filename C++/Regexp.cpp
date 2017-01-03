#include "Regexp.h"

//Converts position of syntax error into the error string 
RegexSyntaxError::RegexSyntaxError(int pos){
    str = itoa(pos, str, 10);
}
const char* RegexSyntaxError::what(){
    return str;
}

//Checks if the whole regexp has been parsed
bool BaseRegexp::RegexpBuilder::notEnd(){
    return regexp[pos] != 0;
}
//Advances the input if the next token matches input. Returns input if matched, otherwise 0
char BaseRegexp::RegexpBuilder::next(char c){
    if (notEnd() && regexp[pos] == c){
        pos++;
        return regexp[pos-1];
    }
    return 0;
}
//Same thing as above, except the input is an ASCII range
char BaseRegexp::RegexpBuilder::next(char low, char high){
    if (notEnd() && regexp[pos] >= low && regexp[pos] <= high){
        pos++;
        return regexp[pos-1];
    }
    return 0;
}
//Called when parser encounters syntax error. Throws custom error along with a message
void BaseRegexp::RegexpBuilder::error(){
    std::cerr << "Regex error at position " << pos;
    throw RegexSyntaxError(pos);
}

//Updates a state's transition bitset for a specific range of chars. 
//Called when individual chars are parsed to set a state's transitions 
void BaseRegexp::RegexpBuilder::updateFragment(std::bitset<regexp::NumOfChars>& transitions, char lower, char upper){
    for (int i=lower; i<=upper; i++){
        transitions[i] = 1;
    }
}

//Pushes a new state onto the NFA and returns its index
int BaseRegexp::RegexpBuilder::push(){
    int statenum = rePtr->nfa.size();
    rePtr->nfa.push_back(new regexp::State);
    //Sets default values of new state
    rePtr->nfa[statenum]->transitions = std::bitset<regexp::NumOfChars>();
    rePtr->nfa[statenum]->edge = -2; //The char edge will always be dangling. It will be invalidated if the transition bitset is 0
    rePtr->nfa[statenum]->epsilon1 = -1; //Epsilon edges default to -1 and will be turned on manually
    rePtr->nfa[statenum]->epsilon2 = -1;
    return statenum; 
}

//Takes the start and end of 2 regexp fragments and alternate them together into a new fragment: a | b => a|b
//Given R1-> , produce S -> R1 -> E -> (dangle)
//      R2->             -> R2 ->
void BaseRegexp::RegexpBuilder::alternate(int& startL, int& endL, int startR, int endR){
    //Push the new start (root) state and links its epsilon edges to the start states
    int root = push();
    rePtr->nfa[root]->epsilon1 = startL;
    rePtr->nfa[root]->epsilon2 = startR;
    //Push the new end (closure) state and concatenate the two end states to it
    int closure = push();
    concatenate(endL, closure);
    concatenate(endR, closure);
    //Closure will have a dangling epsilon so it can be linked to whatever comes next
    rePtr->nfa[closure]->epsilon1 = -2;
    //Update reference variables
    startL = root;
    endL = closure;
}

//Updates all dangling edges of the left state with the right state, which concatentates the two
//Given a-> and b, perform a -> b
void BaseRegexp::RegexpBuilder::concatenate(int left, int right){
    regexp::State* lptr = rePtr->nfa[left];
    //If the edges are dangling (-2), update with right
    if (lptr->edge == -2)
        lptr->edge = right;
    if (lptr->epsilon1 == -2)
        lptr->epsilon1 = right;
    if (lptr->epsilon2 == -2)
        lptr->epsilon2 = right;
}

//Updates a regexp fragment with the ? unary operator
//Given R -> , produce S -> R -> E -> (dangle)
//                       ------>
void BaseRegexp::RegexpBuilder::optional(int& start, int& end){
    //Push new start and end states
    int newStart = push();
    int newEnd = push();
    //Connect start state to the old start state and the new end state
    rePtr->nfa[newStart]->epsilon2 = start;
    rePtr->nfa[newStart]->epsilon1 = newEnd;
    //Concatenate old end to new end
    concatenate(end, newEnd);
    //New end will have dangling epsilon so it can be connected to what comes next
    rePtr->nfa[newEnd]->epsilon1 = -2;
    //Update reference variables
    start = newStart;
    end = newEnd;
}

//Updates a regexp fragment with the + unary operator
//Given R -> , produce R -> E -> (dangle)
//                       <-
void BaseRegexp::RegexpBuilder::repeating(int& start, int& end){
    //Push new end state
    int newEnd = push();
    //Connect one epsilon edge to the old starting state and the other will dangle so it can be connected later
    rePtr->nfa[newEnd]->epsilon2 = start;
    rePtr->nfa[newEnd]->epsilon1 = -2;
    //Concatenate the old end to the new end
    concatenate(end, newEnd);
    end = newEnd;
}

//Updates a regexp fragment with the * unary operator
//Given R -> , produce SE <- R 
//                        ->
//                        ------> (dangle)
void BaseRegexp::RegexpBuilder::kleene(int& start, int& end){
    //Do same thing as + operator, except start at the new end state
    repeating(start, end);
    start = end;
}

//Parses an entire regexp, handles | operator
void BaseRegexp::RegexpBuilder::parseRegexp(int& startL, int& endL){
    //Parse a concatenation
    parseConcat(startL, endL);
    //For every | operator, parse another concatenation and combine it into the previous one via alternation
    while (next('|')){
        int startR, endR;
        parseConcat(startR, endR);
        alternate(startL, endL, startR, endR);
    }
}

//Parses a concatenation of multiple characters
void BaseRegexp::RegexpBuilder::parseConcat(int& startL, int& endL){
    //Handles the case where a concatenation may contain nothing at all. Results in an empty regexp fragment
    if (!notEnd() || regexp[pos]=='|' || regexp[pos]==')'){
        //The fragment will only consist of one new state with only a dangling epsilon edge
        startL = endL = push();
        rePtr->nfa[startL]->epsilon1 = -2;
        return;
    }
    //Parses a unary fragment
    parseUnary(startL, endL);
    //If no operators or delimeters are seen, keep parsing unary fragments and concatente them to the regexp.
    while (notEnd() && regexp[pos]!='|' && regexp[pos]!=')'){
        int startR, endR;
        parseUnary(startR, endR);
        concatenate(endL, startR);
        endL = endR;
    }
}

//Parses a unary group, consisting of a character and an optional unary operator
void BaseRegexp::RegexpBuilder::parseUnary(int& start, int& end){
    //Parse a basic regexp fragment
    parseValue(start, end);
    //Runs unary operation based on the next unary operator
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

//Parses a regexp fragment that is either a bracketed value or a character
void BaseRegexp::RegexpBuilder::parseValue(int& start, int& end){
    //Parse bracketed expressions as a full regexp
    if (next('(')){
        parseRegexp(start, end);
        if (!next(')')) error();
    }
    else{
        //If there's a square bracket, parse a single-state set
        if (next('['))
            start = parseSet();
        //Otherwise parse a char
        else
            start = parseChar();
        end = start;
    }
}

//Parse a non-escaped character. Throw error for special characters and return the value otherwise
char BaseRegexp::RegexpBuilder::parseC(){
    char c = next(1,regexp::NumOfChars-1);
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

//Parse a single char as a single state and returns the state number
int BaseRegexp::RegexpBuilder::parseChar(){
    //Push a new state representing the char and prepare to update its transitions
    int start = push();
    std::bitset<regexp::NumOfChars>& transition = rePtr->nfa[start]->transitions;
    //If \ is encountered, the next character will be parsed with escaped semantics
    if (next('\\')){
        parseSpecial(transition);
    }
    //Update the transition for everything other than \n for special character .
    else if (next('.')){
        updateFragment(transition, 1, '\n');
        updateFragment(transition, '\n'+1, regexp::NumOfChars-1);
    }
    //Otherwise, parse with unescaped semantics
    else{
        char c = parseC();
        transition[c] = 1;
    }
    return start;
}

//Parse a single-state set and returns state numbers
int BaseRegexp::RegexpBuilder::parseSet(){
    int start = push();
    std::bitset<regexp::NumOfChars>& transition = rePtr->nfa[start]->transitions;
    //Set the inverse flag according to the optional ^ 
    bool inverse = false;
    if (next('^'))
        inverse = true;
    //Parse as many elements as possible before ] is reached
    parseElem(transition);
    while (!next(']')){
        parseElem(transition);
    }
    //Flip the transitions of the new state if inverse is set
    if (inverse){
        transition.flip();
    }
    return start;
}

//Parse an element within a single-state set and updates the transition bitset
void BaseRegexp::RegexpBuilder::parseElem(std::bitset<regexp::NumOfChars>& transition){
    //Escaped chars are parsed as a single char, since they cannot form ranges 
    if (next('\\')){
        parseSpecial(transition);
    }
    //Same for special char .
    else if (next('.')){
        updateFragment(transition, 1, '\n');
        updateFragment(transition, '\n'+1, regexp::NumOfChars-1);
    }
    //None-special chars are parsed as ranges or single chars based on whether they are followed by -
    else{
        char c = parseC();
        if (next('-')){
            char r = parseC();
            updateFragment(transition, c, r);
        }
        else transition[c] = 1;
    }
}

//Parses an escaped char using different semantics 
void BaseRegexp::RegexpBuilder::parseSpecial(std::bitset<regexp::NumOfChars>& transition){
    char c = next(1, regexp::NumOfChars-1);
    switch(c){
        //If nothing follows, simply process the backslash as normal char
        case 0:
            transition['\\'] = 1;
            break;
        //Turn on all numbers
        case 'd':
            updateFragment(transition, '0', '9');
            break;
        //Turn on all non-numbers
        case 'D':
            updateFragment(transition, 1, '0'-1);
            updateFragment(transition, '9'+1, regexp::NumOfChars-1);
            break;
        //Turn on space
        case 's':
            transition[' '] = 1;
            break;
        //Turn on every char other than space
        case 'S':
            updateFragment(transition, 1, ' '-1);
            updateFragment(transition, ' '+1, regexp::NumOfChars-1);
            break;
        //Otherwise, treat it as a normal char
        default:
            transition[c] = 1;
    }
}

//Builds the NFA of the Regexp object by running the parser on the input string string 
//"Returns" start and accepts states of the parsed regexp by reference
void BaseRegexp::RegexpBuilder::build(char* re, BaseRegexp *rp, int &start, int &accept){
    regexp = re;
    rePtr = rp;
    int end;
    parseRegexp(start, end);
    //Add extra accept state at the end to close any dangling edges
    accept = push();
    concatenate(end, accept);
}

//Helper function for the simulation that adds a non-repeating state to a list of states
void BaseRegexp::addState(int state, std::vector<int>& curStates, int *listids, int id){
    if (listids[state] != id){
        listids[state] = id;
        curStates.push_back(state);
    }
}

//Simulates the state machine for a string input, obeying maximal munch
//Returns the accepting state if successful, otherwise return -1. Advances the input pointer to the end of the regexp simulation
int BaseRegexp::simulate(char* &str){
    std::vector<int> curStates;
    std::vector<int> nextStates;
    //Position in string the last time the simulation reached an accept state
    int lastAcceptPos = 0;
    //Number of the last accept state reached
    int lastAcceptState = -1;
    //Inintializes the list of current states with starting state of NFA
    curStates.push_back(starting);

    //list ID array assigns a list ID to each state. 
    //ID is updated with the iteration number of the simulation each time the state is added,
    //and can be referenced later to avoid duplicates
    int *listids = new int[nfa.size()];
    for (int i=0; i<nfa.size(); i++)
        listids[i] = -1;

    //Loop thru every char in the input until no more states can be processed and simulation completely stops
    for (int i=0; !curStates.empty(); i++){
        char c = str[i];
        //Loop thru each current state
        for (int j=0; j<curStates.size(); j++){
            int accept = isAccepting(curStates[j]);
            //If the state is accepting, then update the acceptance variables
            if (accept > -1){
                lastAcceptPos = i;
                lastAcceptState = accept;
            }

            regexp::State *stateptr = nfa[curStates[j]];
            //Valid epsilon edges are added to the current list to be processed in the same iteration
            if (stateptr->epsilon1 >= 0)
                addState(stateptr->epsilon1, curStates, listids, i);
            if (stateptr->epsilon2 >= 0)
                addState(stateptr->epsilon2, curStates, listids, i);
            //Valid char edges are added to the next list to be processed the next iteration
            if (stateptr->transitions[c])
                addState(stateptr->edge, nextStates, listids, i+1);
        }
        curStates.swap(nextStates);
        nextStates.clear();
        //Stop the algorithm if the end of input is reached
        if (str[i] == 0) break;
    }

    delete[] listids;
    //Set input pointer to the location where the parse stopped
    str += lastAcceptPos;
    return lastAcceptState;
}

BaseRegexp::~BaseRegexp(){
    for (int i=0; i<nfa.size(); i++){
        delete[] nfa[i];
    }
    starting = -1;
}

//Cout overload that prints all states and transitions of NFA plus start state
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Regexp class has a single accepting state, so any input state must match it to be accepting
int Regexp::isAccepting(int state){
    if (state == accepting)
        return accepting;
    return -1;
}

//Matches Regexp to the beginning of input and returns the # of chars that were matched. Returns -1 if nothing was matched
int Regexp::match(char* str){
    char *end = str;
    int success = simulate(end);
    if (success < 0)
        return -1;
    return end-str;
}

//Runs simulation thru each position of the input to look for matches. Returns start position of first match, -1 if no match
int Regexp::search(char* str){
    for (int i=0; str[i]!=0; i++){
        char *end = str+i;
        int success = simulate(end);
        if (success >= 0){
            return i;
        }
    }
    return -1;
}

//Constructor builds regexp once
Regexp::Regexp(char* re){
    RegexpBuilder builder;
    builder.build(re, this, starting, accepting);

}
//Default constructor builds empty regexp
Regexp::Regexp(){
    RegexpBuilder builder;
    builder.build("", this, starting, accepting);  
}

//Regexp cout overload adds the accept state
std::ostream& operator<<(std::ostream& os, const Regexp& regexp){
    os << (const BaseRegexp&)regexp << "Accepting: " << regexp.accepting << '\n';
    return os;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Lexer has multiple accept states, so the accept state table is queried to see which the regexp number the acceptance corresponds with
int Lexer::isAccepting(int state){
    return acceptTable[state];
}

//Lexer builds multiple regexps into a large NFA with multiple accepts. Can also specify a regexp to ignore
Lexer::Lexer(char* regexplist[], int len, int ignoreNum=-1){
    //Invalid construction for empty regexp list
    if (len == 0){
        //Offset destructor
        acceptTable = new int[0];
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

    delete[] acceptList;
    ignore = ignoreNum;
}

int Lexer::lex(char* input, std::vector<Token> &tokens){
    char* curpos = input;
    while (*curpos != 0){
        //Start position of token
        int start = curpos-input;
        //Run simulation, which advances the curpos pointer and produces the token id
        int tokenNum = simulate(curpos);
        //End position of token
        int end = curpos-input;
        //Lexer fails if simulation fails or an empty token is produced
        if (tokenNum == -1 || start == end) return input-curpos;
        //If the token is ignored. skip it
        if (tokenNum == ignore) continue;
        //Push token onto stack
        tokens.push_back({tokenNum, start, end});
    }
    return 1;
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

//Token cout overload prints all members
std::ostream& operator<<(std::ostream& os, const Token& token){
    os << token.id << "->" << token.start << ':' << token.end;
    return os;
}

// int main(int argc, char* argv[]){
//     Regexp regexp = Regexp();
//     std::cout << regexp << regexp.match(argv[2]);
//     Lexer lexer = Lexer(argv+1, 2, 0);
//     std::cout << lexer;
//     std::vector<Token> tokens;
//     int good = lexer.lex(argv[3], tokens);
//     for (int i=0; i<tokens.size(); i++){
//         std::cout << tokens[i] << ' ' ;
//     }
//     std::cout << good;
// }