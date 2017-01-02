class RegexBuilder():
    def __init__(self):
        self.regex = ""
        self.fsm = []
        self.statenum = 0
        self.pos = 0
        self.dsm = []

    def next(self, lower, upper=None):
        if upper == None: upper = lower
        if self.pos < len(self.regex) and self.regex[self.pos] >= lower and self.regex[self.pos] <= upper:
            t = self.regex[self.pos]
            self.pos += 1
            return t
        return False

    def error(self):
        raise Exception("ParseError, pos = "+str(self.pos))

    # Updates a edge mapping for a range of ASCII values to true. Can also be used to set the inverse to true
    def updateMap(self, mapping, left, right, inverse=False):
        if not inverse:
            for i in xrange(ord(left), ord(right)+1):
                mapping[chr(i)] = True
        else:
            for i in xrange(1, ord(left)):
                mapping[chr(i)] = True
            for i in xrange(ord(right)+1, 256):
                mapping[chr(i)] = True

    # Builds default map of single-state edges
    def makeMap(self):
        return {chr(i):False for i in xrange(1,256)}

    # If the character starts with an escape, parse the next as a special character
    # If the next is a ., then have the state accept all inputs. Otherwise, parse normally
    def parseChar(self, mapping):
        if self.next('\\'):
            self.parseSpecial(mapping)
        elif self.next('.'):
            self.updateMap(mapping, 1, 255)
        else:
            mapping[self.parseC()] = True

    # Consumes the next input char and returns it. Prevents unescaped versions of some special characters
    def parseC(self):
        c = self.next(chr(1), chr(255))
        if not c: self.error()
        if c == '(' or c == ')' or c == '[' or c == ']': self.error()
        return c

    # Recognizes the 4 special characters d, D, s, and S. Everything else is parsed as normal
    def parseSpecial(self, mapping):
        c = self.next(chr(1), chr(255))
        if not c: self.error()
        if c == 'd':
            self.updateMap(mapping, '0', '9')
        elif c == 'D':
            self.updateMap(mapping, '0', '9', True)
        elif c == 's':
            mapping[' '] = True
        elif c == ' S':
            self.updateMap(mapping, ' ', ' ', True)
        else:
            mapping[c] = True

    # Parses an element of a single-state set [], whether it's a char or a range
    def parseElem(self, mapping):
        # Escaped chars cannot form ranges, so it's parsed as a char
        if self.next('\\'):
            self.parseSpecial(mapping)
        # When a char is followed by a -, the char and the char that comes after are parsed as a range
        # Otherwise, the char is processed as usual
        else:
            left = self.parseC()
            if self.next('-'):
                if self.next('\\'): error()
                right = self.parseC()
                self.updateMap(mapping, left, right)
            else:
                mapping[left] = True

    # Parses the contents of a single-state set, including the ending ']''
    def parseSet(self, mapping):
        # First check for the optional ^ to see if the set should be inversed
        inverse = False
        if self.next('^'):
            inverse = True
        # Process at least one element of the set
        self.parseElem(mapping)
        # As long as the set isn't ended by ']', keep processing elements
        while not self.next(']'):
            self.parseElem(mapping)
        # If inverse flag is set, swap true/false on every edge on the mapping
        if inverse:
            for i in range(1, 256):
                mapping[chr(i)] = not mapping[chr(i)]

    # Parse either a single character, a single-state set, or a bracketed regex expression
    def parseValue(self):
        if self.next('('):
            start, accepts = self.parseExp()
            if not self.next(')'):
                self.error()
            return start, accepts
        else:
            mapping = self.makeMap()
            if self.next('['):
                self.parseSet(mapping)
            else:
                self.parseChar(mapping)
            start = len(self.fsm)
            self.fsm.append([])
            for c in mapping:
                if mapping[c]:
                    self.fsm[-1].append((c, start+1))
            self.fsm.append([])
            return start, [start+1]

    def concatenate(self, acceptsL, startR):
        for edge in self.fsm[startR]:
            for accepting in acceptsL:
                self.fsm[accepting].append(edge)

    def optional(self, start, accepts):
        self.fsm.append([])
        newStart = len(self.fsm)-1
        for edge in self.fsm[start]:
            self.fsm[newStart].append(edge)
        start = newStart
        accepts.append(newStart)
        return start, accepts

    def kleene(self, start, accepts):
        start, accepts = self.repeating(start, accepts)
        accepts.append(start)
        return start, accepts

    def repeating(self, start, accepts):
        self.fsm.append([])
        newStart = len(self.fsm)-1
        for edge in self.fsm[start]:
            self.fsm[newStart].append(edge)
            for accepting in accepts:
                self.fsm[accepting].append(edge)
        return newStart, accepts


    def parseUnary(self):
        start, accepts = self.parseValue()
        if self.next('?'):
            start, accepts = self.optional(start, accepts)
        elif self.next('*'):
            start, accepts = self.kleene(start, accepts)
        elif self.next('+'):
            start, accepts = self.repeating(start, accepts)
        return start, accepts

    def parseConcat(self):
        startL, acceptsL = self.parseUnary()
        while self.pos < len(self.regex) and self.regex[self.pos] != '|' and self.regex[self.pos] != ')':
            startR, acceptsR = self.parseUnary()
            self.concatenate(acceptsL, startR)
            if startR in acceptsR:
                acceptsL = acceptsR + acceptsL
            else:
                acceptsL = acceptsR
        return startL, acceptsL

    def parseExp(self):
        startL, acceptsL = self.parseConcat()
        while self.next('|'):
            startR, acceptsR = self.parseConcat()
            self.fsm[startL] += self.fsm[startR]
            acceptsL += acceptsR
        return startL, acceptsL

    def compile(self, regex):
        self.regex = regex
        self.start, self.accepts = self.parseExp()
        self.toDSM()

    def buildTable(self):
        table = []
        for state in self.fsm:
            table.append({})
            for c, dest in state:
                if c not in table[-1]:
                    table[-1][c] = set([dest])
                else:
                    table[-1][c].add(dest)
        return table

    def toDSM(self):
        # Maps nsm states to dsm states
        ntod = {frozenset([self.start]):0}
        # Maps dsm states to sets of nsm states
        dton = [set([self.start])]
        # NSM state table that maps states and chars to sets of transition states
        table = self.buildTable()
        # for state in self.fsm:
        #     print state
        # for state in table:
        #     print state
        for stateSet in dton:
            self.dsm.append({})
            for i in xrange(1, 256):
                c = chr(i)
                destSet = set([])
                for state in stateSet:
                    if c in table[state]: destSet = destSet.union(table[state][c])
                if destSet == set([]):
                    continue
                elif frozenset(destSet) not in ntod:
                    dton.append(destSet)
                    nextDstate = len(dton)-1
                    # Map dsm state to nsm state
                    ntod[frozenset(destSet)] = nextDstate
                self.dsm[-1][c] = ntod[frozenset(destSet)]
        #print ntod, dton
        self.start = 0
        newAccepts = []
        for stateSet in ntod:
            isAccept = False
            for state in stateSet:
                if state in self.accepts:
                    isAccept = True
                    break
            if isAccept:
                newAccepts.append(ntod[stateSet])
        self.accepts = newAccepts



builder = RegexBuilder()
builder.compile("([a-cd]*|az?)")
print builder.start, builder.accepts
for state in builder.dsm:
    print state 
            


