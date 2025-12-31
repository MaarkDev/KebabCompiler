#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdbool.h>

#include "token.h"

typedef enum { 
    SYM_T_IDENTIFIER,         
    SYM_T_LITERAL
} SYMBOL_TYPE;

typedef enum { 
    LITERAL_T_NUM,
    LITERAL_T_STRING,
} LITERAL_TYPE;

typedef enum {
    IDENTIF_T_VARIABLE,
    IDENTIF_T_FUNCTION,
    IDENTIF_T_SETTER,
    IDENTIF_T_GETTER,
    IDENTIF_T_UNSET
} IDENTIF_TYPE;

typedef enum {
    VAR_T_UNSET,
    VAR_T_NUM,
    VAR_T_STRING,
    VAR_T_NULL
} VARIABLE_TYPE;


typedef struct symbol {
    
    SYMBOL_TYPE sym_type;
    char *sym_lexeme;
    int sym_lexeme_length;

    // IDENTIF
    IDENTIF_TYPE sym_identif_type;

    Token *token;

    int sym_identif_declaration_count;
    int *sym_identif_declared_at_line_arr;
    int *sym_identif_declared_at_col_arr;
    int *sym_identif_declared_at_scope_arr;

    int sym_identif_use_count;
    int *sym_identif_used_at_line_arr;
    int *sym_identif_used_at_col_arr;
    int *sym_identif_used_at_scope_arr;

    // VARIABLE
    VARIABLE_TYPE sym_variable_type;
    float sym_variable_num_value;
    char *sym_variable_string_value;
    int is_global;
    bool is_parameter;

    // FUNCTION
    int *sym_function_number_of_params;
    char ***sym_function_param_names;
    SYMBOL_TYPE *sym_function_param_types; 

    // LITERAL
    LITERAL_TYPE sym_literal_type;
    float sym_literal_num_value;
    char *sym_literal_string_value;

} Symbol;

void copy_lexeme_from_token_to_sym(Token *token, Symbol *symbol);
void init_identif_sym_arrays(Symbol *symbol, Token *token);
void print_symbol(Symbol *symbol);
void copy_symbol_usage_info(Symbol *dest, Symbol *source);

Symbol *lexer_create_identifier_sym_from_token(Token *token);
Symbol *lexer_create_global_var_sym_from_token(Token *token);
Symbol *lexer_create_num_literal_sym_from_token(Token *token);
Symbol *lexer_create_string_literal_sym_from_token(Token *token);

void add_symbol_occurence(Symbol *symbol, int line_number, int col_number, int scope);
void symbol_set_var_type(Token *token, Symbol *symbol);
void symbol_add_function_params_count(Symbol *symbol, int number_of_params);


#endif