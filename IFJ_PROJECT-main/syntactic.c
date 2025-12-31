#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "syntactic.h"
#include "lexer.h"
#include "token.h"
#include "tree.h"
#include "utils.h"
#include "symtable.h"

Syntactic *init_syntactic(Symtable *symtable){
    Syntactic *syntactic = malloc(sizeof(Syntactic));
    if(!syntactic){
        return NULL;
    }

    syntactic->symtable = symtable;
    syntactic->scope_counter = 0;
    syntactic->fn_number_of_params = 0;
    tree_init(&syntactic->tree);

    syntactic->error = 0;

    return syntactic;
}

int syntactic_start(Syntactic *syntactic, Lexer *lexer){
    rule_check_skeleton_1(syntactic, lexer);
    return syntactic->error;
}

// returns 1 on succes
int assert_expected_literal(Syntactic *syntactic, Token* token, char *literal){
    if (!token) { syntactic->error = ERR_T_SYNTAX_ERR; return 0; };
    if(strcmp(token->token_lexeme, literal) != 0){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return 0;
    }
    return 1;
}

int rule_check_skeleton_1(Syntactic *syntactic, Lexer *lexer){
    Token *token = get_next_token(lexer);
    if (!token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    if(strcmp(token->token_lexeme, "import") != 0){
        syntactic->error = ERR_T_SYNTAX_ERR;
    }

    token = get_next_token(lexer);
    if (!token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    if(strcmp(token->token_lexeme, "\"ifj25\"") != 0){
        syntactic->error = ERR_T_SYNTAX_ERR;
    }

    token = get_next_token(lexer);
    if (!token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    if(strcmp(token->token_lexeme, "for") != 0){
        syntactic->error = ERR_T_SYNTAX_ERR;
    }

    token = get_next_token(lexer);
    if (!token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    if(strcmp(token->token_lexeme, "Ifj") != 0){
        syntactic->error = ERR_T_SYNTAX_ERR;
    }

    token = get_next_token(lexer);
    if (!token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    if(strcmp(token->token_lexeme, "\n") != 0){
        syntactic->error = ERR_T_SYNTAX_ERR;
    }

    if(syntactic->error != 0){
        return syntactic->error;
    } else {
        rule_check_skeleton_2(syntactic, lexer);
        if(syntactic->error != 0) return syntactic->error;
        
    }

    return syntactic->error;

}

int rule_check_skeleton_2(Syntactic *syntactic, Lexer *lexer){
    Token *token = get_next_token(lexer);
    if (!token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    if(strcmp(token->token_lexeme, "class") != 0){
        syntactic->error = ERR_T_SYNTAX_ERR;
    }

    token = get_next_token(lexer);
    if (!token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    if(strcmp(token->token_lexeme, "Program") != 0){
        syntactic->error = ERR_T_SYNTAX_ERR;
    }

    if(syntactic->error != 0){
        return syntactic->error;
    } else {
        rule_code_block(syntactic, lexer, syntactic->tree);
        if(syntactic->error != 0) return syntactic->error;
    }

    return syntactic->error;
}

int rule_code_block(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_code_block_node = tree_create_nonterminal(NONTERMINAL_T_CODE_BLOCK, GR_CODE_BLOCK);

    // increase scope
    syntactic->scope_counter++;

    Token *current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "{")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }

    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "\n")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }

    if(syntactic->error != 0){
        return syntactic->error;
    }

    rule_sequence(syntactic, lexer, rule_code_block_node);

    if(syntactic->error != 0) return syntactic->error;


    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "}")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }

    if(syntactic->error != 0){
        return syntactic->error;
    }


    tree_insert_child(node, rule_code_block_node);

    // decrease scope
    syntactic->scope_counter--;

    return syntactic->error;  
}

int is_instruction_start(Token *token){
    char *instruction_start_ar[] = { "return", "if", "while", "var", "static", "{" };
    int is_start = 0;

    if(token->token_type == TOKEN_T_KEYWORD || strcmp(token->token_lexeme, "{") == 0){
        for (int i = 0; i < 6; i++) {
            if (strcmp(token->token_lexeme, instruction_start_ar[i]) == 0) {
                return 1;
            }
        }
    }

    if(token->token_type == TOKEN_T_IDENTIFIER || token->token_type == TOKEN_T_GLOBAL_VAR){
        is_start = 1;
    }

    return is_start;
}

int rule_sequence(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_sequence_node = node;

    Token *lookahead_token = get_lookahead_token(lexer);
    if (!lookahead_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };

    if (!(lookahead_token->token_type == TOKEN_T_EOL || is_instruction_start(lookahead_token))) {
        // Sequence can be empty → return success
        return syntactic->error;
    }

    // Handle <EOL>
    if (lookahead_token->token_type == TOKEN_T_EOL) {
        Token *t = get_next_token(lexer); // consume EOL
        if (!t) {
            syntactic->error = ERR_T_SYNTAX_ERR;
            return syntactic->error;
        }
    }
    // Handle INSTRUCTION
    else if (is_instruction_start(lookahead_token)) {
        rule_instruction(syntactic, lexer, rule_sequence_node);

        if (syntactic->error != 0)
            return syntactic->error;
    }

    rule_sequence_prime(syntactic, lexer, rule_sequence_node);

    return syntactic->error;
}
int rule_sequence_prime(Syntactic *syntactic, Lexer *lexer, tree_node_t *node) {
    tree_node_t *rule_sequence_prime_node = node;

    Token *lookahead_token = get_lookahead_token(lexer);
    if (!lookahead_token) {
        return syntactic->error;
    }

    // If token doesnt start a new instruction or EOL
    if (lookahead_token->token_type != TOKEN_T_EOL && !is_instruction_start(lookahead_token)) {
        return syntactic->error;
    }

    // Handle <EOL>
    if (lookahead_token->token_type == TOKEN_T_EOL) {
        Token *t = get_next_token(lexer); // consume newline
        if (!t) {
            syntactic->error = ERR_T_SYNTAX_ERR;
            return syntactic->error;
        }
    }
    // Handle INSTRUCTION
    else if (is_instruction_start(lookahead_token)) {
        rule_instruction(syntactic, lexer, rule_sequence_prime_node);
        if (syntactic->error != 0)
            return syntactic->error;
    }

    rule_sequence_prime(syntactic, lexer, rule_sequence_prime_node);

    return syntactic->error;
}

int rule_instruction(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_instruction_node = node;
    Token *lookahead_token = get_lookahead_token(lexer);
    if (!lookahead_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };

    if(strcmp(lookahead_token->token_lexeme, "return") == 0){
        rule_return(syntactic, lexer, rule_instruction_node);
    } else if(strcmp(lookahead_token->token_lexeme, "if") == 0){
        rule_if(syntactic, lexer, rule_instruction_node);
    } else if(strcmp(lookahead_token->token_lexeme, "while") == 0){
        rule_while(syntactic, lexer, rule_instruction_node);
    } else if(strcmp(lookahead_token->token_lexeme, "var") == 0){
        rule_declaration(syntactic, lexer, rule_instruction_node);
    } else if(lookahead_token->token_type == TOKEN_T_IDENTIFIER || lookahead_token->token_type == TOKEN_T_GLOBAL_VAR){
        rule_assignment(syntactic, lexer, rule_instruction_node);
    } else if(strcmp(lookahead_token->token_lexeme, "static") == 0){
        syntactic->fn_number_of_params = 0;
        rule_function_declaration_begin(syntactic, lexer, rule_instruction_node);
    } else if(strcmp(lookahead_token->token_lexeme, "{") == 0){
        rule_code_block(syntactic, lexer, rule_instruction_node);
    } else {
        syntactic->error = ERR_T_SYNTAX_ERR;
    }

    return syntactic->error;
}

int rule_return(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_return_node = tree_create_nonterminal(NONTERMINAL_T_RETURN, GR_RETURN);

    Token *current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "return")){
        return syntactic->error;
    }

    // ZA RETURN NEMUSI BYT NIC
    Token *lookahead = get_lookahead_token(lexer);
    if(lookahead->token_type == TOKEN_T_EOL){
        return syntactic->error;
    }

    tree_node_t *rule_exp_node = rule_expression(syntactic, lexer);
    if(syntactic->error != 0) return syntactic->error;

    tree_insert_child(rule_return_node, rule_exp_node);

    tree_insert_child(node, rule_return_node);


    return syntactic->error;
}

