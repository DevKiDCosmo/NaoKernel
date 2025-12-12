/*
 * Advanced tokenizer for NaoKernel shell
 * Supports quoted strings, escape sequences, and proper argument parsing
 */

#include "tokenizer.h"
#include "../essentials/types.h"

/* Check if character is whitespace */
static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* Check if character is a quote */
static int is_quote(char c) {
    return c == '"' || c == '\'';
}

/* Copy string with length limit - manual loop to avoid memcpy */
static void str_copy_n(char *dest, const char *src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/*
 * Tokenize input string with support for:
 * - Quoted strings (preserves spaces within quotes)
 * - Escape sequences (\", \', \\, \n, \t)
 * - Multiple whitespace characters as separators
 * - Mixed single and double quotes
 */
void tokenize(const char *input, TokenArray *result) {
    result->count = 0;
    
    if (!input || *input == '\0') {
        return;
    }
    
    int i = 0;
    char current_token[MAX_TOKEN_LEN];
    int token_pos = 0;
    char in_quote = '\0';  /* \0 = not in quote, '"' or '\'' = in that quote type */
    int escaped = 0;
    
    while (input[i] != '\0' && result->count < MAX_TOKENS) {
        char c = input[i];
        
        /* Handle escape sequences */
        if (escaped) {
            /* Process escape sequence */
            switch (c) {
                case 'n':
                    current_token[token_pos++] = '\n';
                    break;
                case 't':
                    current_token[token_pos++] = '\t';
                    break;
                case '\\':
                case '"':
                case '\'':
                    current_token[token_pos++] = c;
                    break;
                default:
                    /* Unknown escape, keep both characters */
                    current_token[token_pos++] = '\\';
                    if (token_pos < MAX_TOKEN_LEN - 1) {
                        current_token[token_pos++] = c;
                    }
            }
            escaped = 0;
            i++;
            continue;
        }
        
        /* Check for escape character */
        if (c == '\\') {
            escaped = 1;
            i++;
            continue;
        }
        
        /* Handle quotes */
        if (is_quote(c)) {
            if (in_quote == '\0') {
                /* Start quote */
                in_quote = c;
            } else if (in_quote == c) {
                /* End matching quote */
                in_quote = '\0';
            } else {
                /* Different quote type inside quotes - treat as literal */
                if (token_pos < MAX_TOKEN_LEN - 1) {
                    current_token[token_pos++] = c;
                }
            }
            i++;
            continue;
        }
        
        /* Handle whitespace */
        if (is_whitespace(c) && in_quote == '\0') {
            /* End current token if we have one */
            if (token_pos > 0) {
                current_token[token_pos] = '\0';
                /* Manually copy token to result array */
                int j = 0;
                while (j < token_pos && j < MAX_TOKEN_LEN - 1) {
                    result->tokens[result->count][j] = current_token[j];
                    j++;
                }
                result->tokens[result->count][j] = '\0';
                result->count++;
                token_pos = 0;
            }
            i++;
            continue;
        }
        
        /* Regular character - add to current token */
        if (token_pos < MAX_TOKEN_LEN - 1) {
            current_token[token_pos++] = c;
        }
        i++;
    }
    
    /* Add final token if exists */
    if (token_pos > 0 && result->count < MAX_TOKENS) {
        current_token[token_pos] = '\0';
        /* Manually copy token to result array */
        int j = 0;
        while (j < token_pos && j < MAX_TOKEN_LEN - 1) {
            result->tokens[result->count][j] = current_token[j];
            j++;
        }
        result->tokens[result->count][j] = '\0';
        result->count++;
    }
}

/*
 * Check if string contains special characters that need proper tokenization
 * Special characters include: quotes, backslashes, pipes, redirects, etc.
 * Returns 1 if special chars found, 0 otherwise
 */
int has_special_chars(const char *str) {
    if (!str) {
        return 0;
    }
    
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        
        /* Check for characters that need special handling */
        if (c == '"' || c == '\'' || c == '\\' ||   /* Quotes and escapes */
            c == '|' || c == '>' || c == '<' ||     /* Pipes and redirects */
            c == '&' || c == ';' ||                 /* Command separators */
            c == '$' || c == '`' ||                 /* Variable expansion */
            c == '(' || c == ')' ||                 /* Grouping */
            c == '{' || c == '}' ||                 /* Braces */
            c == '[' || c == ']' ||                 /* Brackets */
            c == '*' || c == '?') {                 /* Wildcards */
            return 1;
        }
    }
    
    return 0;
}

/*
 * Get the command token (first token) from tokenized array
 * Returns NULL if no tokens exist
 */
const char* get_command_token(TokenArray *tokens) {
    if (!tokens || tokens->count == 0) {
        return 0;  /* NULL */
    }
    return tokens->tokens[0];
}

/*
 * Get the nth argument token (0-indexed after command)
 * arg index 0 = first argument (second token overall)
 * Returns NULL if index out of bounds
 */
const char* get_arg_token(TokenArray *tokens, int index) {
    if (!tokens || index < 0) {
        return 0;  /* NULL */
    }
    
    /* Account for command being token 0 */
    int actual_index = index + 1;
    
    if (actual_index >= tokens->count) {
        return 0;  /* NULL */
    }
    
    return tokens->tokens[actual_index];
}