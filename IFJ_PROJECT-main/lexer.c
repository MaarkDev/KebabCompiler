#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"
#include "symtable.h"
#include "token.h"

char *keyword_array[] = {"class", "if", "else", "is", "null", 
                "return", "var", "while", "Ifj", "static", 
                "import", "for", "Num", "String", "Null"
};

Lexer *init_lexer(Symtable *symtable){
    Lexer *lexer = malloc(sizeof(Lexer));
    if(lexer == NULL){
        return NULL;
    }

    lexer->symtable = symtable;

    lexer->lexeme_length = 0;
    lexer->current_col = 1;
    lexer->current_row = 1;

    lexer->token_count = 0;
    lexer->token_table = NULL;
    lexer->token_index = 0;

    lexer->lexeme = NULL;
    lexer->error = 0;

    lexer->scope = 0;
    lexer->max_scope = 0;
    lexer->previous_scope_arr = NULL;
    lexer->scope_index = 0; 

    lexer->newlines_in_multiline = 0;


    lexer->left_multiline_comment_end_sequence = 0;
    lexer->left_multiline_comment_start_sequence = 0;

    return lexer;
}

#include <stdlib.h>

int* extend_array(int *arr, int new_size, int new_scope_id, int prev_index) {
    // Allocate new array
    int *new_arr = (int*)malloc(new_size * sizeof(int));
    if (!new_arr) {
        perror("malloc failed");
        exit(1);
    }

    // Copy old values
    for (int i = 0; i < prev_index; i++) {
        new_arr[i] = arr[i];
    }

    // Add the previous scope at prev_index
    new_arr[prev_index] = new_scope_id;

    // Free old array
    free(arr);

    return new_arr;
}

