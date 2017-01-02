from grammar import *

# Represents LR state items with dot
class Item(object):
    def __init__(self, lhs, production, dotpos):
        self.lhs = lhs
        self.production = production
        self.dotpos = dotpos

    # Returns the symbol of the production right after the dot, and None if the dot is at the end of the symbol
    def curSymbol(self):
        if len(self.production) <= self.dotpos:
            return None
        return self.production[self.dotpos]

    # Returns copy of the item shifted forward by one
    def advance(self):
        return Item(self.lhs, self.production, self.dotpos+1)

    def __str__(self):
        return self.lhs+" -> "+' '.join(self.production)+' '+str(self.dotpos)

    __repr__ = __str__

    def __eq__(self, other):
        return self.lhs == other.lhs and self.production == other.production and self.dotpos == other.dotpos

    def __hash__(self):
        return hash((self.lhs, tuple(self.production), self.dotpos))

# LR(0) parser generator
class LRParser(object):
    # Parser prepares LR parse table for input grammar upon initialization 
    def __init__(self, grammar, startSymbol):
        self.grammar = grammar
        self.startSymbol = startSymbol
        self.nonterminals = set([])
        self.terminals = set([])
        self.states = []
        self.table = []
        self.readSymbols()
        self.makeTable()

    # Produces symbol list of terminals and non-terminals. Non-terminals are defined as symbols that appear on the LHS of grammar
    def readSymbols(self):
        for rule in self.grammar.rules:
            self.nonterminals.add(rule.lhs)
        for rule in self.grammar.rules:
            for production in self.grammar.getProductions(rule.lhs):
                for symbol in production:
                    if symbol not in self.nonterminals: self.terminals.add(symbol)

    # Parse states are (set, list) pairs that contain the kernel and closure item sets respectively
    # Generator iterates across the kernel then the closure of the state
    def enumItems(self, state):
        for item in state[0]:
            yield item
        for item in state[1]:
            yield item

    # Creates a row in the parse table using the symbol lists. Values default to None, which represents error
    def makeEntry(self):
        entry = {}
        for x in self.terminals:
            entry[x] = None
        for x in self.nonterminals:
            entry[x] = None
        return entry

    # Runs the closure operation on the kernel items of a state that has not been closed
    def closure(self, state):
        closed = set([])
        for item in self.enumItems(state):
            symbol = item.curSymbol()
            # Each non-terminal symbol can only be closed once
            if symbol != None and symbol in self.nonterminals and symbol not in closed:
                closed.add(symbol)
                # Add all productions that derive from the closed symbol to the closure set as new items
                for production in self.grammar.getProductions(symbol):
                    state[1].append(Item(symbol, production, 0))

    # Shifts the items in a state for a particular symbol to a new or existing state
    def shift(self, state, symbol):
        newSet = set([])
        # Build a new kernel set that corresponds to the shift of the input symbol on the input state
        for item in self.enumItems(state):
            if item.curSymbol() == symbol:
                newSet.add(item.advance())
        # See if the new kernel set matches any of the kernel sets of old states in order to avoid repeated states.
        # If not, add a new state with the new kernel
        for i in xrange(len(self.states)):
            if self.states[i][0] == newSet:
                return i
        self.states.append((newSet, []))
        # Return the number of the state corresponding to the shift (the matching state or the new state)
        return len(self.states)-1

    # Modifies a table row that corresponds to a reducing state. 
    # Make every column a tuple indicating the production to be reduced unless it is already occupied.
    # This prioritizes shifts in shift-reduce conflicts
    def reduce(self, entry, item):
        for i in entry:
            if entry[i] == None:
                entry[i] = (item.lhs, item.production)

    # Build the parse states and the tables for the grammar using the above helper functions
    def makeTable(self):
        # Initialize the State 0 kernel set with a closure of the start symbol
        startSet = set([])
        for production in self.grammar.getProductions(self.startSymbol):
            startSet.add(Item(self.startSymbol, production, 0))
        self.states.append((startSet, []))
        # Loop through each parse state, process all items, and create corresponding table row until no more are added
        for state in self.states:
            # Run closure on each state
            self.closure(state)
            shifted = set([])
            entry = self.makeEntry()
            # Loop through each item of the state. 
            for item in self.enumItems(state):
                # If the item has its dot at the end, the state becomes a reduction state
                if item.curSymbol() == None:
                    self.reduce(entry, item)      
                # Otherwise, shift for the item's current symbol if it hasn't yet been shifted and update the table row       
                elif item.curSymbol() not in shifted:
                    transition = self.shift(state, item.curSymbol())
                    entry[item.curSymbol()] = transition
                    shifted.add(item.curSymbol())
            self.table.append(entry)
            #print state

    # Parsing algorithm that traverses the parse table based on the input tokens
    def parse(self, tokens):
        # Begin parse with the starting state on the parse stack
        stack = [0]
        curState = 0
        # Add extra '$' sign to the token to mark the end of file
        tokens += ['$']
        i = 0
        # While no error occurs
        while curState != None:
            # Query the table with the current state and the current token for next action
            action = self.table[curState][tokens[i]]
            # Shift
            if isinstance(action, int):
                # If the token shifted is the eof marker, then the parse is complete
                if tokens[i] == '$':
                    return True
                # Update the current state, add it to the stack, and shift the token stream
                curState = action
                i += 1
                stack.append(curState)
            # Reduce
            elif isinstance(action, tuple):
                # Pop off the stack as many times as there are symbol in the reduced production
                for n in xrange(len(action[1])):
                    stack.pop()
                # Find the next state based on the lhs of the reduced production and append it to the stack
                curState = self.table[stack[-1]][action[0]]
                stack.append(curState)
            # Error
            else:
                return False
        return False

# grammar = Grammar(
#     """
#     Start -> E $ ;
#     E -> E E + | n ;
#     """)
# grammar = Grammar(
#     """
#     Start -> S $ ;
#     S -> < S | M ;
#     M -> < M > | < > ;
#     """)
grammar = Grammar(
    """
    Start -> Exp $ ;
    Exp -> Term ExpTail ;
    ExpTail -> + Exp | - Exp | epsilon ;
    Term -> Factor TermTail ;
    TermTail -> * Term | / Term | epsilon ;
    Factor -> Value FactorTail | - Factor;
    FactorTail -> ^ Factor | epsilon ;
    Value -> Num | ( Exp ) ;
    Num -> 6 ;
    """)
print grammar
parser = LRParser(grammar, 'Start')
# for e in xrange(len(parser.table)):
#     print e, parser.table[e]
print parser.parse('6 + - ( 6 * 6 ) ^ 6 / '.split())

