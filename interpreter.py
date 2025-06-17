import re
from typing import List, Tuple

INDENT_SIZE = 4

TOKEN_SPECIFICATION = [
    ('NUMBER',     r'\d+(\.\d+)?'),
    ('STRING',     r'"[^"]*"|\'[^\']*\''),
    ('ASSIGN',     r'='),
    ('COLON',      r':'),
    ('ARROW',      r'\|>'),
    ('OP',         r'[+\-*/%]'),
    ('COMP_OP',    r'==|!=|<=|>=|<|>'),
    ('LOGIC_OP',   r'\b(and|or|not)\b|!'),
    ('KEYWORD',    r'\b(start|let|if|else|while|loop|command|object|check|equals|write|ask|as|wait|async|true|false|null|num|text|bool|list|map)\b'),
    ('IDENTIFIER', r'[A-Za-z_][A-Za-z0-9_]*'),
    ('NEWLINE',    r'\n'),
    ('SKIP',       r'[ \t]+'),
    ('COMMENT',    r'--[^\n]*'),
    ('MLCOMMENT',  r'#-[\s\S]*?-#'),
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

    lines = code.splitlines()
    for line in lines:
        if re.match(r'^\s*($|--|#-)', line):
            pass
        else:
            leading_spaces = len(line) - len(line.lstrip(' '))
            if leading_spaces % INDENT_SIZE != 0:
                raise IndentationError(f"Inconsistent indentation: {leading_spaces} spaces not multiple of {INDENT_SIZE}")

            current_level = leading_spaces // INDENT_SIZE
            last_level = indent_stack[-1]

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

        pos = 0
        while pos < len(line):
            match = TOKEN_RE.match(line, pos)
            if not match:
                if line[pos] not in (' ', '\t'):
                    raise SyntaxError(f'Illegal character {line[pos]!r} at position {pos}')
                pos += 1
                continue

            kind = match.lastgroup
            value = match.group()
            pos = match.end()

            if kind in ('SKIP', 'COMMENT', 'MLCOMMENT'):
                continue
            elif kind == 'NEWLINE':
                tokens.append(('NEWLINE', '\\n'))
            else:
                tokens.append((kind, value))

    while len(indent_stack) > 1:
        tokens.append(('DEDENT', '1'))
        indent_stack.pop()

    tokens.append(('EOF', ''))
    return tokens


sample_code = '''#- This is a multi-line comment -#

start:
    -- This is a single-line comment

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
        i = i + 1

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
