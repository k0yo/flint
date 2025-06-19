import re
from typing import List, Tuple


TOKEN_SPECIFICATION = [
    ('MLCOMMENT',  r';-[\s\S]*?-;'),
    ('COMMENT',    r';[^\n]*'),
    ('NUMBER',     r'\d+(\.\d+)?'),
    ('STRING',     r'"([^"\\\n]|\\.)*"|\'([^\'\\\n]|\\.)*\''),
    ('BOOL',       r'\b(true|false)\b'),
    ('INC_DEC',    r'\+\+|--'),
    ('COMP_ASSIGN', r'[+\-*/%]='),
    ('ASSIGN',     r'='),
    ('COLON',      r':'),
    ('PIPE',      r'\|>'),
    ('COMP_OP',    r'==|!=|<=|>=|<|>'),
    ('LOGIC_OP',   r'\b(and|or|not)\b|!'),
    ('KEYWORD',    r'\b(start|let|if|else|while|loop|command|object|check|equals|write|ask|as|wait|async|null|num|text|bool|list|map|return)\b'),
    ('OP',         r'[+\-*/%]'),
    ('IDENTIFIER', r'[A-Za-z_][A-Za-z0-9_]*'),
    ('NEWLINE',    r'\n'),
    ('SKIP',       r'[ \t]+'),
    ('LBRACKET',   r'\['),
    ('RBRACKET',   r'\]'),
    ('LBRACE',     r'\{'),
    ('RBRACE',     r'\}'),
    ('COMMA',      r','),
    ('DOT',        r'\.'),
    ('LPAREN',     r'\('),
    ('RPAREN',     r'\)'),
]

TOKEN_REGEX = '|'.join(f'(?P<{name}>{pattern})' for name, pattern in TOKEN_SPECIFICATION)
TOKEN_RE = re.compile(TOKEN_REGEX)

def tokenize(code: str) -> List[Tuple[str, str]]:
    tokens = []
    indent_stack = ['']
    open_positions = [m.start() for m in re.finditer(r';-', code)]
    close_positions = [m.start() for m in re.finditer(r'-;', code)]
    if len(open_positions) > len(close_positions):
        matched_pairs = min(len(open_positions), len(close_positions))
        unclosed_pos = open_positions[matched_pairs]
        line_number = code.count('\n', 0, unclosed_pos) + 1
        raise SyntaxError(f"Unclosed multi-line comment starting at line {line_number}")
    elif len(close_positions) > len(open_positions):
        matched_pairs = len(open_positions)
        unopened_pos = close_positions[matched_pairs]
        line_number = code.count('\n', 0, unopened_pos) + 1
        raise SyntaxError(f"Unmatched multi-line comment ending '-;' at line {line_number}")

    pos = 0
    current_line = ""
    last_newline_pos = 0
    expect_indent = False
    line_buffer = []
    last_token_newline = False

    def emit_line_buffer():
        nonlocal expect_indent, indent_stack, tokens, line_buffer, current_line, last_token_newline
        if not line_buffer:
            return
        leading_ws = re.match(r'^[ \t]*', current_line).group(0)
        current_indent = leading_ws
        last_indent = indent_stack[-1] if indent_stack else ''
        if current_indent != last_indent:
            if current_indent.startswith(last_indent):
                tokens.append(('INDENT', '1'))
                indent_stack.append(current_indent)
            elif last_indent.startswith(current_indent):
                while indent_stack and indent_stack[-1] != current_indent:
                    tokens.append(('DEDENT', '1'))
                    indent_stack.pop()
                if not indent_stack or indent_stack[-1] != current_indent:
                    raise IndentationError("Unindent does not match any outer indentation level")
            else:
                raise IndentationError("Inconsistent use of tabs and spaces in indentation")
        tokens.extend(line_buffer)
        line_buffer.clear()
        last_token_newline = False

    while pos < len(code):
        if code[pos] == '\n':
            is_blank = not current_line.strip() or re.match(r'^\s*($|;|;-)', current_line)
            if not is_blank:
                emit_line_buffer()
                if not last_token_newline:
                    tokens.append(('NEWLINE', '\\n'))
                    last_token_newline = True
            else:
                if not last_token_newline:
                    tokens.append(('NEWLINE', '\\n'))
                    last_token_newline = True
            current_line = ""
            pos += 1
            last_newline_pos = pos
            continue
        match = TOKEN_RE.match(code, pos)
        if not match:
            if code[pos] not in (' ', '\t'):
                line_number = code.count('\n', 0, pos) + 1
                raise SyntaxError(f'Illegal character {code[pos]!r} at line {line_number}')
            pos += 1
            continue
        kind = match.lastgroup
        value = match.group()
        pos = match.end()
        current_line = code[last_newline_pos:pos]
        if kind in ('SKIP', 'COMMENT', 'MLCOMMENT'):
            continue
        if kind == 'COLON':
            expect_indent = True
        line_buffer.append((kind, value))
        last_token_newline = False
    if line_buffer:
        emit_line_buffer()
    while len(indent_stack) > 1:
        tokens.append(('DEDENT', '1'))
        indent_stack.pop()
    tokens.append(('EOF', ''))
    return tokens


sample_code = ''';- Flint Café sample with all features -;
; This is a single-line comment
start:
    write "☕ Welcome to Flint Café! ☕"
    ask "What is your name, customer?" as customer_name

    object Customer:
        name = ""
        balance = 20
        order = ""
        mood = "neutral"
        loyalty = false

    let guest = Customer()
    guest.name = customer_name

    menu = {"coffee": 5, "tea": 3, "cake": 7}
    write "Today's menu:"
    write "Coffee: $${menu[\\"coffee\\"]}, Tea: $${menu[\\"tea\\"]}, Cake: $${menu[\\"cake\\"]}"

    ask "What would you like to order? (coffee/tea/cake)" as guest.order
    price = menu[guest.order]
    if guest.balance >= price:
        guest.balance -= price
        write "Enjoy your ${guest.order}, ${guest.name}! Remaining balance: $${guest.balance}"
    else:
        write "Sorry, you don't have enough money for ${guest.order}."

    ask "How do you feel about your order?" as guest.mood
    check guest.mood:
        equals "happy":
            write "We're glad you're happy!"
        equals "disappointed":
            write "Sorry to hear that. Next time will be better!"
        equals "excited":
            write "Excitement is contagious!"
        equals "bored":
            write "We'll try to spice things up!"

    guest.loyalty = guest.balance < 10 or guest.mood == "happy"
    if guest.loyalty:
        write "You've joined our loyalty program! Free cookie next time."
    else:
        write "Earn loyalty by visiting more or sharing your happiness!"

    favorites = ["coffee", "tea", "cake"]
    i = 0
    loop 3:
        write "Customer favorite #${i+1}: ${favorites[i]}"
        i++

    command shout item:
        write upper item

    write upper "thank you for visiting!"

    random.seed 7
    lucky_number = random.int 1 100
    write "Your lucky number for today is: ${lucky_number}"
    wait 1

    review = "  Great service!  "
    clean_review = trim review
    write "Customer review: '${clean_review}'"
    write "Reversed: ${reverse clean_review}"

    guest.balance += 5
    guest.balance--
    write "You found $5! New balance: $${guest.balance}"

    write "Goodbye, ${guest.name}! Come back soon to Flint Café."
'''

tokens = tokenize(sample_code)
for token in tokens:
    print(token)
