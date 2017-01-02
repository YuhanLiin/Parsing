class Rule (object):
    def __init__(self, lhs):
        self.lhs = lhs
        self.productions = []

    def addProduction(self, production):
        if production == ["epsilon"]:
            self.productions.append([])
        else:
            self.productions.append(production)

    def __str__(self):
        return self.lhs + " -> " + str(self.productions)

    def __repr__(self):
        return str(self)

class Grammar (object):
    def __init__(self, grammarString):
        self.rules = []

        #Takes a formatted string and parses it into a list of rule to represent the grammar
        def parseGrammar():
            rules = [rule.split('->') for rule in grammarString.replace('\n', '').split(';')]
            for ruleList in rules:
                if len(ruleList) != 2:
                    continue
                lhs = ruleList[0].strip()
                rhs = [prod.split() for prod in ruleList[1].split('|')]
                rule = Rule(lhs)
                for production in rhs:
                    rule.addProduction(production)
                self.rules.append(rule)

        parseGrammar()

    def __str__(self):
        out = ""
        for rule in self.rules:
            out += str(rule) + '\n'
        return out

    def __repr__(self):
        return str(self)

    def getProductions(self, lhs):
        for rule in self.rules:
            if rule.lhs == lhs:
                return rule.productions


def isTerminal(symbol):
    if len(symbol) == 1:
        return True
    return False


# Example Grammar

# grammar = Grammar(
#     """
#     Exp -> Term ExpTail ;
#     ExpTail -> + Exp | - Exp | epsilon ;
#     Term -> Factor TermTail ;
#     TermTail -> * Term | / Term | epsilon ;
#     Factor -> Value FactorTail ;
#     FactorTail -> ^ Factor | epsilon ;
#     Value -> Num | ( Exp ) | - Term ;
#     Num -> 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0 ;
#     """)





