#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "symtable.h"

typedef struct lexer {
    int current_col;
    int current_row;

    Symtable *symtable;

    int error;

    int scope;
    int max_scope; 
    int *previous_scope_arr;
    int scope_index;
    int scope_id;

    int lexeme_length;
    char *lexeme;
    char current_char;

    Token *token_table;
    int token_count;

    int token_index;

    int newlines_in_multiline;

    int left_multiline_comment_start_sequence;
    int left_multiline_comment_end_sequence;
} Lexer;

Lexer *init_lexer(Symtable *symtable); 
void extend_lexeme(Lexer *lexer, char curent_char);
int lexer_start(Lexer *lexer);
void add_token_to_token_table(Lexer *lexer, Token *token);
void print_token_table(Lexer *lexer);

Token *get_next_token(Lexer *lexer);
Token *get_lookahead_token(Lexer *lexer);

void final_state_end_of_line(Lexer *lexer);
void final_state_comma(Lexer *lexer);
void final_state_brackets(Lexer *lexer);

void final_state_identif(Lexer *lexer);
void final_state_keyword(Lexer *lexer);
void state_id_start(Lexer *lexer);
void state_id_read(Lexer *lexer);

void final_state_global_identif(Lexer *lexer);
int state_global1(Lexer *lexer);
int state_global2(Lexer *lexer);
int state_global3(Lexer *lexer);

void final_state_number(Lexer *lexer);
int state_zero(Lexer *lexer);
int state_digit(Lexer *lexer);
int state_hex_prefix(Lexer *lexer);
int state_hex_numba(Lexer *lexer);
int state_dot_arrived(Lexer *lexer);
int state_decimal_part(Lexer *lexer);
int state_exponent_prefix(Lexer *lexer);
int state_exponent(Lexer *lexer);

void final_state_string(Lexer *lexer);
int state_string_reading(Lexer *lexer);
int state_special_symbol(Lexer *lexer);
int state_string_hex_prefix(Lexer *lexer);
int state_string_hex_number(Lexer *lexer);
int state_multiline_string_reading(Lexer *lexer);
int state_multiline_string1(Lexer *lexer);
int state_multiline_string2(Lexer *lexer);
int state_multiline_special_symbol(Lexer *lexer);
int state_multiline_string_hex_prefix(Lexer *lexer);
int state_multiline_string_hex_number(Lexer *lexer);

void final_state_comment(Lexer *lexer);
int state_comment_start(Lexer *lexer);
int state_comment_reading(Lexer *lexer);
int state_multiline_comment_reading(Lexer *lexer);
void state_reading_comment_start_sequence(Lexer *lexer);
void state_reading_comment_end_sequence(Lexer *lexer);


void final_state_operator(Lexer *lexer);
void state_two_char_operator(Lexer *lexer);
int state_exclamation_operator(Lexer *lexer);


int* extend_array(int *int_arr, int number_of_elements, int value_to_add, int index);

#endif