int rule_if(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_if_node = tree_create_nonterminal(NONTERMINAL_T_IF, GR_IF);

    Token *current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "if")){
        return syntactic->error;
    }
    current_token = get_next_token(lexer);

    if(!assert_expected_literal(syntactic, current_token, "(")){
        return syntactic->error;
    }

    tree_node_t *rule_predicate_node = tree_create_nonterminal(NONTERMINAL_T_PREDICATE, GR_PREDICATE_PARENTH);
    tree_node_t *preditace = rule_predicate(syntactic, lexer);

    tree_insert_child(rule_predicate_node, preditace);
    tree_insert_child(rule_if_node, rule_predicate_node);

    if(syntactic->error != 0) return syntactic->error;

    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, ")")){
        return syntactic->error;
    }

    rule_code_block(syntactic, lexer, rule_if_node);
    
    if(syntactic->error != 0) return syntactic->error;

    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "else")){
        return syntactic->error;
    }

    rule_code_block(syntactic, lexer, rule_if_node);

    if(syntactic->error != 0) return syntactic->error;

    tree_insert_child(node, rule_if_node);

    return syntactic->error;
}

int rule_while(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_while_node = tree_create_nonterminal(NONTERMINAL_T_WHILE, GR_WHILE);

    Token *current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "while")){
        return syntactic->error;
    }
    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "(")){
        return syntactic->error;
    }

    tree_node_t *rule_predicate_node = tree_create_nonterminal(NONTERMINAL_T_PREDICATE, GR_PREDICATE_PARENTH);
    tree_node_t *preditace = rule_predicate(syntactic, lexer);
    tree_insert_child(rule_predicate_node, preditace);
    tree_insert_child(rule_while_node, rule_predicate_node);

    if(syntactic->error != 0) return syntactic->error;

    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, ")")){
        return syntactic->error;
    }
    rule_code_block(syntactic, lexer, rule_while_node);
    if(syntactic->error != 0) return syntactic->error;

    tree_insert_child(node, rule_while_node);

    return syntactic->error;
}

