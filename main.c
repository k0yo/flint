#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <sourcefile.fln>\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];

    const char *ext = strrchr(filename, '.');
    if (!ext || strcmp(ext, ".fln") != 0) {
        fprintf(stderr, "Error: Only .fln files are supported.\n");
        return 1;
    }
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(filesize + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(file);
        return 1;
    }
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0';
    fclose(file);

    int token_count = 0;
    Token *tokens = tokenize(buffer, &token_count);
    free(buffer);

    if (tokens == NULL) {
        return 1;
    }

    for (int i = 0; i < token_count; i++) {
        const char *type_str = token_type_to_string(tokens[i].type);
        if (tokens[i].type == T_NEWLINE) {
            printf("(%s, '\\n')\n", type_str);
        } else {
            printf("(%s, '%s')\n", type_str, tokens[i].value);
        }
    }

    free_tokens(tokens, token_count);

    return 0;
}
