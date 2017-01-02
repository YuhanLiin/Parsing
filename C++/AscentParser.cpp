#include <iostream>

// Grammar:
// S' -> S $ ;
// S -> < S | M ;
// M -> < M > | < > ;

//Reference parameter "reduction" indicates which symbol is being returned down the stack
//Return value is the depth that needs to be popped off the call stack. Only calls at depth 0 should process symbol as goto
class AscentParser{
private:
    int pos = 0;
    char *tokens;
    bool bad = false;

    bool next(char c){
        if (tokens[pos] == c){
            pos++;
            return true;
        }
        return false;
    }

    // S' -> .S $ 
    // -----------
    // S -> .< S 
    // S -> .M (automatically returns S)
    // M -> .< M > 
    // M -> .< > 
    int start(char &reduction){
        int depth;
        if (next('<')){
            depth = s_m_rb(reduction);
        }
        else{
            reduction = 0;
            return 100;
        }
        if (depth == 0)
            return end(reduction)-1;
        return depth-1;
    }

    // S -> < .S 
    // M -> < .M > 
    // M -> < .> 
    // ------------
    // S -> .< S 
    // S -> .M
    // M -> .< M > 
    // M -> .< > 
    int s_m_rb(char &reduction){
        int depth;
        if (next('<'))
            depth = s_m_rb(reduction);
        else if (next('>')){
            reduction = 'M';
            return 0;
        }
        else{
            reduction = 0;
            return 100;
        }
        while (depth == 0){
            if (reduction == 'S') 
                return 0;
            else
                depth = rb_m(reduction);
        }
        return depth-1;
    }

    // M -> < M .>
    // S -> M .
    int rb_m(char &reduction){
        if (next('>')){
            reduction = 'M';
            return 1;
        }
        reduction = 'S';
        return 0;
    }

    // S' -> S .$ (accept if at end)
    int end(char &reduction){
        if (next(0)){
            reduction = 'A';
            return 1;
        }
    }

public:
    //Runs start() and checks if the symbol passed back is accept or error. If there's an error, return the position of error
    //Otherwise return -1
    int parse(char input[]){
        tokens = input;
        char state;
        start(state);
        if (state == 'A'){
            return -1;
        }
        return pos;
    }
};

int main(int argc, char *argv[]){
    AscentParser parser = AscentParser();
    int fail = parser.parse(argv[1]); 
    if (fail != -1)
        std::cerr << "Parse error at " << fail;
    else{
        std::cout << "Valid!";
    } 
}

