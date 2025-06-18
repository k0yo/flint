import re
from typing import List, Tuple

INDENT_SIZE = 4

TOKEN_SPECIFICATION = [
    ('MLCOMMENT',  r';-[\s\S]*?-;'),
    ('COMMENT',    r';[^\n]*'),
    ('NUMBER',     r'\d+(\.\d+)?'),
    ('STRING',     r'"[^"\n]*"|\'[^\'\n]*\''),
    ('INC_DEC',    r'\+\+|--'),
    ('COMP_ASSIGN', r'[+\-*/%]='),
    ('ASSIGN',     r'='),
    ('COLON',      r':'),
    ('PIPE',      r'\|>'),
    ('COMP_OP',    r'==|!=|<=|>=|<|>'),
    ('LOGIC_OP',   r'\b(and|or|not)\b|!'),
    ('KEYWORD',    r'\b(start|let|if|else|while|loop|command|object|check|equals|write|ask|as|wait|async|true|false|null|num|text|bool|list|map)\b'),
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
    indent_stack = [0]

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
    
    def emit_line_buffer():
        nonlocal expect_indent, indent_stack, tokens, line_buffer, current_line
        if not line_buffer:
            return
        leading_spaces = len(current_line) - len(current_line.lstrip(' '))
        if leading_spaces % INDENT_SIZE != 0:
            raise IndentationError(f"Inconsistent indentation: {leading_spaces} spaces not multiple of {INDENT_SIZE}")
        current_level = leading_spaces // INDENT_SIZE
        last_level = indent_stack[-1]
        if expect_indent:
            if current_level > last_level:
                tokens.append(('INDENT', '1'))
                indent_stack.append(last_level + 1)
            expect_indent = False
        else:
            if current_level > last_level:
                for _ in range(current_level - last_level):
                    tokens.append(('INDENT', '1'))
                    indent_stack.append(last_level + 1)
                    last_level += 1
            elif current_level < last_level:
                for _ in range(last_level - current_level):
                    tokens.append(('DEDENT', '1'))
                    indent_stack.pop()
                    last_level -= 1
        tokens.extend(line_buffer)
        line_buffer.clear()

    while pos < len(code):
        if code[pos] == '\n':
            if current_line.strip() and not re.match(r'^\s*($|;|;-)', current_line):
                emit_line_buffer()
            tokens.append(('NEWLINE', '\\n'))
            current_line = ""
            pos += 1
            last_newline_pos = pos
            continue
        match = TOKEN_RE.match(code, pos)
        if not match:
            if code[pos] not in (' ', '\t'):
                raise SyntaxError(f'Illegal character {code[pos]!r} at position {pos}')
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
    if current_line.strip() and not re.match(r'^\s*($|;|;-)', current_line):
        emit_line_buffer()
    while len(indent_stack) > 1:
        tokens.append(('DEDENT', '1'))
        indent_stack.pop()
    tokens.append(('EOF', ''))
    return tokens


sample_code = ''';- This is 
multi-line comment -;
start:
    ; This is a single-line comment

    write "🧙 Welcome to FlintQuest!"
    
    ask "What is your name, adventurer?" as player_name
    write "Greetings, ${player_name}!"

    object Player:
        name = ""
        health = 100
        gold = 0
        mood = "neutral"

    let hero = Player()
    hero.name = player_name

    random.seed num:42
    hero.gold = random.int num:5 num:15

    write "You wake up in a dark forest with ${hero.gold} gold coins."
    write "Your health is at ${hero.health}. The adventure begins..."

    ask "How do you feel today?" as hero.mood
    check hero.mood:
        equals "happy":
            write "A positive mind is your greatest weapon!"
        equals "tired":
            write "Even heroes need rest."
        equals "angry":
            write "Focus that fire!"
        equals "scared":
            write "Courage is feeling fear and moving forward anyway."

    mood_clean = lower text:trim text:hero.mood
    write "Mood saved as: '${mood_clean}'"

    write "You encounter a wild goblin!"
    loop 3:
        write "The goblin attacks!"

    goblin_attack = random.int num:5 num:20
    hero.health = hero.health - goblin_attack

    if hero.health > 0:
        write "You survive the hit! Health: ${hero.health}"
    else:
        write "The goblin defeated you..."
        wait 2
        write "💀 Game Over."
        return

    command heal amount:
        hero.health = hero.health + amount
        write "You healed for ${amount} points. Current health: ${hero.health}"

    heal 10

    treasures = ["ruby", "emerald", "gold coin"]
    i = 0
    while i < 3:
        item = treasures[i]
        write "You found a ${item}!"
        i += 1

    num_gold = num hero.gold
    write "Total treasure converted to number: ${num_gold}"

    write "You hear a whisper: " |> text "sretneva daerac rof sknahT" |> reverse

    ask "Do you want to enter the cave? (yes/no)" as choice
    check choice:
        equals "yes":
            write "You bravely step into the darkness..."
        equals "no":
            write "You walk away into the safety of the woods..."

    write "✨ The End. Thanks for playing, ${hero.name}!"
    write true
    write 6 + 7
'''

tokens = tokenize(sample_code)
for token in tokens:
    print(token)
