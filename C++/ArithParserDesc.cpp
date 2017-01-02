#include <iostream>
#include <cmath>

// Corresponds to this grammar

// Start -> Exp ;
// 0 Exp -> Term [ (+|-) Term ]* ;
// 1 Term -> Factor [ (*|/) Factor ]* ;
// 2 Factor -> Value [ ^ Factor ]* | - Factor;
// 3 Value -> Intpart [ . Fractpart ]? | ( Exp );
// 4 Intpart -> [0-9]+
// 5 Fractpart -> [0-9]*

class ArithParser{
private:
    int pos = 0;
    char *tokens;
    bool bad = false;

    char next(char c){
        if (tokens[pos] == c){
            pos++;
            return c;
        }
        return 0;
    }

    char next(char low, char high){
        if (tokens[pos] >= low && tokens[pos] <= high){
            pos++;
            return tokens[pos-1];
        }
        return 0;
    }

    void error(){
        bad = true;
        throw "0";
    }

    //Parses integer part of a float number and computes the value
    float intPart(){
        //std::cout << pos << '\n';
        float num = 0;
        char c = next('0', '9');
        //std::cout << pos << ' ' << c << '\n';
        if (c){
            num = c - '0';
        }
        else{
            //std::cout << pos << '\n';
            error();
        }
        while (c = next('0', '9')){
            num *= 10;
            num += c - '0';
        }
        return num;
    }

    //Parses optional fractional part of float number and calculates value
    float fractPart(){
        float num = 0;
        float factor = 0.1;
        char c;
        // Divide each subsequent digit by 10
        while (c = next('0', '9')){
            num += (c - '0')*factor;
            factor /= 10;
        }
        return num;
    }

    //Parses a number, consisting of an intpart and an optional .floatpart
    float number(){
        float ip = intPart();
        if (next('.')){
            float fp = fractPart();
            ip += fp;
        }
        return ip;
    }

    //Parses a value, which is either a number or a bracketed expression
    float value(){
        if (next('(')){
            float num = exp();
            if (!next(')')) error();
            return num;
        }
        else 
            return number();
    }

    //Parses factor, which is either a -ve factor or a base and exponent
    float factor(){
        if (next('-')){
            return -factor();
        }
        else{
            float base = value();
            if (next('^')){
                base = pow(base, factor());
            }
            return base;
        }
    }

    //Parses sequence of factors linked by * or / signs 
    float term(){
        float num1 = factor();
        char op;
        while ((op = next('*')) || (op = next('/'))){
            float num2 = factor();
            if (op == '*')
                num1 *= num2;
            else
                num1 /= num2;
        }
        return num1;
    }

    //Parses sequence of terms linked by + or - signs
    float exp(){
        float num1 = term();
        char op;
        while ((op = next('+')) || (op = next('-'))){
            float num2 = term();
            if (op == '+')
                num1 += num2;
            else
                num1 -= num2;
        }
        return num1;
    }

public:
    //Parses input as expression and returns numerical result. Catches any errors and sets the bad flag on doing so
    float parse(char input[]){
        pos = 0;
        bad = false;
        tokens = input;
        float result;
        try {
            result = exp();
        }
        catch (char *) {
            result = 0;
        }
        if (tokens[pos] != 0){
            bad = true;
        }
        return result;
    }

    //Returns position of the parser error
    int failed(){
        if (bad){
            return pos;
        }
        return -1;
    }
};

int main(int argc, char *argv[]){
    ArithParser parser = ArithParser();
    float ans = parser.parse(argv[1]); 
    int fail = parser.failed();
    if (fail != -1)
        std::cerr << "Parse error at " << fail;
    else{
        std::cout << "Answer: " << ans;
    } 
}