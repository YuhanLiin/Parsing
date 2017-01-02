from grammar import *

class LLParser(object):
    def __init__(self, grammar):
        # Maps (lhs, first token) to production
        self.table = {}
        self.firstSets = {}
        self.followSets = {}
        self.derivesEpsilon = {}
        self.grammar = grammar

        # Populates the parse table and first set for the lhs symbol. Also indicates whether the symbol can derive epsilon
        def populateFirstSet(lhs):
            self.firstSets[lhs] = set([])
            emptyProduction = False
            productions = self.grammar.getProductions(lhs)
            for production in productions:
                i = 0
                # For each production run through all its symbols in order
                while i < len(production):
                    frontSymbol = production[i]
                    if isTerminal(frontSymbol):
                        # If the symbol is a terminal, update the first set and parse table with it.
                        self.table[(lhs, frontSymbol)] = production
                        self.firstSets[lhs].add(frontSymbol)
                        # Since the production will add no more to the first set, move on to the next one
                        break
                    else:
                        # If the symbol is non-terminal, find out its first set recursively
                        if frontSymbol not in self.firstSets:
                            populateFirstSet(frontSymbol)
                        # The first set of lhs includes the first set of the symbol, which is also added to the parse table
                        for token in self.firstSets[frontSymbol]:
                            self.firstSets[lhs].add(token)
                            self.table[(lhs, token)] = production
                        # If the current symbol derives epsilon, then the next symbol will contribute to the first set as well
                        # Otherwise we can continue to the next production
                        if not self.derivesEpsilon[frontSymbol]:
                            break
                    i += 1
                # If every symbol in the production derives epsilon, that means this production derives epsilon as well
                if i >= len(production):
                    emptyProduction = True
            # If any production derives epsilon, lhs derives epsilon
            self.derivesEpsilon[lhs] = emptyProduction

        # Populates the follow sets of all non-terminals without recursion. Does not populate parse table
        # def populateFollowSet():
        #     # Initialize sets
        #     for rule in grammar.rules:
        #         self.followSets[rule.lhs] = set([])

        #     changing = True
        #     #Loop the algorithm until no more changes to follow sets occur
        #     while changing:
        #         changing = False
        #         for rule in self.grammar.rules:
        #             # For each production of each rule, compute follow sets for every nonterminal symbol by accumulating a list of symbols to add
        #             for production in rule.productions:
        #                 addlhs = True
        #                 symbolsToAdd = set([])
        #                 # Read the symbols backwards
        #                 for symbol in production[::-1]:
        #                     if isTerminal(symbol):
        #                         # If the symbol is terminal, reset the symbol list and add the terminal
        #                         symbolsToAdd = set([symbol])
        #                         addlhs = False
        #                     else:
        #                         # If the symbol is non-terminal, first check if all the symbols after it derives epsilon
        #                         # If so, include the rule's follow set in the symbol list
        #                         if addlhs:
        #                             symbolsToAdd.update(self.followSets[rule.lhs])
        #                         # Also track if the symbol's follow set changes due to symbol list before adding the symbol list to the symbol's follow set
        #                         if not (symbolsToAdd <= self.followSets[symbol]):
        #                             changing = True
        #                         self.followSets[symbol].update(symbolsToAdd)

        #                         # If the symbol is not empty, reset the symbol list. Always add its first set to the symbols
        #                         if self.derivesEpsilon[symbol]:
        #                             symbolsToAdd.update(self.firstSets[symbol])
        #                         else:
        #                             symbolsToAdd = self.firstSets[symbol]
        #                             addlhs = False


        # Compute all first sets and their corresponding table entries
        for rule in self.grammar.rules:
            populateFirstSet(rule.lhs)

        # Do same thing for all follow sets of nonterminals that derive epsilon
        # populateFollowSet()
        # for rule in self.grammar.rules:
        #     if self.derivesEpsilon[rule.lhs]:
        #         for token in self.followSets[rule.lhs]:
        #             self.table[(rule.lhs, token)] = []

    # Does an LL parse of a token list based on the parse table
    def parse(self, startSymbol, tokens):
        tokens += ['$']
        stack = [startSymbol]
        pos = 0
        # Parse until stack is empty or input has been completely parsed
        while stack != [] and pos < len(tokens):
            symbol = stack.pop()
            # If top of stack is terminal, match the next token to it and advance the input
            if isTerminal(symbol):
                if symbol == tokens[pos]:
                    pos += 1
                else:
                    return False
            # If top of stack is non-terminal, insert the correct production backwards into the stack based on the table
            else:
                if (symbol, tokens[pos]) in self.table:
                    reversedProduction = self.table[(symbol, tokens[pos])][::-1]
                    for s in reversedProduction:
                        stack.append(s)
                elif not self.derivesEpsilon[symbol]:
                    return False
        if pos == len(tokens) and stack == []:
            return True
        else:
            return False


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

parser = LLParser(grammar)
for entry in sorted(parser.table):
    print entry, parser.table[entry]
print ""
tokens = "6 * 6 + 6".split()
print parser.parse("Start", tokens)





