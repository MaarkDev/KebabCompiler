#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_T_IDENTIFIER,  
    TOKEN_T_NUM,  
    TOKEN_T_STRING,     
    TOKEN_T_KEYWORD,     
    TOKEN_T_OPERATOR,
    TOKEN_T_GLOBAL_VAR,
    TOKEN_T_COMMA,
    TOKEN_T_BRACKET,
    TOKEN_T_EOL
} TOKEN_TYPE;

typedef struct token {
    TOKEN_TYPE token_type;
    char *token_lexeme;
    int lexeme_length;
    int token_line_number;
    int token_col_number;
    int scope;
    int scope_count;
    int *previous_scope_arr;
} Token;

Token *create_token(TOKEN_TYPE token_type, char *lexeme, int lexeme_length, int line_number, int col_number, int scope, int *previous_scope_arr, int scope_count);
void print_token(Token *token);
void free_token(Token *token);

#endif
