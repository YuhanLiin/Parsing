class BaseParser():
    def __init__(self):
        self.pos = 0
        self.tokens = ""

    def next(self, t):
        if self.pos < len(tokens) and tokens[self.pos] in t:
            self.pos += 1
            return True
        return False

    def error(self):
        raise Exception("ParseError, pos = "+str(self.pos))

# Grammar:
# S' -> S $ ;
# S -> < S | M ;
# M -> < M > | < > ;
class Parser1(BaseParser):
    # S' -> .S $ 
    # -----------
    # S -> .< S 
    # S -> .M (automatically returns S)
    # M -> .< M > 
    # M -> .< > 
    def start(self):
        if self.next('<'):
            self.s_lb_m_rb()
            # S or M will be reduced and returned back, which will advance the start rule
            return self.end()
        else:
            self.error()

    # S -> < .S 
    # M -> < .M > 
    # M -> < .> 
    # ------------
    # S -> .< S 
    # S -> .M
    # M -> .< M > 
    # M -> .< > 
    def s_lb_m_rb(self):
        if self.next('<'):
            symbol = self.s_lb_m_rb()
            if symbol == 'M':
                symbol = self.m_rb()
                if symbol == 'M':
                    return 'M'
                symbol = 'S'
            # S -> < S. (reduce and return)
            if symbol == 'S':
                return 'S'
        # M -> < >. (reduce and return)
        elif self.next('>'):
            return 'M'
        else:
            self.error()

    # M -> < M .>
    # S -> M .
    def m_rb(self):
        # M -> < M >. (reduce and return)
        if self.next('>'):
            return 'M'
        else:
            return 'S'

    # S' -> S .$ (accept if at end)
    def end(self):
        if len(tokens) == self.pos:
            return 'S'
        else:
            self.error()

    def parse(self, tokens):
        self.tokens = tokens
        self.start()
        pos = 0
        return True

# Grammar:
# S -> Exp $ ;
# Exp -> Exp (+|-) Term | Term ;
# Term -> Term (*|/) Factor / Factor ;
# Factor -> Value ^ Factor | Value | -Factor ;
# Value -> int | ( Exp ) ;
class ArithParser(BaseParser):
    # S -> .Exp $
    # Closures of Exp, Term, Factor, Value
    def start(self):
        if self.next('1234567890'):
            symbol = 'V'
        elif self.next('-'):
            symbol = self.factor_value_lb_int()
        elif self.next('('):
            symbol = self.exp()
        else:
            self.error()

        while symbol != 'S':
            if symbol == 'V':
                symbol = self.carat()
            elif symbol == 'F':
                symbol = 'T'
            elif symbol == 'T':
                symbol = self.mul_div_exp()
            elif symbol == 'E':
                symbol = self.plus_minus_end()

    # Factor -> Value . (reduce)
    # Factor -> Value .^ Factor
    def carat(self):
        if self.next('^'):
            return self.factor_value_lb_int()
        else:
            return 'F'

    # Factor -> Value ^ .Factor
    # Closures of Factor, Value
    def factor_value_lb_int(self):
        if self.next('1234567890'):
            symbol = 'V'
        elif self.next('-'):
            symbol = self.factor_value_lb_int()
        elif self.next('('):
            symbol = self.exp()
        else:
            self.error()

        while symbol != 'F':
            symbol = self.carat()
        return 'F'

    # Exp -> Term . (reduce)
    # Term -> Term .(*|/) Factor
    def mul_div_exp(self):
        if self.next('*') or self.next('/'):
            return self.term_value_lb_int()
        else:
            return 'E'

    # Term -> Term (*|/) .Factor
    # Closures of Factor, Value
    def term_value_lb_int(self):
        if self.next('1234567890'):
            symbol = 'V'
        elif self.next('-'):
            symbol = self.factor_value_lb_int()
        elif self.next('('):
            symbol = self.exp()
        else:
            self.error()

        while symbol != 'F':
            symbol = self.carat()
        return 'T'

    # S -> Exp .$ (will reduce)
    # Exp -> Exp .(+|-) Term
    def plus_minus_end(self):
        if len(tokens) == self.pos:
            return 'S'
        elif self.next('+') or self.next('-'):
            return self.term_factor_value_lb_int()
        else:
            self.error()

    # Exp -> Exp (+|-) .Term
    # Closures of Term, Factor, Value
    def term_factor_value_lb_int(self):
        if self.next('1234567890'):
            symbol = 'V'
        elif self.next('-'):
            symbol = self.factor_value_lb_int()
        elif self.next('('):
            symbol = self.exp()
        else:
            self.error()

        while symbol != 'E':
            if symbol == 'T':
                symbol = self.mul_div_exp()
            elif symbol == 'F':
                symbol = 'T'
            elif symbol == 'V':
                symbol = self.carat()
        return 'E'

    # Value -> ( .Exp )
    # Closures of Exp, Term, Factor, Value
    def exp(self):
        if self.next('1234567890'):
            symbol = 'V'
        elif self.next('-'):
            symbol = self.factor_value_lb_int()
        elif self.next('('):
            symbol = self.exp()
        else:
            self.error()

        while True:
            if symbol == 'V':
                symbol = self.carat()
            elif symbol == 'F':
                symbol = 'T'
            elif symbol == 'T':
                symbol = self.mul_div_exp()
            elif symbol == 'E':
                symbol = self.plus_minus_rb()
                if symbol == 'V':
                    return symbol

    # Value -> ( Exp .)
    # Exp -> Exp .(+|-) Term
    def plus_minus_rb(self):
        if self.next(')'):
            return 'V'
        elif self.next('+') or self.next('-'):
            return self.term_factor_value_lb_int()
        else:
            self.error()

    def parse(self, tokens):
        self.tokens = tokens
        self.start()
        pos = 0
        return True


tokens = "-(1*-1---1^--(-4+5)^-5)"
print ArithParser().parse(tokens)