import re
from typing import List, Tuple


TOKEN_SPECIFICATION = [
    ('NUMBER',     r'\d+(\.\d+)?'),       # Integer or decimal number
    ('STRING',     r'"[^"]*"|\'[^\']*\''), # String literals
    ('ASSIGN',     r'='),                 # Assignment operator
    ('COLON',      r':'),                 # Colon
    ('ARROW',      r'\|>'),               # Pipe operator
    ('OP',         r'[+\-*/%]'),          # Arithmetic operators
    ('COMP_OP',    r'==|!=|<=|>=|<|>'),   # Comparison operators
    ('LOGIC_OP',   r'\b(and|or|not)\b|!'),# Logical operators
    ('KEYWORD',    r'\b(start|let|if|else|while|loop|command|object|check|equals|write|ask|as|wait|async|true|false|null|num|text|bool|list|map)\b'),
    ('IDENTIFIER', r'[A-Za-z_][A-Za-z0-9_]*'), # Identifiers
    ('NEWLINE',    r'\n'),                # Line endings
    ('SKIP',       r'[ \t]+'),            # Skip over spaces and tabs
    ('COMMENT',    r'--[^\n]*'),          # Single-line comments
    ('MLCOMMENT',  r'#-[\s\S]*?-#'),      # Multi-line comments
    ('LBRACKET',   r'\['),                # Left bracket
    ('RBRACKET',   r'\]'),                # Right bracket
    ('LBRACE',     r'\{'),                # Left brace
    ('RBRACE',     r'\}'),                # Right brace
    ('COMMA',      r','),                 # Comma
    ('DOT',        r'\.'),                # Dot (e.g., object.attr)
    ('LPAREN',     r'\('),                # Left parenthesis
    ('RPAREN',     r'\)'),                # Right parenthesis
]

TOKEN_REGEX = '|'.join('(?P<%s>%s)' % pair for pair in TOKEN_SPECIFICATION)
TOKEN_RE = re.compile(TOKEN_REGEX)

def tokenize(code: str) -> List[Tuple[str, str]]:
    tokens = []
    for mo in TOKEN_RE.finditer(code):
        kind = mo.lastgroup
        value = mo.group()
        if kind == 'NEWLINE':
            tokens.append((kind, '\\n'))
        elif kind in ('SKIP', 'COMMENT', 'MLCOMMENT'):
            continue
        else:
            tokens.append((kind, value))
    return tokens


sample_code = '''
start:
    write "Hello, ${name}!"
    let x: num = 5
    if x > 3:
        write "x is large"
    else:
        write "x is small"
-- This is a comment
#- This is a
multi-line comment -#
'''

tokens = tokenize(sample_code)
print(tokens)
