#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "tree.h"
#include "symtable.h"

typedef struct semantic {
    int error;
    int scope_counter;
    Symtable *symtable;
} Semantic;

typedef enum{
    TYPE_NUM,
    TYPE_STRING, 
    TYPE_NULL,
    TYPE_ERROR,
    TYPE_KEYWORD,
    TYPE_UNKNOWN //for unknown variables 
} EXPR_TYPE;

Semantic *init_semantic(Symtable *symtable);
int traverse_tree(tree_node_t *tree_node, Symtable *symtable, Semantic *semantic);
void handle_rule(tree_node_t *tree_node);
Symbol *check_if_identif_is_parameter(Symbol *symbol, Semantic *semantic);
EXPR_TYPE infer_expression_type(tree_node_t *node, Symtable *symtable);
bool has_relational_operator(tree_node_t *node);
bool multiple_declaration_valid(Symbol *symbol);
int check_main_function(Symtable *symtable);


#endif