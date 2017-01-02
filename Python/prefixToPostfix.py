ops = ['+', '-', '*', '/']

def preToPost(tokens):
    stack = []

    #Recursively converts a section of the token input corresponding to a prefix expression into a postfix expression on the stack
    #Takes as input the token stream start position of target expression and outputs the position after the end of the expression
    def convert(start):
        if tokens[start] not in ops:
            stack.append(tokens[start])
            return start+1
        else:
            op = tokens[start]
            firstValEnd = convert(start+1)
            secondValEnd = convert(firstValEnd)
            stack.append(op)
            return secondValEnd

    end = convert(0)
    if end == len(tokens):
        return stack
    else:
        raise Exception("Something's wrong.")

tokens = raw_input().split()
print ' '.join(preToPost(tokens))
