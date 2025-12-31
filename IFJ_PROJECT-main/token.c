#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

Token *create_token(TOKEN_TYPE token_type, char *lexeme, int lexeme_length, int line_number, int col_number, int scope, int *previous_scope_arr, int scope_count) {
    Token *token = malloc(sizeof(Token));

    if (!token){
        return NULL;
    }

    token->token_type = token_type;
    token->lexeme_length = lexeme_length;
    token->token_line_number = line_number;
    token->token_col_number = col_number - lexeme_length;
    token->scope = scope;

    token->scope_count = scope_count;
    token->previous_scope_arr = malloc(scope_count * sizeof(int));

    for(int i = 0; i < scope_count; i++){
        token->previous_scope_arr[i] = previous_scope_arr[i];
    }

    token->token_lexeme = malloc(sizeof(char) * (lexeme_length + 1));
    
    if(token->token_lexeme == NULL){
        free(token);
        return NULL;
    }

    memcpy(token->token_lexeme, lexeme, lexeme_length);
    token->token_lexeme[lexeme_length] = '\0';

    return token;
}

void free_token(Token *token){
    free(token->token_lexeme);
    free(token);
}

void print_token(Token *token){
    printf("Type:   %i\n", token->token_type);
    printf("Lexeme: %s\n", token->token_lexeme);
    printf("Length: %i\n", token->lexeme_length);
    printf("Line:   %i\n", token->token_line_number);
    printf("Col:    %i\n", token->token_col_number);
    printf("Scope count:    %i\n", token->scope_count);
    printf("Scope arr:      ");
    for(int i = 0; i < token->scope_count; i++){
        printf("%i ", token->previous_scope_arr[i]);
    }

    printf("\n");
    printf("\n");
}