void print_scope_array(int *arr, int size) {
    printf("Previous scopes: [");
    for (int i = 0; i < size; i++) {
        printf("%d", arr[i]);
        if (i < size - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}



void extend_lexeme(Lexer *lexer, char currect_char){
    lexer->lexeme_length++;
    lexer->lexeme = realloc(lexer->lexeme, sizeof(char) * lexer->lexeme_length + 1);
    lexer->lexeme[lexer->lexeme_length - 1] = currect_char;
    lexer->lexeme[lexer->lexeme_length] = '\0';
}

void add_token_to_token_table(Lexer *lexer, Token *token){
    lexer->token_count++;
    lexer->token_table = realloc(lexer->token_table, sizeof(Token) * lexer->token_count);
    lexer->token_table[lexer->token_count - 1] = *token;
}

void print_token_table(Lexer *lexer){
    printf("%i\n", lexer->token_count);
    for(int i = 0; i < lexer->token_count; i++){
        print_token(&(lexer->token_table[i]));
    }
}

Token *get_next_token(Lexer *lexer){
    Token *token;
    
    // if theres no more tokens to pass return null
    if(lexer->token_index == lexer->token_count){
        return NULL;
    }
    
    token = &(lexer->token_table[lexer->token_index]);
    lexer->token_index++;

    return token;
}

Token *get_lookahead_token(Lexer *lexer){
    Token *token;
    
    // if theres no more tokens to pass return null
    if(lexer->token_index == lexer->token_count){
        return NULL;
    }
    
    token = &(lexer->token_table[lexer->token_index]);
    return token;
}

void read_next_char(Lexer *lexer){
    lexer->current_char = getchar();
    lexer->current_col++;

    if(lexer->current_char == '{'){
        lexer->scope_index++;
        lexer->max_scope++;
        lexer->scope_id = lexer->max_scope + 100;
        lexer->previous_scope_arr = extend_array(
            lexer->previous_scope_arr, 
            lexer->scope_index, 
            lexer->scope_id, 
            lexer->scope_index - 1);
        
        lexer->scope = lexer->scope_id;
    } else if(lexer->current_char == '}'){
        lexer->scope_index--;
        lexer->scope = lexer->previous_scope_arr[lexer->scope_index - 1];
    }

    extend_lexeme(lexer, lexer->current_char);
}

int lexer_start(Lexer *lexer){
    // reset lexeme
    lexer->lexeme = NULL;
    lexer->lexeme_length = 0; 

    read_next_char(lexer);    

    // newline
    if(lexer->current_char == '\n'){ // ok :)
        final_state_end_of_line(lexer);
    }
    // whitespace
    else if(isspace(lexer->current_char)){ // ok
        lexer_start(lexer);
    }
    // 1-9
    else if(lexer->current_char >= '1' && lexer->current_char <= '9'){
        state_digit(lexer);
    }
    // 0
    else if(lexer->current_char == '0'){
        state_zero(lexer);
    }
    // "
    else if(lexer->current_char == '"'){
        state_string_reading(lexer);
    }
    // ,
    else if(lexer->current_char == ','){
        final_state_comma(lexer);
    }
    // (){}
    else if(lexer->current_char == '(' || lexer->current_char == ')' || lexer->current_char == '{' || lexer->current_char == '}'){
        final_state_brackets(lexer);
    }
    // /
    else if(lexer->current_char == '/'){
        state_comment_start(lexer);
    }
    // _
    else if(lexer->current_char == '_'){
        state_global1(lexer);
    }
    // !
    else if(lexer->current_char == '!'){
        state_exclamation_operator(lexer);
    }
    // +-*.
    else if(lexer->current_char == '+' || lexer->current_char == '-' || lexer->current_char == '*' || lexer->current_char == '.'){
        final_state_operator(lexer);
    }
    // ><=
    else if(lexer->current_char == '>' || lexer->current_char == '<' || lexer->current_char == '='){
        state_two_char_operator(lexer);
    }
    // a-z A-Z
    else if((lexer->current_char >= 'a' && lexer->current_char <= 'z') || (lexer->current_char >= 'A' && lexer->current_char <= 'Z')){
        state_id_start(lexer);
    }
    else {
        if(lexer->current_char != EOF){
            lexer->error = 1;
        }
    }

    return lexer->error;
}



// FSM STATES

void final_state_end_of_line(Lexer *lexer){
    lexer->current_row++;
    lexer->current_col = 1;
    Token *token = create_token(TOKEN_T_EOL, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    add_token_to_token_table(lexer, token);
    lexer_start(lexer);
    

}

void final_state_comma(Lexer *lexer){
    Token *token = create_token(TOKEN_T_COMMA, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    add_token_to_token_table(lexer, token);
    lexer_start(lexer);
}

void final_state_brackets(Lexer *lexer){
    Token *token = create_token(TOKEN_T_BRACKET, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    add_token_to_token_table(lexer, token);
    lexer_start(lexer);
}


void final_state_identif(Lexer *lexer){
    Token *token = create_token(TOKEN_T_IDENTIFIER, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    Symbol *symbol = search_table(token, lexer->symtable);
    if(symbol == NULL){
        token = create_token(TOKEN_T_IDENTIFIER, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);

        symbol = lexer_create_identifier_sym_from_token(token);
        insert_into_symtable(lexer->symtable, symbol);
    }else {
        add_symbol_occurence(symbol, lexer->current_row, lexer->current_col, lexer->scope);
    }
    add_token_to_token_table(lexer, token);
    lexer_start(lexer);
}

void final_state_keyword(Lexer *lexer){
    Token *token = create_token(TOKEN_T_KEYWORD, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    add_token_to_token_table(lexer, token);
    lexer_start(lexer);
}

void state_id_start(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);
    if(
        (lexer->current_char >= 'a' && lexer->current_char <= 'z' )||
        (lexer->current_char >= 'A' && lexer->current_char <= 'Z' )||
        (lexer->current_char >= '0' && lexer->current_char <= '9' )||
        lexer->current_char == '_'
    ) {
        read_next_char(lexer);
        state_id_start(lexer);
    } else {
        state_id_read(lexer);
    }
}

void state_id_read(Lexer *lexer){
    int not_id = 0;
    for(int i = 0; i < 15; i++){
        if(strcmp(lexer->lexeme, keyword_array[i]) == 0){
            not_id = 1;
            if(strcmp(lexer->lexeme, "is") == 0){
                final_state_operator(lexer);
            } else {
                final_state_keyword(lexer);
            }
        }
    }

    if(!not_id) {
        final_state_identif(lexer);
    }
}

void final_state_global_identif(Lexer *lexer){
    Token *token = create_token(TOKEN_T_GLOBAL_VAR, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    Symbol *symbol = search_table(token, lexer->symtable);
    if (symbol == NULL){
        symbol = lexer_create_global_var_sym_from_token(token);
    }
    add_token_to_token_table(lexer, token);
    insert_into_symtable(lexer->symtable, symbol);
    lexer_start(lexer);
}

int state_global1(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '_'){
        state_global2(lexer);
    } else {
        lexer->error = 1;
    }
    
    return lexer->error;
}

int state_global2(Lexer *lexer){
    read_next_char(lexer);
    if(
        (lexer->current_char >= 'a' && lexer->current_char <= 'z' )||
        (lexer->current_char >= 'A' && lexer->current_char <= 'Z' )||
        (lexer->current_char >= '0' && lexer->current_char <= '9' )||
        lexer->current_char == '_'
    ) {
        state_global3(lexer);
    } else {
        lexer->error = 1;
    }

    return lexer->error;
}

int state_global3(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);
    if(
        (lexer->current_char >= 'a' && lexer->current_char <= 'z' )||
        (lexer->current_char >= 'A' && lexer->current_char <= 'Z' )||
        (lexer->current_char >= '0' && lexer->current_char <= '9' )||
        lexer->current_char == '_'
    ) {
        read_next_char(lexer);
        state_global3(lexer);
    } else {
        final_state_global_identif(lexer);
    }

    return lexer->error;
}

void final_state_number(Lexer *lexer){
    Token *token = create_token(TOKEN_T_NUM, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    add_token_to_token_table(lexer, token);
    lexer_start(lexer);
}

int state_zero(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);
    if(lexer->current_char >= '0' && lexer->current_char <= '9'){
        read_next_char(lexer);
        state_digit(lexer);
    }
    else if(lexer->current_char == 'x'){
        read_next_char(lexer);
        state_hex_prefix(lexer);
    }
    else if(lexer->current_char == '.'){
        read_next_char(lexer);
        state_dot_arrived(lexer);
    }
    else {
        final_state_number(lexer);
    }

    return lexer->error;
}

int state_digit(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);
    if(lexer->current_char >= '0' && lexer->current_char <= '9'){
        read_next_char(lexer);
        state_digit(lexer);
    }
    else if(lexer->current_char == '.'){
        read_next_char(lexer);
        state_dot_arrived(lexer);
    }
    else if(lexer->current_char == 'E' || lexer->current_char == 'e'){
        read_next_char(lexer);
        state_exponent_prefix(lexer);
    }
    else {
        final_state_number(lexer);
    }

    return lexer->error;
}

int state_hex_prefix(Lexer *lexer){
    read_next_char(lexer);
    if((lexer->current_char >= '0' && lexer->current_char <= '9') ||
       (lexer->current_char >= 'a' && lexer->current_char <= 'f') ||
       (lexer->current_char >= 'A' && lexer->current_char <= 'F')
    ){
        state_hex_numba(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_hex_numba(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);
    if((lexer->current_char >= '0' && lexer->current_char <= '9') ||
       (lexer->current_char >= 'a' && lexer->current_char <= 'f') ||
       (lexer->current_char >= 'A' && lexer->current_char <= 'F')
    ){
        read_next_char(lexer);
        state_hex_numba(lexer);
    } else {
        final_state_number(lexer);
    }

    return lexer->error;
}

int state_dot_arrived(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char >= '0' && lexer->current_char <= '9'){
        state_decimal_part(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_decimal_part(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);
    if(lexer->current_char >= '0' && lexer->current_char <= '9'){
        read_next_char(lexer);
        state_decimal_part(lexer);
    }
    else if(lexer->current_char == 'e' || lexer->current_char == 'E'){
        read_next_char(lexer);
        state_exponent_prefix(lexer);
    }
    else {
        final_state_number(lexer);
    }
    return lexer->error;
}

int state_exponent_prefix(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char >= '0' && lexer->current_char <= '9'){
        state_exponent(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_exponent(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);
    if(lexer->current_char >= '0' && lexer->current_char <= '9'){
        read_next_char(lexer);
        state_exponent(lexer);
    } else {
        final_state_number(lexer);
    }
    return lexer->error;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void final_state_string(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);

    if(strcmp(lexer->lexeme, "\"\"") == 0 && lexer->current_char == '"'){
        read_next_char(lexer);
        state_multiline_string_reading(lexer);
    } else {
        Token *token = create_token(TOKEN_T_STRING, lexer->lexeme, lexer->lexeme_length, lexer->current_row - lexer->newlines_in_multiline, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
        add_token_to_token_table(lexer, token);
        lexer->newlines_in_multiline = 0;

        lexer_start(lexer);
    }
}

int state_string_reading(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char > 31 && lexer->current_char != 34){
        state_string_reading(lexer);
    } else if(lexer->current_char == '\\'){
        state_special_symbol(lexer);
    } else if(lexer->current_char == '"'){
        final_state_string(lexer);
    } else {
        lexer->error = 1;
    }

    return lexer->error;
}

int state_special_symbol(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '"' || lexer->current_char == 'n' || lexer->current_char == 'r' || lexer->current_char == 't' || lexer->current_char == '\\' ){
        state_string_reading(lexer);
    } else if (lexer->current_char == 'x'){
        state_string_hex_prefix(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_string_hex_prefix(Lexer *lexer){
    read_next_char(lexer);
    if((lexer->current_char >= '0' && lexer->current_char <= '9') ||
       (lexer->current_char >= 'a' && lexer->current_char <= 'f') ||
       (lexer->current_char >= 'A' && lexer->current_char <= 'F')
    ){
        state_string_hex_number(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_string_hex_number(Lexer *lexer){
    read_next_char(lexer);
    if((lexer->current_char >= '0' && lexer->current_char <= '9') ||
       (lexer->current_char >= 'a' && lexer->current_char <= 'f') ||
       (lexer->current_char >= 'A' && lexer->current_char <= 'F')
    ){
        state_string_reading(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_multiline_string_reading(Lexer *lexer){
    read_next_char(lexer);
    if((lexer->current_char > 31 && lexer->current_char != 34) || isspace(lexer->current_char) ){
        if(lexer->current_char == '\n'){
            lexer->newlines_in_multiline++;
            lexer->current_row++;
        }
        state_multiline_string_reading(lexer);
    } 
    else if(lexer->current_char == '\\'){
        state_multiline_special_symbol(lexer);
    } 
    else if(lexer->current_char == '"'){
        state_multiline_string1(lexer);
    }
    else {
        lexer->error = 1;
    }

    return lexer->error;
}

int state_multiline_string1(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '"'){
        state_multiline_string2(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_multiline_string2(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '"'){
        final_state_string(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_multiline_special_symbol(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '"' || lexer->current_char == 'n' || lexer->current_char == 'r' || lexer->current_char == 't' || lexer->current_char == '\\' ){
        state_multiline_string_reading(lexer);
    } else if (lexer->current_char == 'x'){
        state_multiline_string_hex_prefix(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_multiline_string_hex_prefix(Lexer *lexer){
    read_next_char(lexer);
    if((lexer->current_char >= '0' && lexer->current_char <= '9') ||
       (lexer->current_char >= 'a' && lexer->current_char <= 'f') ||
       (lexer->current_char >= 'A' && lexer->current_char <= 'F')
    ){
        state_multiline_string_hex_number(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}

int state_multiline_string_hex_number(Lexer *lexer){
    read_next_char(lexer);
    if((lexer->current_char >= '0' && lexer->current_char <= '9') ||
       (lexer->current_char >= 'a' && lexer->current_char <= 'f') ||
       (lexer->current_char >= 'A' && lexer->current_char <= 'F')
    ){
        state_multiline_string_reading(lexer);
    } else {
        lexer->error = 1;
    }
    return lexer->error;
}



void final_state_comment(Lexer *lexer){
    Token *token = create_token(TOKEN_T_EOL, "\n", 1, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    add_token_to_token_table(lexer, token);
    lexer->current_row++;
    lexer_start(lexer);
}

int state_comment_start(Lexer *lexer){

    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);

    if(lexer->current_char == '/'){
        read_next_char(lexer);
        state_comment_reading(lexer);
    } 
    else if (lexer->current_char == '*'){
        lexer->left_multiline_comment_start_sequence++;
        read_next_char(lexer);
        state_multiline_comment_reading(lexer);
    } 
    else {
        final_state_operator(lexer);
    }
    return lexer->error;
}

int state_comment_reading(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '\n'){
        final_state_comment(lexer);
    }else {
        state_comment_reading(lexer);
    }

    return lexer->error;
}

int state_multiline_comment_reading(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '/'){
        state_reading_comment_start_sequence(lexer);
    } 
    else if(lexer->current_char == '*') {
        state_reading_comment_end_sequence(lexer);
    }
    else if(lexer->current_char > 31 || (lexer->current_char != '\n' && isspace(lexer->current_char))){
        state_multiline_comment_reading(lexer);
    }
    else if(lexer->left_multiline_comment_start_sequence == lexer->left_multiline_comment_end_sequence){
        final_state_comment(lexer);
    }
    else if(lexer->current_char == '\n'){
        lexer->current_row++;
        state_multiline_comment_reading(lexer);
    }
    else {
        lexer->error = 1;
    }

    return lexer->error;
}

void state_reading_comment_start_sequence(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '*'){
        lexer->left_multiline_comment_start_sequence++;
    }
    
    state_multiline_comment_reading(lexer);
}

void state_reading_comment_end_sequence(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '/'){
        lexer->left_multiline_comment_end_sequence++;
    }
    
    state_multiline_comment_reading(lexer);
}


void final_state_operator(Lexer *lexer){
    Token *token = create_token(TOKEN_T_OPERATOR, lexer->lexeme, lexer->lexeme_length, lexer->current_row, lexer->current_col, lexer->scope, lexer->previous_scope_arr, lexer->scope_index);
    add_token_to_token_table(lexer, token);
    lexer_start(lexer);
}

void state_two_char_operator(Lexer *lexer){
    lexer->current_char = getc(stdin);
    ungetc(lexer->current_char, stdin);
    if(lexer->current_char == '='){
        read_next_char(lexer);
        final_state_operator(lexer);
    } else {
        final_state_operator(lexer);
    }
}

int state_exclamation_operator(Lexer *lexer){
    read_next_char(lexer);
    if(lexer->current_char == '='){
        final_state_operator(lexer);
    } else {
        lexer->error = 1;
    }

    return lexer->error;
}