int rule_declaration(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_declaration_node = tree_create_nonterminal(NONTERMINAL_T_DECLARATION, GR_DECLARATION);
    // assert var
    Token *current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "var")){
        return syntactic->error;
    }
    // assert identif
    current_token = get_next_token(lexer);
    if (!current_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };

    if(current_token->token_type != TOKEN_T_IDENTIFIER && current_token->token_type != TOKEN_T_GLOBAL_VAR){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }

    tree_node_t *indentif_node = tree_create_terminal(current_token);
    tree_insert_child(rule_declaration_node, indentif_node);

    // HOTFIX
    lexer->scope = syntactic->scope_counter;

    // update symbol table
    Symbol *symbol = search_table(current_token, syntactic->symtable);
    symbol->sym_identif_type = IDENTIF_T_VARIABLE;
    symtable_add_declaration_info(symbol, current_token->token_line_number, current_token->token_col_number, current_token->scope);
    
    tree_insert_child(node, rule_declaration_node);

    return syntactic->error;
}

int rule_allowed_eol(Syntactic *syntactic, Lexer *lexer) {
    Token *current_token = get_next_token(lexer);

    if (strcmp(current_token->token_lexeme, ",") != 0 && current_token->token_type != TOKEN_T_OPERATOR) {
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    // Allow one or more newlines after
    current_token = get_lookahead_token(lexer); // peek to see if there's a newline next
    if (strcmp(current_token->token_lexeme, "\n") != 0) {
        return syntactic->error;
    }

    // Consume all consecutive newlines
    do {
        current_token = get_lookahead_token(lexer);
    } while (strcmp(current_token->token_lexeme, "\n") == 0);

    return syntactic->error;
}


int rule_assignment(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_assignment_node = tree_create_nonterminal(NONTERMINAL_T_ASSIGNMENT, GR_ASSIGNMENT);

    Token *current_token = get_next_token(lexer);
    if (!current_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };

    if(current_token->token_type != TOKEN_T_IDENTIFIER && current_token->token_type != TOKEN_T_GLOBAL_VAR){
        syntactic->error = ERR_T_SYNTAX_ERR; 
        return syntactic->error;
    }

    // ADD OCCURENCE
    Symbol *symbol_to_update = search_table(current_token, syntactic->symtable); // identif
    add_symbol_occurence(symbol_to_update, current_token->token_line_number, current_token->token_col_number, current_token->scope);

    tree_node_t *identif_node = tree_create_terminal(current_token);
    tree_insert_child(rule_assignment_node, identif_node);

    Token *identif_token_to_be_updated = current_token;

    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "=")){
        syntactic->error = ERR_T_SYNTAX_ERR; 
        return syntactic->error;
    }
    symbol_to_update = search_table(identif_token_to_be_updated, syntactic->symtable); // identif

    // based on what to update
    Token *lookahead_token = get_lookahead_token(lexer);

    symbol_set_var_type(lookahead_token, symbol_to_update);

    rule_expression_or_function(syntactic, lexer, rule_assignment_node);
    
    if(syntactic->error != 0) return syntactic->error;

    tree_insert_child(node, rule_assignment_node);

    return syntactic->error;
}

