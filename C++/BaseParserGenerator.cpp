#include "BaseParserGenerator.h"

BaseParserGenerator::GrammarParser::GrammarParser(BaseParserGenerator *p, char *grammarConfig){
    parser = p;
    curpos = grammarConfig;
    prevpos = curpos;
}

void BaseParserGenerator::GrammarParser::error(Gtoken token){
    std::cerr << "Grammar config error at line " << lexer.tokenLine << " position " << lexer.tokenCol << 
    " : Unexpected token '" << lexer.tokenID << "' encountered instead of token '" << token << "'\n";
    throw 0;
}

void BaseParserGenerator::GrammarParser::next(){
    do{
        prevpos = curpos;
        curpos = lexer.lex(curpos);
        //std::cout << lexer.tokenID << ' ' << lexer.tokenCol << ' ' << (*prevpos) << '\n';
    } while((lexer.tokenID == 0 || lexer.tokenID == 1) && prevpos != curpos);
}

void BaseParserGenerator::GrammarParser::next(Gtoken gtoken){
    next();
    if (lexer.tokenID != gtoken) 
        error(gtoken);
}

bool BaseParserGenerator::GrammarParser::tokenIs(Gtoken gtoken){
    return lexer.tokenID == gtoken;
}

void BaseParserGenerator::GrammarParser::getWord(std::string &word){
    for (char *c = prevpos; c < curpos; c++){
        word.push_back(*c);
    }
}

void BaseParserGenerator::GrammarParser::replaceSymbol(int old, int replacement){
    for (int i=0; i<parser->grammar.size(); i++){
        if (parser->grammar[i] == old){
            parser->grammar[i] = replacement;
        }
    }
}

void BaseParserGenerator::GrammarParser::parseTokens(){
    next();
    if(tokenIs(LBRAC)){
        while(true) {
            next();
            if (tokenIs(TRML)){
                std::string str;
                getWord(str);
                symbolTable[str] = tokenNum;
                parser->tokenIgnore.push_back(0);
            }
            else if (tokenIs(STAR)){
                parser->tokenIgnore.push_back(1);
            }
            else{
                break;
            }
            tokenNum++;
        }
        if (!tokenIs(RBRAC)) error(RBRAC);
    }
}

void BaseParserGenerator::GrammarParser::parseRule(){
    if (!tokenIs(NTRML)) error(NTRML);
    std::string lhs;
    getWord(lhs);
    //If there already exists a previous instance of this nonterminal mapped to a placeholder, replace it with the newly derived symbol number
    int lhsnum = symbolNumber(lhs);
    if (lhsnum){
        replaceSymbol(lhsnum, ruleNum);
    }
    //Map the number of the left hand nonterminal to the position in the grammar vector where its rules start
    parser->ruleNumStart.push_back(parser->grammar.size());
    //Maps lhs nonterminal string to number
    symbolTable[lhs] = ruleNum;
    next(COLON);

    //Parse as many pipe-separated productions as possible before ending the rule with a semicolon
    do {
        parseProduction();
    } while(tokenIs(PIPE));
    if (!tokenIs(SCOLON)) error(SCOLON);
    //The next rule/lhs nonterminal is assigned an increased number
    ruleNum++;
}

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
        if (tokenIs(CHR)){
            parser->grammar.push_back(rhs[1]);
        }
        //Terminal symbols are assigned their numbers from the symbol table. Error raised if symbol doesn't exist
        else if (tokenIs(TRML)){
            int num = symbolNumber(rhs);
            if (!num) error(TRML);
            parser->grammar.push_back(num);
        }
        //Nonterminal symbol is either assigned its # from symbol table or given a -ve placeholder # that's replaced later 
        else if (tokenIs(NTRML)){
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
        parser->grammar[countIndex]++;
    }
}

void BaseParserGenerator::GrammarParser::parseGrammar(){
    parseTokens();
    next();
    ruleNum = tokenNum + 1;
    do{
        parseRule();
        next();
    } while (*curpos != 0);
}

int BaseParserGenerator::GrammarParser::symbolNumber(std::string &symbol){
    if (symbolTable.count(symbol) == 0)
        return 0;
    else
        return symbolTable[symbol];
}

BaseParserGenerator::BaseParserGenerator(char *grammarConfig){
    GrammarParser gparser = GrammarParser(this, grammarConfig);
    gparser.parseGrammar();
    ruleNumStart.push_back(grammar.size());
    tokenNum = gparser.tokenNum;
}

std::ostream& operator<<(std::ostream& os, const BaseParserGenerator& parser){
    for (int i=0; i<parser.ruleNumStart.size()-1; i++){
        int j = parser.ruleNumStart[i];
        int end = parser.ruleNumStart[i+1];
        while (j < end){
            os << i+parser.tokenNum+1 << " : ";
            int rhsLimit = j + parser.grammar[j];
            j++;
            for (j; j <= rhsLimit; j++){
                os << parser.grammar[j] << ' ';
            }
            os << ";\n";
        }
    }
    return os;
}

int main()
{
    char *grammar = R"({ NAME NUM CHAR BRAC *}
    exp : NUM BRAC 'a' | CHAR CHAR abc exp abc;
    abc : BRAC exp |; 
)";

    BaseParserGenerator parser = BaseParserGenerator(grammar);
    std::cout << parser;
    return 0;
}