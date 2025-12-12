/*
 * Advanced tokenizer for NaoKernel shell
 * Supports quoted strings, escape sequences, and proper argument parsing
 */

#ifndef TOKENIZE_H
#define TOKENIZE_H

#include "../essentials/types.h"

#define MAX_TOKENS 32
#define MAX_TOKEN_LEN 256

typedef struct {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int count;
} TokenArray;

/* Main tokenization function - handles quotes, escapes, etc. */
void tokenize(const char *input, TokenArray *result);

/* Check if string contains special characters that need tokenization */
int has_special_chars(const char *str);

/* Get pointer to first token (command name) from tokenized array */
const char* get_command_token(TokenArray *tokens);

/* Get pointer to nth argument (0-indexed, after command) */
const char* get_arg_token(TokenArray *tokens, int index);

#endif
