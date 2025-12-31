#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "symbol.h"
#include "token.h"


// UTILS 

void copy_lexeme_from_token_to_sym(Token *token, Symbol *symbol){
    symbol->sym_lexeme = malloc(sizeof(char) * (symbol->sym_lexeme_length + 1));
    memcpy(symbol->sym_lexeme, token->token_lexeme, symbol->sym_lexeme_length);
    symbol->sym_lexeme[symbol->sym_lexeme_length] = '\0';
}

void init_identif_sym_arrays(Symbol *symbol, Token *token){
    symbol->sym_identif_declared_at_line_arr = NULL;
    symbol->sym_identif_declared_at_col_arr = NULL;
    symbol->sym_identif_declared_at_scope_arr = NULL;

    symbol->sym_identif_used_at_line_arr = malloc(sizeof(int) * symbol->sym_identif_use_count);
    symbol->sym_identif_used_at_col_arr = malloc(sizeof(int) * symbol->sym_identif_use_count);
    symbol->sym_identif_used_at_scope_arr = malloc(sizeof(int) * symbol->sym_identif_use_count);

    symbol->sym_identif_used_at_line_arr[0] = token->token_line_number;
    symbol->sym_identif_used_at_col_arr[0] = token->token_col_number;
    symbol->sym_identif_used_at_scope_arr[0] = token->scope;
}

void symbol_set_var_type(Token *token, Symbol *symbol){
    if(strcmp(token->token_lexeme, "null") == 0){
        symbol->sym_variable_type = VAR_T_NULL;
    }
    else if(token->token_lexeme[0] == '"'){
        symbol->sym_variable_type = VAR_T_STRING;
    }else if (token->token_type == TOKEN_T_IDENTIFIER || token->token_type == TOKEN_T_GLOBAL_VAR) {
        return;
    }
    else {
        symbol->sym_variable_type = VAR_T_NUM;
    }
}

// INTERFACE FOR LEXER

Symbol *lexer_create_identifier_sym_from_token(Token *token){
    Symbol *symbol = malloc(sizeof(Symbol));
    
    symbol->sym_type = SYM_T_IDENTIFIER;
    symbol->sym_identif_type = IDENTIF_T_UNSET;
    symbol->is_global = 0;

    symbol->sym_function_number_of_params = NULL;

    symbol->sym_variable_type = VAR_T_UNSET;

    symbol->sym_lexeme_length = token->lexeme_length;
    copy_lexeme_from_token_to_sym(token, symbol);

    symbol->sym_identif_declaration_count = 0;
    symbol->sym_identif_use_count = 1;

    symbol->token = token;

    init_identif_sym_arrays(symbol, token);

    return symbol;
}

Symbol *lexer_create_global_var_sym_from_token(Token *token){
    Symbol *symbol = malloc(sizeof(Symbol));
    symbol->sym_type = SYM_T_IDENTIFIER;
    symbol->sym_identif_type = IDENTIF_T_VARIABLE;
    symbol->is_global = 1;
    
    symbol->sym_lexeme_length = token->lexeme_length;
    copy_lexeme_from_token_to_sym(token, symbol);

    symbol->sym_identif_declaration_count = 0;
    symbol->sym_identif_use_count = 1;
    init_identif_sym_arrays(symbol, token);

    return symbol;
}

Symbol *lexer_create_num_literal_sym_from_token(Token *token){
    Symbol *symbol = malloc(sizeof(Symbol));
    symbol->sym_type = SYM_T_LITERAL;
    symbol->sym_literal_type = LITERAL_T_NUM;

    symbol->sym_lexeme_length = token->lexeme_length;
    copy_lexeme_from_token_to_sym(token, symbol);

    symbol->sym_identif_declaration_count = 0;
    symbol->sym_identif_use_count = 1;
    init_identif_sym_arrays(symbol, token);

    return symbol;
}

Symbol *lexer_create_string_literal_sym_from_token(Token *token){
    Symbol *symbol = malloc(sizeof(Symbol));
    symbol->sym_type = SYM_T_LITERAL;
    symbol->sym_literal_type = LITERAL_T_STRING;

    symbol->sym_lexeme_length = token->lexeme_length;
    copy_lexeme_from_token_to_sym(token, symbol);

    symbol->sym_identif_declaration_count = 0;
    symbol->sym_identif_use_count = 1;
    init_identif_sym_arrays(symbol, token);

    return symbol;
}

// MODIFIERS FOR PARSER

void add_symbol_occurence(Symbol *symbol, int line_number, int col_number, int scope){
    symbol->sym_identif_use_count++;

    symbol->sym_identif_used_at_line_arr = realloc(symbol->sym_identif_used_at_line_arr, sizeof(int) * symbol->sym_identif_use_count);
    symbol->sym_identif_used_at_col_arr = realloc(symbol->sym_identif_used_at_col_arr, sizeof(int) * symbol->sym_identif_use_count);
    symbol->sym_identif_used_at_scope_arr = realloc(symbol->sym_identif_used_at_scope_arr, sizeof(int) * symbol->sym_identif_use_count);

    symbol->sym_identif_used_at_line_arr[symbol->sym_identif_use_count - 1] = line_number;
    symbol->sym_identif_used_at_col_arr[symbol->sym_identif_use_count - 1] = col_number;
    symbol->sym_identif_used_at_scope_arr[symbol->sym_identif_use_count - 1] = scope;
}

