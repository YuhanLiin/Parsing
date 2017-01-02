opFunctions = {'+': lambda x, y: x+y,
       '-': lambda x, y: x-y,
       '/': lambda x, y: x/y,
       '*': lambda x, y: x*y,
       '^': lambda x, y: x**y, }

highestPrec = 2
precedence = {'+':0 , '-':0 , '/':1, '*':1, '^':2}
leftAssoc = {'+':True , '-':True , '/':True, '*':True, '^':False}

# Corresponds to this grammar

# Start -> Exp ;
# 0 Exp -> Term [ (+|-) Term ]* ;
# 1 Term -> Factor [ (*|/) Factor ]* ;
# 2 Factor -> Value [ ^ Factor ]* | - Factor;
# 3 Value -> int | ( Exp );

class Parser():
    def __init__(self, tokens):
        #External variables for the position of the parser in the token stream and the token stream
        self.pos = 0
        self.tokens = tokens

    def parse(self):
        self.pos = 0
        result = self.evalExp(0)
        if self.pos != len(self.tokens)-1:
            raise Exception("ParseError, pos = "+str(self.pos))
        else:
            return result

    def getToken(self, i=0):
        return self.tokens[self.pos+i]

    # Recursive descent parsing routine for expressions via precedence climbing
    def evalExp(self, minPrec):
        #If a bracket is reached, parse the tokens after it with the lowest precedence and assign it to leftVal
        if highestPrec < minPrec:
            if self.getToken() == '(':
                self.pos += 1
                bracketVal = self.evalExp(0)
                self.pos += 1
                return bracketVal
            else:
                return float(self.getToken())

        if minPrec == 2 and self.getToken() == '-':
                self.pos += 1
                return -self.evalExp(2)

        leftVal = self.evalExp(minPrec+1)
        # Keep parsing the next operator and the next operand of the minimum precedence until the end or a lower precedence operator is reached
        while self.pos < len(self.tokens)-1 and self.getToken(1) in precedence and precedence[self.getToken(1)] == minPrec:
            op = self.getToken(1)
            self.pos += 2
            # Parse at higher precedence for left-associative operators; same precedence for right associative operators
            if leftAssoc[op]:
                rightVal = self.evalExp(minPrec+1)
            else:
                rightVal = self.evalExp(minPrec)
            # Reduce the left and right operands into a new left operand each time
            leftVal = float(opFunctions[op](leftVal, rightVal))
        return leftVal

# tokens = " - ( 5 - ( 3 - 1 ) ^ 2 ) * 7 + ( 5 - ( 3 - 1 ) ^ 2 ) * 7 ".split()
# print Parser(tokens).parse()

class BacktrackParser():
    def __init__(self, tokens):
        #External variables for the position of the parser in the token stream and the token stream
        self.pos = 0
        self.tokens = tokens

    def parse(self):
        self.pos = 0
        result = self.evalExp()
        if result == None or self.pos != len(self.tokens):
            raise Exception("ParseError, pos = "+str(self.pos))
        else:
            return result

    def next(self, t):
        if self.pos < len(self.tokens) and self.tokens[self.pos] in t:
            self.pos += 1
            return True
        return False

    def evalExp(self):
        leftVal = self.evalTerm()
        if leftVal == None:
            return None
        while self.next('+') or self.next('-'):
            op = self.tokens[self.pos-1]
            rightVal = self.evalTerm()
            if rightVal == None:
                return None
            if op =='+':
                leftVal += rightVal
            else:
                leftVal -= rightVal
        return leftVal

    def evalTerm(self):
        leftVal = self.evalFactor()
        if leftVal == None:
            return None
        while self.next('*') or self.next('/'):
            op = self.tokens[self.pos-1]
            rightVal = self.evalFactor()
            if rightVal == None:
                return None
            if op=='*':
                leftVal *= rightVal
            else:
                leftVal /= rightVal
        return leftVal

    def evalFactor(self):        
        if self.next('-'):
            value = self.evalFactor()
            if value:
                return -value

        leftVal = self.evalValue()
        if leftVal == None:
            return None
        while self.next('^'):
            rightVal = self.evalFactor()
            if rightVal == None:
                return None
            leftVal **= rightVal
        return leftVal

    def evalValue(self):
        if self.next('('):
            value=self.evalExp()
            if value and self.next(')'):
                return value
        if self.next('1234567890'):
            return int(self.tokens[self.pos-1])  

tokens = "  ( 5 - ( 3 - 1 ) ^ 2 ) * 7 + ( 5 - ( 3 - 1 ) ^ 2 ) * 7 ".split()
print BacktrackParser(tokens).parse()