int rule_expression_or_function(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_expression_or_fn_node = node;

    Token *current_token = get_next_token(lexer);
    if (!current_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    Token *lookahead_token = get_lookahead_token(lexer);
    if (!lookahead_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };
    
    lexer->token_index--;

    if(current_token->token_type == TOKEN_T_STRING || current_token->token_type == TOKEN_T_NUM || current_token->token_type == TOKEN_T_GLOBAL_VAR){
        tree_node_t *exp_node = rule_expression(syntactic, lexer);
        tree_insert_child(rule_expression_or_fn_node, exp_node);
        if(syntactic->error != 0) return syntactic->error;
    } else {
        if (strcmp(lookahead_token->token_lexeme, "(") == 0 || strcmp(lookahead_token->token_lexeme, ".") == 0) {
            rule_function_call(syntactic, lexer, rule_expression_or_fn_node);
            if(syntactic->error != 0) return syntactic->error;
        }
        else {
            tree_node_t *exp_node = rule_expression(syntactic, lexer);
            tree_insert_child(rule_expression_or_fn_node, exp_node);
            if(syntactic->error != 0) return syntactic->error;
        }
    }

    return syntactic->error;
}

void add_prefix(Token *token, char *prefix) {
    size_t prefix_len = strlen(prefix);
    size_t old_len = token->lexeme_length;

    token->lexeme_length = old_len + prefix_len;

    char *new_ptr = realloc(token->token_lexeme,
                            token->lexeme_length + 1);
    if (!new_ptr) {
        return;
    }
    token->token_lexeme = new_ptr;

    memmove(token->token_lexeme + prefix_len,
            token->token_lexeme,
            old_len + 1);

    memcpy(token->token_lexeme, prefix, prefix_len);
}

void add_prefix_to_symbol(Symbol *symbol, char *prefix) {
    if (symbol == NULL || prefix == NULL) {
        return;
    }

    size_t prefix_len = strlen(prefix);
    size_t old_len = symbol->sym_lexeme_length;

    symbol->sym_lexeme_length = old_len + prefix_len;

    char *new_ptr = realloc(symbol->sym_lexeme, symbol->sym_lexeme_length + 1);
    if (!new_ptr) {
        return;
    }

    symbol->sym_lexeme = new_ptr;

    // Move existing lexeme to make room for prefix
    memmove(symbol->sym_lexeme + prefix_len,
            symbol->sym_lexeme,
            old_len + 1);

    // Copy prefix to the beginning
    memcpy(symbol->sym_lexeme, prefix, prefix_len);
}

int rule_function_declaration_begin(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_fn_dec_begin_node = tree_create_nonterminal(NONTERMINAL_T_DECLARATION, GR_FUN_DECLARATION);

    Token *current_token = get_next_token(lexer);
    if (!current_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };

    if(!assert_expected_literal(syntactic, current_token, "static")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    current_token = get_next_token(lexer);
    if(current_token->token_type != TOKEN_T_IDENTIFIER ){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    //TODO DOPLNIT

    tree_node_t *identif_node = tree_create_terminal(current_token);
    tree_insert_child(rule_fn_dec_begin_node, identif_node);

    Token *lookahead_token = get_lookahead_token(lexer);
    if (!lookahead_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };

    // function
    if(strcmp(lookahead_token->token_lexeme, "(") == 0){
        // HOTFIX
        lexer->scope = syntactic->scope_counter;

        // Find the original symbol created by lexer
        Symbol *original_symbol = search_table(current_token, syntactic->symtable);

        if (original_symbol == NULL) {
            syntactic->error = 0;
            return syntactic->error;
        }

        // Mark it as a function and add declaration info
        original_symbol->sym_identif_type = IDENTIF_T_FUNCTION;
        symtable_add_declaration_info(original_symbol, current_token->token_line_number,
                                       current_token->token_col_number, syntactic->scope_counter);

        syntactic->fn_number_of_params = 0;
        rule_function_declaration(syntactic, lexer, rule_fn_dec_begin_node);

        symbol_add_function_params_count(original_symbol, syntactic->fn_number_of_params);

    } else if (strcmp(lookahead_token->token_lexeme, "=") == 0) { // setter
        // HOTFIX
        lexer->scope = syntactic->scope_counter;

        // Find the original symbol created by lexer
        Symbol *original_symbol = search_table(current_token, syntactic->symtable);

        if (original_symbol == NULL) {
            syntactic->error = 0;
            return syntactic->error;
        }

        // Create a new setter symbol with prefix
        Symbol *setter_symbol = lexer_create_identifier_sym_from_token(current_token);
        add_prefix_to_symbol(setter_symbol, "setter+");

        // Copy usage info from original symbol (where lexer put it)
        copy_symbol_usage_info(setter_symbol, original_symbol);

        // Set setter-specific properties
        setter_symbol->sym_identif_type = IDENTIF_T_SETTER;
        symtable_add_declaration_info(setter_symbol, current_token->token_line_number,
                                       current_token->token_col_number, syntactic->scope_counter);

        // Insert the setter symbol into symtable
        insert_into_symtable(syntactic->symtable, setter_symbol);

        rule_setter_declaration(syntactic, lexer, rule_fn_dec_begin_node);

    } else { // getter
        // HOTFIX
        lexer->scope = syntactic->scope_counter;

        // Find the original symbol created by lexer
        Symbol *original_symbol = search_table(current_token, syntactic->symtable);

        if (original_symbol == NULL) {
            syntactic->error = 0;
            return syntactic->error;
        }

        // Create a new getter symbol with prefix
        Symbol *getter_symbol = lexer_create_identifier_sym_from_token(current_token);
        add_prefix_to_symbol(getter_symbol, "getter+");

        // Copy usage info from original symbol (where lexer put it)
        copy_symbol_usage_info(getter_symbol, original_symbol);

        // Set getter-specific properties
        getter_symbol->sym_identif_type = IDENTIF_T_GETTER;
        symtable_add_declaration_info(getter_symbol, current_token->token_line_number,
                                       current_token->token_col_number, syntactic->scope_counter);

        // Insert the getter symbol into symtable
        insert_into_symtable(syntactic->symtable, getter_symbol);

        rule_getter_declaration(syntactic, lexer, rule_fn_dec_begin_node);
    }

    tree_insert_child(node, rule_fn_dec_begin_node);

    return syntactic->error;
}

int rule_function_declaration(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_function_declaration_node = node;

    Token *current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "(")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }

    rule_function_parameters(syntactic, lexer, rule_function_declaration_node, true);

    if(syntactic->error != 0) return syntactic->error;

    int actual_param_count = syntactic->fn_number_of_params;

    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, ")")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    rule_code_block(syntactic, lexer, rule_function_declaration_node);

    if(syntactic->error != 0) return syntactic->error;

    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "\n")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }

    syntactic->fn_number_of_params = actual_param_count;

    return syntactic->error;
}

