#ifndef SYNCTACTIC_H
#define SYNCTACTIC_H

#include "lexer.h"
#include "symtable.h"
#include "tree.h"

typedef struct syntactic {
    int error;
    tree_node_t *tree;
    Symtable *symtable;
    int scope_counter;

    int fn_number_of_params;
} Syntactic;

Syntactic *init_syntactic(Symtable *symtable);

int syntactic_start(Syntactic *syntactic, Lexer *lexer);

int assert_expected_literal(Syntactic *syntactic, Token *token, char *literal);

int rule_check_skeleton_1(Syntactic *syntactic, Lexer *lexer); // k1
int rule_check_skeleton_2(Syntactic *syntactic, Lexer *lexer); // k2

int rule_code_block(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_sequence(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_sequence_prime(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_instruction(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);

int rule_return(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_if(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_while(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_declaration(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);

tree_node_t *rule_expression(Syntactic *syntactic, Lexer *lexer);
tree_node_t *rule_expression_1(tree_node_t *node, int min_precedence, Syntactic *syntactic, Lexer *lexer);

tree_node_t *rule_parse_primary(Syntactic *syntactic, Lexer *lexer);
tree_node_t *rule_predicate(Syntactic *syntactic, Lexer *lexer);

int rule_allowed_eol(Syntactic *syntactic, Lexer *lexer);
int rule_assignment(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_assignment(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_function_declaration(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_function_call(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_getter_declaration(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);
int rule_setter_declaration(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);

int rule_expression_or_function(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);

int rule_function_parameters(Syntactic *syntactic, Lexer *lexer, tree_node_t *node, bool is_declaration);
int rule_function_parameters_prime(Syntactic *syntactic, Lexer *lexer, tree_node_t *node, bool is_declaration);

int rule_function_declaration_begin(Syntactic *syntactic, Lexer *lexer, tree_node_t *node);

void add_prefix(Token *token, char *s);
void add_prefix_to_symbol(Symbol *symbol, char *prefix);

#endif