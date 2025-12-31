#include "symbol.h"
#include "symtable.h"
#include "tree.h"
#include "syntactic.h"
#include <stdio.h>
#include <string.h>
#include "semantic.h"
#include <stdlib.h>

Semantic *init_semantic(Symtable *symtable){
    Semantic *semantic = malloc(sizeof(Semantic));
    semantic->error = 0;
    semantic->scope_counter = 0;
    semantic->symtable = symtable;
    return semantic;
}

bool has_relational_operator(tree_node_t *node) {
    if (!node || !node->token) return false;
    
    if (node->token->token_lexeme) {
        char *op = node->token->token_lexeme;
        if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
            strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0 ||
            strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
            strcmp(op, "is") == 0) {
            return true;
        }
    }
    
    for (int i = 0; i < node->children_count; i++) {
        if (has_relational_operator(node->children[i])) {
            return true;
        }
    }
    
    return false;
}

EXPR_TYPE infer_expression_type(tree_node_t *node, Symtable *symtable){
    if (!node) return TYPE_ERROR;
    if (node->rule == GR_FUN_CALL){
        return TYPE_UNKNOWN;
    }

    if(node->type == NODE_T_TERMINAL){
        if (node->token->token_type == TOKEN_T_NUM){
            return TYPE_NUM;
        }
        if (node->token->token_type == TOKEN_T_STRING){
            return TYPE_STRING;
        }
        if (strcmp(node->token->token_lexeme, "null") == 0){
            return TYPE_NULL;
        }
        if (node->token->token_type == TOKEN_T_IDENTIFIER ||
            node->token->token_type == TOKEN_T_GLOBAL_VAR){
            
            return TYPE_UNKNOWN;
        }
        if (strcmp(node->token->token_lexeme, "Num") == 0 ||
            strcmp(node->token->token_lexeme, "String") == 0 ||
            strcmp(node->token->token_lexeme, "Null") == 0) {
            return TYPE_KEYWORD; // Special type for 'is' operator
        }

    }

    char *op = node->token->token_lexeme;

    if(node->children_count == 2){
        EXPR_TYPE left = infer_expression_type(node->children[0], symtable);
        EXPR_TYPE right = infer_expression_type(node->children[1], symtable);

        if(left == TYPE_ERROR || right == TYPE_ERROR){
            return TYPE_ERROR;
        }

        if(strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
            strcmp(op, "*") == 0 || strcmp(op, "/") == 0){
            
            if (strcmp(op, "+") == 0 && left == TYPE_STRING && right == TYPE_STRING) {
                return TYPE_STRING;
            }
            
            // Special case: String * Num (iteration)
            if (strcmp(op, "*") == 0 && left == TYPE_STRING && right == TYPE_NUM) {
                return TYPE_STRING;
            }
            
            // Standard arithmetic: both must be Num
            if (left == TYPE_NUM && right == TYPE_NUM) {
                return TYPE_NUM;
            }
            
            // If either is UNKNOWN, can't determine statically
            if (left == TYPE_UNKNOWN || right == TYPE_UNKNOWN) {
                return TYPE_UNKNOWN;
            }
            
            // Type mismatch - error 6
            if (left != TYPE_UNKNOWN && right != TYPE_UNKNOWN) {
                return TYPE_ERROR; // Set semantic->error = 6 in caller
            }
        }

        // Relational: <, >, <=, >=
        if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
            strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) {
            
            if (left == TYPE_NUM && right == TYPE_NUM) {
                return TYPE_NUM; // In basic variant, returns truthiness
            }
            
            if (left == TYPE_UNKNOWN || right == TYPE_UNKNOWN) {
                return TYPE_UNKNOWN;
            }
            
            return TYPE_ERROR; // Type mismatch
        }
        
        // Equality: ==, !=
        if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
            // These work with any types
            return TYPE_NUM; // Returns truthiness
        }
        
        // Type test: is
        if (strcmp(op, "is") == 0) {
            // Right side should be a type keyword (checked elsewhere)
            return TYPE_NUM; // Returns truthiness
        }
    }
    
    // Unary minus
    if (node->children_count == 1 && strcmp(op, "-") == 0) {
        EXPR_TYPE operand = infer_expression_type(node->children[0], symtable);
        if (operand == TYPE_NUM) return TYPE_NUM;
        if (operand == TYPE_UNKNOWN) return TYPE_UNKNOWN;
        return TYPE_ERROR;
    }
    
    return TYPE_ERROR;
}