int rule_function_call(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_function_call_node = tree_create_nonterminal(NONTERMINAL_T_FUN_CALL, GR_FUN_CALL);

    Token *current_token = get_next_token(lexer);
    if (!current_token) { syntactic->error = ERR_T_SYNTAX_ERR; return syntactic->error = ERR_T_SYNTAX_ERR; };

    if(current_token->token_type == TOKEN_T_IDENTIFIER ){
        
        tree_node_t *func = tree_create_terminal(current_token);
        tree_insert_child(rule_function_call_node, func);

        current_token = get_next_token(lexer);
        if(!assert_expected_literal(syntactic, current_token, "(")){
            syntactic->error = ERR_T_SYNTAX_ERR;
            return syntactic->error;
        }
        rule_function_parameters(syntactic, lexer, rule_function_call_node, false);

        if(syntactic->error != 0) return syntactic->error;

        current_token = get_next_token(lexer);
        if(!assert_expected_literal(syntactic, current_token, ")")){
            syntactic->error = ERR_T_SYNTAX_ERR;
            return syntactic->error;
        }
    } else if (strcmp(current_token->token_lexeme, "Ifj") == 0){
        rule_allowed_eol(syntactic, lexer);

        if(syntactic->error != 0) return syntactic->error;

        current_token = get_next_token(lexer);
        if(current_token->token_type != TOKEN_T_IDENTIFIER ){
            syntactic->error = ERR_T_SYNTAX_ERR;
            return syntactic->error;
        }

        tree_node_t *identif_node_1 = tree_create_terminal(current_token);
        tree_insert_child(rule_function_call_node, identif_node_1);

        current_token = get_next_token(lexer);
        if(!assert_expected_literal(syntactic, current_token, "(")){
            syntactic->error = ERR_T_SYNTAX_ERR;
            return syntactic->error;
        }
       
        rule_function_parameters(syntactic, lexer, rule_function_call_node, false);

        if(syntactic->error != 0) return syntactic->error;

        current_token = get_next_token(lexer);
        if(!assert_expected_literal(syntactic, current_token, ")")){
            syntactic->error = ERR_T_SYNTAX_ERR;
            return syntactic->error;
        }
    }


    tree_insert_child(node, rule_function_call_node);

    return syntactic->error;
}

