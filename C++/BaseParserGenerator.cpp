#include "BaseParserGenerator.h"

//Error constructor takes a message
GrammarConfigError::GrammarConfigError(char *msg){
    str = msg;
}
//Error outputs a message
const char* GrammarConfigError::what(){
    return str;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

//Gtoken constructor initializes tracker pointers
BaseParserGenerator::GrammarParser::GrammarParser(BaseParserGenerator *p, char *grammarConfig){
    parser = p;
    curpos = grammarConfig;
    prevpos = curpos;
}

//Convert gtoken enum into string form
const char* BaseParserGenerator::GrammarParser::gtokenName(Gtoken::Gtoken gtoken){
    if (gtoken == Gtoken::INVALID) return "INVALID TOKEN";
    const char* const name[11] = {"NEWLINES", "SPACES", "NONTERMINAL", "TERMINAL", "LEFT BRACE", "RIGHT BRACE", 
        "CHAR", "COLON", "PIPE", "SEMICOLON", "STAR"};
    return name[gtoken];
}
//Error for unexpected token
void BaseParserGenerator::GrammarParser::error(Gtoken::Gtoken gtoken){
    std::cerr << "Grammar config error at line " << lexer.tokenLine << " position " << lexer.tokenCol << 
    " : Unexpected token " << gtokenName((Gtoken::Gtoken)lexer.tokenID) << " encountered instead of token " << gtokenName(gtoken) << "\n";
    throw GrammarConfigError("Grammar Config Error");
}
//Error with custom message
void BaseParserGenerator::GrammarParser::error(char *message){
    std::cerr << "Grammar config error at line " << lexer.tokenLine << " position " << lexer.tokenCol << " : " << message << '\n';
    throw GrammarConfigError("Grammar Config Error");
}

//Get the next token from the grammar config string. Ignore newlines and spaces
void BaseParserGenerator::GrammarParser::next(){
    do{
        prevpos = curpos;
        curpos = lexer.lex(curpos);
        //std::cout << lexer.tokenID << ' ' << lexer.tokenCol << ' ' << (*prevpos) << '\n';
    } while((lexer.tokenID == 0 || lexer.tokenID == 1) && prevpos != curpos);
    // Quits when empty token is encountered, as it signifies an incorrect input
}

//Gets and asserts that the next token is equal to the parameter token
void BaseParserGenerator::GrammarParser::next(Gtoken::Gtoken gtoken){
    next();
    if (lexer.tokenID != gtoken) 
        error(gtoken);
}

//Returns whether the current token equals the parameter. Does not advance input
bool BaseParserGenerator::GrammarParser::tokenIs(Gtoken::Gtoken gtoken){
    return lexer.tokenID == gtoken;
}

//Extract the lexeme of the current token by reference
void BaseParserGenerator::GrammarParser::getWord(std::string &word){
    for (char *c = prevpos; c < curpos; c++){
        word.push_back(*c);
    }
}

//Replace each instance of a placeholder symbol # in a grammar with a new symbol #. Called when a symbol is found in lhs.
void BaseParserGenerator::GrammarParser::replaceSymbol(int old, int replacement){
    for (int i=0; i<parser->grammar.size(); i++){
        if (parser->grammar[i] == old){
            parser->grammar[i] = replacement;
        }
    }
}

// Parses the optional Token Declaration section
void BaseParserGenerator::GrammarParser::parseTokens(){
    next();
    if(tokenIs(Gtoken::LBRAC)){
        while(true) {
            next();
            // For token, extract and map the name to the token number 
            if (tokenIs(Gtoken::TRML)){
                std::string str;
                getWord(str);
                symbolTable[str] = tokenNum;
                // Makes sure the vector is same length as token number.
                parser->tokenIgnore.push_back(0);
            }
            // For stars (*), mark the current token number as ignored
            else if (tokenIs(Gtoken::STAR)){
                parser->tokenIgnore.push_back(1);
            }
            else{
                break;
            }
            // Increment the token number for subsequent tokens
            tokenNum++;
        }
        if (!tokenIs(Gtoken::RBRAC)) error(Gtoken::RBRAC);
        // Shift input here to achieve similar semantics as the tokenless case
        next();
    }
}

// Parses entire grammar rule and registers it into the internal representation
// Responsible for granting the lhs symbol a non-placeholder (+ve) symbol #
void BaseParserGenerator::GrammarParser::parseRule(){
    if (!tokenIs(Gtoken::NTRML)) error(Gtoken::NTRML);
    std::string lhs;
    getWord(lhs);
    //If there already exists a previous instance of the lhs nonterminal mapped to a placeholder, replace it with the newly derived symbol number
    int lhsnum = symbolNumber(lhs);
    if (lhsnum < 0){
        replaceSymbol(lhsnum, ruleNum);
    }
    //If the nonterminal has already appeared in another lhs, raise error
    else if (lhsnum > 0){
        error("Cannot have rules with duplicate left hand symbols");
    }

    //Map the number of the left hand nonterminal to the position in the grammar vector where its rules start
    parser->ruleNumStart.push_back(parser->grammar.size());
    //Maps lhs nonterminal string to number
    symbolTable[lhs] = ruleNum;
    next(Gtoken::COLON);

    //Parse as many pipe-separated productions as possible before ending the rule with a semicolon
    do {
        parseProduction();
    } while(tokenIs(Gtoken::PIPE));
    if (!tokenIs(Gtoken::SCOLON)) error(Gtoken::SCOLON);
    //The next rule/lhs nonterminal is assigned an increased number
    ruleNum++;
}

// Processes a single production
void BaseParserGenerator::GrammarParser::parseProduction(){
    //The first number of a production will be its # of rhs symbols. Increments as production is parsed
    parser->grammar.push_back(0);
    int countIndex = parser->grammar.size()-1;
    while(true) {
        //Repeatedly obtain tokens
        next();
        std::string rhs;
        getWord(rhs);
        //Chars are added to the production as is
        if (tokenIs(Gtoken::CHR)){
            parser->grammar.push_back(rhs[1]);
        }
        //Terminal symbols are assigned their numbers from the symbol table. Error raised if symbol doesn't exist
        else if (tokenIs(Gtoken::TRML)){
            int num = symbolNumber(rhs);
            if (!num) error("The terminal symbol does not exist in the Token Declaration");
            parser->grammar.push_back(num);
        }
        //Nonterminal symbol is either assigned its # from symbol table or given a -ve placeholder # that's replaced later 
        else if (tokenIs(Gtoken::NTRML)){
            int rhsnum = symbolNumber(rhs);
            if (rhsnum){
                parser->grammar.push_back(rhsnum);
            }
            else{
                //Map placeholder # to the symbol and push it back. Decrement the placeholder, since its -ve.
                symbolTable[rhs] = unfoundRuleNum;
                parser->grammar.push_back(unfoundRuleNum);
                unfoundRuleNum--;
            }
        }
        else{
            return;
        }
        //Increase the symbol count
        parser->grammar[countIndex]++;
    }
}

// Parses whole config string
void BaseParserGenerator::GrammarParser::parseGrammar(){
    parseTokens();
    // Initialize the nonterminal number
    ruleNum = tokenNum;
    // Parse rules until there are no more
    do{
        parseRule();
        // Shift because parseRule() assumes that its first token has already been advanced
        next();
    } while (*curpos != 0);

    for (auto word : symbolTable){
        if (word.second < 0){
            std::cerr << "Grammar config error: Nonterminal symbol '" << word.first << "' does not appear on the left side of any rule.";
            throw GrammarConfigError("Grammar Config Error");
        }
    }
}

// Returns symbol's symbol #. Returns 0 if symbol is not in table
int BaseParserGenerator::GrammarParser::symbolNumber(std::string &symbol){
    if (symbolTable.count(symbol) == 0)
        return 0;
    else
        return symbolTable[symbol];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Constructor calls on grammar parser
BaseParserGenerator::BaseParserGenerator(char *grammarConfig, Lexer *lex){
    GrammarParser gparser{this, grammarConfig};
    gparser.parseGrammar();
    // Pads the symbol-to-grammar-position mapping to simplify looping operations
    ruleNumStart.push_back(grammar.size());
    // Assign the token and rule numbers
    tokenNum = tokenIgnore.size() + NumOfChars;
    ruleNum = toRuleNum(ruleNumStart.size()-1);
    //Assign lexer
    lexptr = lex;
}

//Add the rhs symbols of a production to a stack
void BaseParserGenerator::addProduction(int ruleStart, std::vector<int>& stack, bool reverse=false){
    int start = ruleStart+1;
    int end = grammar[ruleStart] + ruleStart;
    if (!reverse){
        for (int i=start; i<=end; i++){
            stack.push_back(grammar[i]);
        }
    }
    else{
        for (int i=end; i>=start; i--){
            stack.push_back(grammar[i]);
        }
    }
}

//Go to starting pos of next production in grammar
int BaseParserGenerator::nextProduction(int ruleStart){
    return ruleStart + grammar[ruleStart] + 1;
}

//Return start of lhs rule in grammar
int BaseParserGenerator::ruleStart(int symbol){
    return ruleNumStart[symbol];
}

//Return the token or char
int BaseParserGenerator::next(){
    //Skip all ignored tokens. Stop when empty token is encountered
    do {
        prevpos = curpos;
        curpos = lexptr->lex(curpos);
    } while(prevpos!=curpos && tokenIgnore[lexptr->tokenID]);
    //If no actual token is available or if token is empty, advance the input by 1 and return the char
    if (lexptr->tokenID < 0 || prevpos==curpos){
        curpos++;
        lexptr->tokenCol++;
        return (int)(*prevpos);
    } 
    //Otherwise, return the incremented token number
    return lexptr->tokenID + NumOfChars;
}

//Get contents of current token
void BaseParserGenerator::getWord(std::string &word){
    for (char* i=prevpos; i<curpos; i++){
        word.push_back(*i);
    }
}

//Convert to and from incremented/nonincremented rule number
int BaseParserGenerator::toRuleCount(int ruleNum){
    return ruleNum - tokenNum;
}
int BaseParserGenerator::toRuleNum(int ruleCount){
    return ruleCount + tokenNum;
}

//TokenNum is the start of rule numbers, so check if symbol is below it
bool BaseParserGenerator::isTerminal(int symbol){
    return symbol < tokenNum;
}

//Return lexer's column and line numbers
int BaseParserGenerator::colNum(){
    return lexptr->tokenCol-1;
}
int BaseParserGenerator::lineNum(){
    return lexptr->tokenLine;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

//Iostream overload that prints the internal grammar as numbers
std::ostream& operator<<(std::ostream& os, BaseParserGenerator& parser){
    for (int i=0; i<parser.ruleNumStart.size()-1; i++){
        int j = parser.ruleNumStart[i];
        int end = parser.ruleNumStart[i+1];
        while (j < end){
            os << parser.toRuleNum(i) << " : ";
            int rhsLimit = j + parser.grammar[j];
            j++;
            for (j; j <= rhsLimit; j++){
                os << parser.grammar[j] << ' ';
            }
            os << ";\n";
        }
    }
    os << "Ignored: ";
    for (int i=0; i<parser.tokenIgnore.size(); i++){
        os << (int)parser.tokenIgnore[i] << ' ';
    }
    os << '\n';
    return os;
}
