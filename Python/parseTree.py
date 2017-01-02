class ParseNode(object):
    def __init__(self, symbol):
        self.symbol = symbol
        self.children = []

    def addChild(self, child):
        self.children.append(child)

class ParseTree(object):
    def __init__(self, root):
        self.root = ParseNode(root)