int rule_function_parameters(Syntactic *syntactic, Lexer *lexer, tree_node_t *node, bool is_declaration){
    tree_node_t *rule_function_parameters_node = tree_create_nonterminal(NONTERMINAL_T_FUN_PARAM, GR_FUN_PARAM);
    
    Token *lookahead_token = get_lookahead_token(lexer);
    
    if(strcmp(lookahead_token->token_lexeme, ")") == 0){
        return syntactic->error;
    }
    if(lookahead_token->token_type != TOKEN_T_STRING && lookahead_token->token_type != TOKEN_T_IDENTIFIER){
        tree_node_t *exp_node = rule_expression(syntactic, lexer);
        tree_insert_child(rule_function_parameters_node, exp_node);
        if(syntactic->error != 0) return syntactic->error;
    }
    else if(lookahead_token->token_type == TOKEN_T_IDENTIFIER){
        Token *t = get_next_token(lexer);
        Symbol *symbol = search_table(t, syntactic->symtable);

        tree_node_t *str_node = tree_create_terminal(t);
        tree_insert_child(rule_function_parameters_node, str_node);

        if(is_declaration){
            symtable_add_declaration_info(symbol, t->token_line_number, t->token_col_number, syntactic->scope_counter+101);
            symbol->is_parameter = true;
        }
    }
    
    else { //consume if string
        Token *t = get_next_token(lexer);

        tree_node_t *str_node = tree_create_terminal(t);
        tree_insert_child(rule_function_parameters_node, str_node);
    }

    syntactic->fn_number_of_params++;
    rule_function_parameters_prime(syntactic, lexer, rule_function_parameters_node, is_declaration);

    tree_insert_child(node, rule_function_parameters_node);

    return syntactic->error;
}

