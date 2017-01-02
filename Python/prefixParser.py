
ops = {"+": (lambda x,y: x+y, 2),
       "-": (lambda x,y: x-y, 2),
       "*": (lambda x,y: x*y, 2),
       "/": (lambda x,y: x/y, 2), }

def parse(tokens):
    value_counter = []
    val_stack = []
    op_stack = []

    def op_arg_count():
        if len(op_stack) == 0:
            return -1
        else:
            return ops[op_stack[-1]][1]

    def op_funct():
        if len(op_stack) == 0:
            return -1
        else:
            return ops[op_stack[-1]][0]

    for token in tokens:
        if token in ops:
            value_counter.append(0)
            op_stack.append(token)

        else:
            val_stack.append(token)
            value_counter[-1] += 1

            while value_counter[-1] == op_arg_count():
                args = []
                for i in xrange(op_arg_count()):
                    args.append(float(val_stack.pop()))
                val_stack.append(op_funct()(*args))

                op_stack.pop()
                value_counter.pop()
                if len(value_counter) == 0:
                    break
                value_counter[-1] += 1

        if len(op_stack) == 0:
            return val_stack[-1]

tokens = raw_input().split()
print parse(tokens)