int check_builtin_function(tree_node_t *node, Semantic *semantic){
    // only check if this is a function call node
    if(node->rule != GR_FUN_CALL || node->children_count == 0){
        return -1; // not a function call
    }
    
    // get the function name from the first child (should be the identifier)
    if(!node->children[0]->token || !node->children[0]->token->token_lexeme){
        return -1;
    }
    
    char *func_name = node->children[0]->token->token_lexeme;
    
    // get parameter count (second child should be gr_fun_param if it exists)
    int param_count = 0;
    if(node->children_count >= 2 && node->children[1]->rule == GR_FUN_PARAM){
        param_count = node->children[1]->children_count;
    }
    
    // check each builtin function
    if(strcmp(func_name, "write") == 0){
        if(param_count != 1){
            semantic->error = 5;
            return 5;
        }

        return 0;
    }
    else if(strcmp(func_name, "read_str") == 0){
        if(param_count != 0){
            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    else if(strcmp(func_name, "read_num") == 0){
        if(param_count != 0){
            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    else if(strcmp(func_name, "floor") == 0){
        if(param_count != 1){
            semantic->error = 5;
            return 5;
        }
        if(node->children[1]->children[0]->token->token_type != TOKEN_T_NUM &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_GLOBAL_VAR){
            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    else if(strcmp(func_name, "str") == 0){
        if(param_count != 1){
            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    else if(strcmp(func_name, "length") == 0){
        if(param_count != 1){
            semantic->error = 5;
            return 5;
        }
        if(node->children[1]->children[0]->token->token_type != TOKEN_T_STRING &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_GLOBAL_VAR){
            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    else if(strcmp(func_name, "substring") == 0){
        if(param_count != 3){
            semantic->error = 5;
            return 5;
        }
        if((node->children[1]->children[0]->token->token_type != TOKEN_T_STRING  &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_GLOBAL_VAR) ||

           (node->children[1]->children[1]->token->token_type != TOKEN_T_NUM  &&
            node->children[1]->children[1]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[1]->token->token_type != TOKEN_T_GLOBAL_VAR) ||

           (node->children[1]->children[2]->token->token_type != TOKEN_T_NUM &&
            node->children[1]->children[2]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[2]->token->token_type != TOKEN_T_GLOBAL_VAR)){

            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    else if(strcmp(func_name, "strcmp") == 0){
        if(param_count != 2){
            semantic->error = 5;
            return 5;
        }
        if((node->children[1]->children[0]->token->token_type != TOKEN_T_STRING &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_GLOBAL_VAR) ||
           (node->children[1]->children[1]->token->token_type != TOKEN_T_STRING &&
            node->children[1]->children[1]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[1]->token->token_type != TOKEN_T_GLOBAL_VAR)){
            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    else if(strcmp(func_name, "ord") == 0){
        if(param_count != 2){
            semantic->error = 5;
            return 5;
        }
        if((node->children[1]->children[0]->token->token_type != TOKEN_T_STRING &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_GLOBAL_VAR )||
           (node->children[1]->children[1]->token->token_type != TOKEN_T_NUM &&
            node->children[1]->children[1]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[1]->token->token_type != TOKEN_T_GLOBAL_VAR)){
            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    else if(strcmp(func_name, "chr") == 0){
        if(param_count != 1){
            semantic->error = 5;
            return 5;
        }
        if(node->children[1]->children[0]->token->token_type != TOKEN_T_NUM &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_IDENTIFIER &&
            node->children[1]->children[0]->token->token_type != TOKEN_T_GLOBAL_VAR){
            semantic->error = 5;
            return 5;
        }
        return 0;
    }
    
    return -1; // Not a builtin function
}

int traverse_tree(tree_node_t *tree_node, Symtable *symtable, Semantic *semantic) {
    if(tree_node->rule == GR_CODE_BLOCK){
        semantic->scope_counter++;
    }

    int builtin_result = check_builtin_function(tree_node, semantic);
    if(builtin_result == 0){
        // It's a valid builtin function, continue to children but don't check symtable

        return semantic->error;
    }
    else if(builtin_result > 0){
        // It's a builtin function but with wrong parameters
        return semantic->error;
    }
    if(tree_node->rule == GR_CODE_BLOCK){
        semantic->scope_counter--;
    }
    // scope safe zone - dobre rata scope odtialto

    Symbol *symbol = search_table(tree_node->token, symtable);
    if(symbol) {
        // terminal node
        if(tree_node->type == NODE_T_TERMINAL &&
                   tree_node->token &&
                   tree_node->token->token_type == TOKEN_T_IDENTIFIER) {

            char *name = tree_node->token->token_lexeme;
            // Check if it's a builtin function identifier
            if(strcmp(name, "write") == 0 ||
               strcmp(name, "read_str") == 0 ||
               strcmp(name, "read_num") == 0 ||
               strcmp(name, "floor") == 0 ||
               strcmp(name, "str") == 0 ||
               strcmp(name, "length") == 0 ||
               strcmp(name, "substring") == 0 ||
               strcmp(name, "strcmp") == 0 ||
               strcmp(name, "ord") == 0 ||
               strcmp(name, "chr") == 0) {

                return 0;
            }

            // if identifier, check declaration
            if((symbol->sym_identif_type == IDENTIF_T_VARIABLE || symbol->sym_identif_type == IDENTIF_T_UNSET) && tree_node->parent->rule != GR_DECLARATION
                && !symbol->is_global){

                if(tree_node->parent != NULL){
                    if (tree_node->parent->parent != NULL){
                        if (tree_node->parent->parent->rule == GR_FUN_DECLARATION){
                            return semantic->error;
                        }
                    }
                }

                Token *token = create_token(TOKEN_T_IDENTIFIER, symbol->sym_lexeme,symbol->sym_lexeme_length,
            0,0,semantic->scope_counter+100, symbol->token->previous_scope_arr, symbol->token->scope_count);

                add_prefix(token, "setter+");
                if (search_table_for_setter_or_getter(token, symtable) != NULL) {
                    return 0;
                }

                token = create_token(TOKEN_T_IDENTIFIER, symbol->sym_lexeme,symbol->sym_lexeme_length,
            0,0,semantic->scope_counter+100, symbol->token->previous_scope_arr, symbol->token->scope_count);
                add_prefix(token, "getter+");
                Symbol *getter_sym = search_table_for_setter_or_getter(token, symtable);
                if (getter_sym != NULL) {
                    if (getter_sym->sym_identif_declaration_count)
                    return 0;
                }

                if(symbol->sym_identif_declaration_count == 0){
                    semantic->error = 3;
                    return semantic->error;
                }else {
                    semantic->error = 0;
                    return semantic->error;
                }
            }

        }
        tree_node_t *parent = tree_node->parent;
        while(parent != NULL){
            if(parent->rule == GR_FUN_DECLARATION){
                break;
            }
            parent = parent->parent;
        }


        if (symbol->sym_identif_declaration_count > 1 && symbol->sym_identif_type == IDENTIF_T_FUNCTION) {
            int cnt = 0;
            for (int i = 0; i < symbol->sym_identif_declaration_count; i++) {
                for (int j = i + 1; j < symbol->sym_identif_declaration_count; j++) {
                    if (symbol->sym_function_number_of_params[i] ==
                        symbol->sym_function_number_of_params[j]) {
                        
                        cnt++;
                    }
                }
            }
            if(cnt > 1){
                semantic->error = 4;
                return semantic->error;
            }
        }else if(symbol->sym_identif_declaration_count > 1 && !symbol->is_parameter){
            if(!multiple_declaration_valid(symbol)){
                semantic->error = 4;
                return semantic->error;
            }
        }
    } else { // nonterminal node
        if(tree_node->rule == GR_FUN_CALL && tree_node->children_count == 1){
            symbol = search_table(tree_node->children[0]->token,symtable);
            if(symbol == NULL){
                return 0;
            }

            bool found_param_count_match = false;
            for(int i = 0;i<symbol->sym_identif_declaration_count;i++){
                if(symbol->sym_function_number_of_params[i] == 0){
                    found_param_count_match = true;
                    break;
                }
            }

            if(!found_param_count_match){
                semantic->error = 5;
                return semantic->error;
            }
        }
        else if(tree_node->rule == GR_FUN_PARAM && tree_node->parent->rule == GR_FUN_CALL ){
            Symbol *symbol = search_table(tree_node->parent->children[0]->token, symtable);
            bool found_param_count_match = false;
            for(int i = 0;i<symbol->sym_identif_declaration_count;i++){
                if(tree_node->children_count == symbol->sym_function_number_of_params[i]){
                    found_param_count_match = true;
                    break;
                }
            }
            if(!found_param_count_match){
                semantic->error = 5;
                return semantic->error;
            }
        }
    }

    if (tree_node->parent &&
        (tree_node->parent->rule == GR_ASSIGNMENT ||
         tree_node->parent->rule == GR_RETURN)
         && tree_node->parent->children[0] != tree_node) {

        EXPR_TYPE type = infer_expression_type(tree_node, symtable);
        if (type == TYPE_ERROR || type == TYPE_NULL) {
            semantic->error = 6;
            return semantic->error;
        }
    }
    if(tree_node->parent && tree_node->parent->rule == GR_PREDICATE_PARENTH){

        EXPR_TYPE type = infer_expression_type(tree_node, symtable);
        if(type == TYPE_ERROR || type == TYPE_NULL){
            semantic->error = 6;
            return semantic->error;
        }
    }

    for (int i = 0; i < tree_node->children_count; i++) {
        traverse_tree(tree_node->children[i], symtable, semantic);
        if(semantic->error != 0){
            return semantic->error;
        }
    }

    if(tree_node->rule == GR_CODE_BLOCK){
        semantic->scope_counter--;
    }
    return 0;
}


Symbol *check_if_identif_is_parameter(Symbol *symbol, Semantic *semantic) {
    Symtable *symtable = semantic->symtable;
    for (int i = 0; i < symtable->symtable_size; i++) {
        Symbol *sym = symtable->symtable_rows[i].symbol;
        if (!sym || !sym->sym_lexeme || sym->sym_identif_declared_at_scope_arr == NULL) continue;

        if (strcmp(sym->sym_lexeme, symbol->sym_lexeme) == 0 &&
            symbol->sym_identif_used_at_scope_arr[0] == sym->sym_identif_declared_at_scope_arr[0]) {
            
            return sym;
        }
    }

    return NULL;

}

bool multiple_declaration_valid(Symbol *symbol){
    for(int i = 0; i < symbol->sym_identif_declaration_count; i++){
        for(int j = i + 1; j < symbol->sym_identif_declaration_count; j++){
            // If two declarations are in the same scope, it's invalid
            if(symbol->sym_identif_declared_at_scope_arr[i] == 
               symbol->sym_identif_declared_at_scope_arr[j]){
                return false;
            }
        }
    }
    return true;
}

int check_main_function(Symtable *symtable) {
    // Search through the entire symbol table for 'main'
    for (int i = 0; i < symtable->symtable_size; i++) {
        Symbol *sym = symtable->symtable_rows[i].symbol;
        if (!sym || !sym->sym_lexeme) continue;

        // Check if this is 'main'
        if (strcmp(sym->sym_lexeme, "main") == 0) {
            if (sym->sym_identif_type != IDENTIF_T_FUNCTION) {
                continue; 
            }

            // Check if any declaration has 0 parameters
            for (int j = 0; j < sym->sym_identif_declaration_count; j++) {
                if (sym->sym_function_number_of_params[j] == 0) {
                    return 0; // Found main() with 0 parameters
                }
            }
            
            return 3; 
        }
    }

    return 3; 
}