int rule_function_parameters_prime(Syntactic *syntactic, Lexer *lexer, tree_node_t *node, bool is_declaration) {
    tree_node_t *rule_function_params_prime_node = node;

    Token *lookahead_token = get_lookahead_token(lexer);

    if (strcmp(lookahead_token->token_lexeme, ",") != 0) {
        return syntactic->error;
    }

    rule_allowed_eol(syntactic, lexer);

    // uz je v allowed_eol vynutena ciarka

    // Parse next EXPRESSION
    if(lookahead_token->token_type != TOKEN_T_STRING){
        tree_node_t *exp_node = rule_expression(syntactic, lexer);
        tree_insert_child(rule_function_params_prime_node, exp_node);
        if(syntactic->error != 0) return syntactic->error;
    } else { // ak je to string tak ho skonzumuj
        Token *t = get_next_token(lexer);
        tree_node_t *str_node = tree_create_terminal(t);
        tree_insert_child(rule_function_params_prime_node, str_node);
    }

    syntactic->fn_number_of_params++;
    // Recurse for more parameters
    rule_function_parameters_prime(syntactic, lexer, rule_function_params_prime_node, is_declaration);

    return syntactic->error;
}

int rule_getter_declaration(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_getter_declaration_node = tree_create_nonterminal(NONTERMINAL_T_DECLARATION, GR_GETTER_DECLARATION);

    rule_code_block(syntactic, lexer, rule_getter_declaration_node);

    if(syntactic->error != 0) return syntactic->error;

    Token *current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "\n")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    tree_insert_child(node, rule_getter_declaration_node);

    return syntactic->error;
}

int rule_setter_declaration(Syntactic *syntactic, Lexer *lexer, tree_node_t *node){
    tree_node_t *rule_setter_declaration_node = tree_create_nonterminal(NONTERMINAL_T_DECLARATION, GR_SETTER_DECLARATION);

    Token *current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "=")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "(")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    current_token = get_next_token(lexer);
    if(current_token->token_type != TOKEN_T_IDENTIFIER){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    Symbol *symbol = search_table(current_token, syntactic->symtable);
    if(!symbol){
        return 0;
    }

    symtable_add_declaration_info(symbol, current_token->token_line_number, current_token->token_col_number, syntactic->scope_counter+101);
    symbol->is_parameter = true;

    tree_node_t *identif_node = tree_create_terminal(current_token);
    tree_insert_child(rule_setter_declaration_node, identif_node);

    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, ")")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    rule_code_block(syntactic, lexer, rule_setter_declaration_node);

    if(syntactic->error != 0) return syntactic->error;
    
    current_token = get_next_token(lexer);
    if(!assert_expected_literal(syntactic, current_token, "\n")){
        syntactic->error = ERR_T_SYNTAX_ERR;
        return syntactic->error;
    }
    tree_insert_child(node, rule_setter_declaration_node);

    return syntactic->error;
}

int is_binary_operator(Token *token){
    if(!token) return 0;

    if(token->token_type == TOKEN_T_OPERATOR && strcmp(token->token_lexeme, ".") != 0){
        return 1;
    }
    return 0;
}

