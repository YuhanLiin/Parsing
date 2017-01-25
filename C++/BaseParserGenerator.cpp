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
        Rule &curRule = parser->grammar[i];
        for (int j=0; j<curRule.size(); j++){
            if (curRule[j] == old){
                curRule[j] = replacement;
            }
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
    int lhsnum = symbolNumber(lhs);
    if (lhsnum){
        replaceSymbol(lhsnum, ruleNum);
    }
    parser->ruleNumStart.push_back(parser->grammar.size());
    symbolTable[lhs] = ruleNum;
    next(COLON);

    do {
        parser->grammar.push_back(Rule());
        Rule &production = parser->grammar.back();
        production.push_back(ruleNum);
        parseProduction(production);
    } while(tokenIs(PIPE));
    if (!tokenIs(SCOLON)) error(SCOLON);
    ruleNum++;
}

void BaseParserGenerator::GrammarParser::parseProduction(Rule &production){
    while(true) {
        next();
        std::string rhs;
        getWord(rhs);
        if (tokenIs(CHR)){
            production.push_back(rhs[1]);
        }
        else if (tokenIs(TRML)){
            int num = symbolNumber(rhs);
            if (!num) error(TRML);
            production.push_back(num);
        }
        else if (tokenIs(NTRML)){
            int rhsnum = symbolNumber(rhs);
            if (rhsnum){
                production.push_back(rhsnum);
            }
            else{
                symbolTable[rhs] = unfoundRuleNum;
                production.push_back(unfoundRuleNum);
                unfoundRuleNum--;
            }
        }
        else
            return;
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
}

std::ostream& operator<<(std::ostream& os, const BaseParserGenerator& parser){
    for (int i=0; i<parser.grammar.size(); i++){
        const BaseParserGenerator::Rule &rule = parser.grammar[i];
        os << rule[0] << " : ";
        for (int j=1; j<rule.size(); j++){
            os << rule[j] << ' ';
        }
        os << '\n';
    }
    return os;
}

int main()
{
    char *grammar = R"({ NAME NUM CHAR BRAC }
    exp : NUM BRAC 'a' | CHAR CHAR abc;
    abc : BRAC exp |; 
    d : )";

    BaseParserGenerator parser = BaseParserGenerator(grammar);
    std::cout << parser;
    return 0;
}