void symbol_add_function_params_count(Symbol *symbol, int number_of_params){
    if (symbol->sym_function_number_of_params == NULL){
        symbol->sym_function_number_of_params = malloc(sizeof(int)*symbol->sym_identif_declaration_count);
    } else {
        symbol->sym_function_number_of_params = realloc(symbol->sym_function_number_of_params, sizeof(int)*symbol->sym_identif_declaration_count);
    }

    if(symbol->sym_function_number_of_params == NULL){
    }
    symbol->sym_function_number_of_params[symbol->sym_identif_declaration_count-1] = number_of_params;
}

void copy_symbol_usage_info(Symbol *dest, Symbol *source) {
    if (dest == NULL || source == NULL) {
        return;
    }

    // Free existing usage arrays in destination
    if (dest->sym_identif_used_at_line_arr != NULL) {
        free(dest->sym_identif_used_at_line_arr);
    }
    if (dest->sym_identif_used_at_col_arr != NULL) {
        free(dest->sym_identif_used_at_col_arr);
    }
    if (dest->sym_identif_used_at_scope_arr != NULL) {
        free(dest->sym_identif_used_at_scope_arr);
    }

    // Copy use count
    dest->sym_identif_use_count = source->sym_identif_use_count;

    // Allocate and copy usage arrays
    if (source->sym_identif_use_count > 0) {
        dest->sym_identif_used_at_line_arr = malloc(sizeof(int) * source->sym_identif_use_count);
        dest->sym_identif_used_at_col_arr = malloc(sizeof(int) * source->sym_identif_use_count);
        dest->sym_identif_used_at_scope_arr = malloc(sizeof(int) * source->sym_identif_use_count);

        if (dest->sym_identif_used_at_line_arr != NULL &&
            dest->sym_identif_used_at_col_arr != NULL &&
            dest->sym_identif_used_at_scope_arr != NULL) {

            memcpy(dest->sym_identif_used_at_line_arr, source->sym_identif_used_at_line_arr,
                   sizeof(int) * source->sym_identif_use_count);
            memcpy(dest->sym_identif_used_at_col_arr, source->sym_identif_used_at_col_arr,
                   sizeof(int) * source->sym_identif_use_count);
            memcpy(dest->sym_identif_used_at_scope_arr, source->sym_identif_used_at_scope_arr,
                   sizeof(int) * source->sym_identif_use_count);
            }
    } else {
        dest->sym_identif_used_at_line_arr = NULL;
        dest->sym_identif_used_at_col_arr = NULL;
        dest->sym_identif_used_at_scope_arr = NULL;
    }
}

void print_symbol(Symbol *symbol) {

    // ALL
    switch (symbol->sym_type) {
    case SYM_T_IDENTIFIER:
        printf("Symbol type: IDENTIFIER\n");
        break;
    case SYM_T_LITERAL:
        printf("Symbol type: LITERAL\n");
        break;
    default:
        break;
    }

    printf("Symbol lexeme: %s\n", symbol->sym_lexeme);
    printf("Lexeme length: %i\n", symbol->sym_lexeme_length);

    // IDENTIF
    if(symbol->sym_type == SYM_T_IDENTIFIER){
        switch (symbol->sym_identif_type) {
            case IDENTIF_T_UNSET:
                printf("Identif type: UNSET\n");
                break;
            case IDENTIF_T_VARIABLE:
                printf("Identif type: VARIABLE\n");
                break;
            case IDENTIF_T_FUNCTION:
                printf("Identif type: FUNCTION\n"); 
                printf("PARAM COUNT: [");
                for(int i = 0; i < symbol->sym_identif_declaration_count;i++){
                    printf("%i,", symbol->sym_function_number_of_params[i]);
                }
                printf("]\n");
                break;
            case IDENTIF_T_GETTER:
                printf("Identif type: GETTER\n");
                break;
            case IDENTIF_T_SETTER:
                printf("Identif type: SETTER\n");
                break;
            default:
                break;
            }

        printf("Declaration count %i\n", symbol->sym_identif_declaration_count);

        for(int i = 0; i < symbol->sym_identif_declaration_count; i++) {
            printf("Declared at: line %i col %i scope %i\n", symbol->sym_identif_declared_at_line_arr[i], symbol->sym_identif_declared_at_col_arr[i], symbol->sym_identif_declared_at_scope_arr[i]);
        }

        printf("Use count %i\n", symbol->sym_identif_use_count);
        for(int i = 0; i < symbol->sym_identif_use_count; i++) {
            printf("Used at: line %i col %i scope %i\n", symbol->sym_identif_used_at_line_arr[i], symbol->sym_identif_used_at_col_arr[i], symbol->sym_identif_used_at_scope_arr[i]);
        }

        // VARIABLE ... tbd

        if(symbol->sym_identif_type == IDENTIF_T_VARIABLE){
            if(symbol->is_global){
                printf("Global: true\n");
            } else {
                printf("Global: false\n");
            }
        }

        // FUNCTION ... tbd
    }

    if(symbol->sym_identif_type == IDENTIF_T_VARIABLE){
        switch (symbol->sym_variable_type) {
            case VAR_T_NUM:
                printf("VAR type: NUM\n");
                break;
            case VAR_T_STRING:
                printf("VAR type: STRING\n");
                break;
            default: 
                break;
            }
    }

    if(symbol->sym_type == SYM_T_LITERAL){
        switch (symbol->sym_literal_type) {
            case LITERAL_T_NUM:
                printf("Literal type: NUM\n");
                break;
            case LITERAL_T_STRING:
                printf("Literal type: STRING\n");
                break;
            default:
                break;
            }
    }

    printf("\n");
}