int get_operator_precedence(Token *token){

    if(!token) return 0;

    int op_priority;

    char *op1 = token->token_lexeme;

    if(strcmp(op1, "*") == 0 || strcmp(op1, "/") == 0){
        op_priority = 5;
    } else if (strcmp(op1, "+") == 0 || strcmp(op1, "-") == 0){
        op_priority = 4;
    } else if (strcmp(op1, "<") == 0 || strcmp(op1, ">") == 0 || strcmp(op1, "<=") == 0 || strcmp(op1, ">=") == 0){
        op_priority = 3;
    } else if (strcmp(op1, "is") == 0){
        op_priority = 2;
    } else if (strcmp(op1, "==") == 0 || strcmp(op1, "!=") == 0){
        op_priority = 1;
    } else {
        return -1;
    }

    return op_priority;
}

tree_node_t *rule_expression(Syntactic *syntactic, Lexer *lexer){
    tree_node_t *primary = rule_parse_primary(syntactic, lexer);
    return rule_expression_1(primary, 0, syntactic, lexer);
}

tree_node_t *rule_expression_1(tree_node_t *lhs, int min_precedence, Syntactic *syntactic, Lexer *lexer) {
    Token *lookahead = get_lookahead_token(lexer);
    if (!lookahead) { syntactic->error = ERR_T_SYNTAX_ERR; return lhs; };

    while (is_binary_operator(lookahead) && get_operator_precedence(lookahead) >= min_precedence) {
        Token *op = lookahead;
        get_next_token(lexer); // consume operator

        tree_node_t *rhs = rule_parse_primary(syntactic, lexer); 
        
        // ---------------------------------------------------

        lookahead = get_lookahead_token(lexer);

        while (is_binary_operator(lookahead) && get_operator_precedence(lookahead) > get_operator_precedence(op)) {
            
            int optional_increment;
            if(get_operator_precedence(op) < get_operator_precedence(lookahead)){
                optional_increment = 1;
            } else {
                optional_increment = 0;
            }
            
        
            rhs = rule_expression_1(rhs, get_operator_precedence(op) + optional_increment, syntactic, lexer);
            lookahead = get_lookahead_token(lexer);
        }

        // ---------------------------------------------------

        tree_node_t *node_op;
        tree_init(&node_op);
        node_op->token = op;
        node_op->type = NODE_T_NONTERMINAL;  // ADDED
        node_op->nonterm_type = NONTERMINAL_T_EXPRESSION;  // ADDED

        tree_insert_child(node_op, lhs);
        tree_insert_child(node_op, rhs);

        lhs = node_op;
    }

    return lhs;
}

tree_node_t *rule_parse_primary(Syntactic *syntactic, Lexer *lexer) {
    Token *current_token = get_next_token(lexer);

    tree_node_t *root;
    tree_init(&root);

    if(strcmp(current_token->token_lexeme, "(") == 0) {
        tree_node_t *expr_node = rule_expression(syntactic, lexer);
        if(syntactic->error != 0) return NULL;

        Token *end_token = get_next_token(lexer);
        if(!end_token || strcmp(end_token->token_lexeme, ")") != 0){
            syntactic->error = ERR_T_SYNTAX_ERR;
            return NULL;
        }

        return expr_node;
    }
    else if (current_token->token_type == TOKEN_T_NUM || 
             current_token->token_type == TOKEN_T_STRING ||
             current_token->token_type == TOKEN_T_IDENTIFIER || 
             current_token->token_type == TOKEN_T_GLOBAL_VAR ||
             current_token->token_type == TOKEN_T_KEYWORD ) {
        tree_node_t *node;
        tree_init(&node);
        node->token = current_token;
        node->type = NODE_T_TERMINAL;

        return node;
    }
    else if(strcmp(current_token->token_lexeme, "-") == 0) {

        tree_node_t *unary_node;
        tree_init(&unary_node);
        unary_node->token = current_token; 

        tree_node_t *expr_node;
        tree_init(&expr_node);
        expr_node->token = get_next_token(lexer);

        tree_insert_child(unary_node, expr_node);

        return unary_node;
    }
    else {
        syntactic->error = ERR_T_SYNTAX_ERR;
        return NULL;
    }

    return root;
}


tree_node_t *rule_predicate(Syntactic *syntactic, Lexer *lexer){
    tree_node_t *n = rule_expression(syntactic, lexer);
    return n;

}