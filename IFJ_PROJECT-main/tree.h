#ifndef IAL_BTREE_H
#define IAL_BTREE_H

#include <stdbool.h>
#include "token.h"
#include "symbol.h"
#include "symtable.h"

/**
 * @brief Enumeration of possible data types for node content.
 */
typedef enum tree_node_content_type_t { 
    NODE_T_TERMINAL, 
    NODE_T_NONTERMINAL
} tree_node_type_t;

typedef enum nonterminal_types  { 
    NONTERMINAL_T_EXPRESSION,
    NONTERMINAL_T_PREDICATE,
    NONTERMINAL_T_SEQUENCE,
    NONTERMINAL_T_INSTRUCTION,
    NONTERMINAL_T_CODE_BLOCK,
    NONTERMINAL_T_ASSIGNMENT,
    NONTERMINAL_T_DECLARATION,
    NONTERMINAL_T_LOC_VAR_DEC,
    NONTERMINAL_T_FUN_PARAM,
    NONTERMINAL_T_FUN_CALL,
    NONTERMINAL_T_ALLOWED_EOL,
    NONTERMINAL_T_TERM,
    NONTERMINAL_T_EXPRESSION_INDETIF,
    NONTERMINAL_T_IF,
    NONTERMINAL_T_RETURN,
    NONTERMINAL_T_WHILE,
    NONTERMINAL_T_EXPRESSION_OR_FN,
    NT_NONE
} nonterminal_types;

typedef enum rules {
    K1,
    K2,
    GR_SEQUENCE,
    GR_SEQUENCE_2,
    GR_ALLOWED_EOL,
    GR_INSTRUCTION_RETURN,
    GR_INSTRUCTION_IF,
    GR_INSTRUCTION_WHILE,
    GR_INSTRUCTION_DECLARATION,
    GR_INSTRUCTION_EXPRESSION,
    GR_CODE_BLOCK,
    GR_EXPRESSION,
    GR_EXPRESSION_OPERATOR,
    GR_EXPRESSION_IDENTIF,
    GR_TERM,
    GR_PREDICATE_RELATOR,
    GR_PREDICATE_PARENTH,
    GR_ASSIGNMENT,
    GR_SETTER_CALL,
    GR_DECLARATION,
    GR_FUN_DECLARATION,
    GR_FUN_PARAM,
    GR_FUN_PARAM_COMMA,
    GR_FUN_CALL,
    GR_FUN_CALL_BUILTIN,
    GR_GETTER_DECLARATION,
    GR_SETTER_DECLARATION,
    GR_IF,
    GR_RETURN,
    GR_WHILE,
    GR_EXPRESSION_OR_FN,
    GR_NONE
} grammar_rules;

/**
 * @struct tree_node
 * @brief Node of a general tree (multiple children).
 */
typedef struct tree_node {
    // General
    int symbol_symtable_index;
    struct tree_node *parent;
    struct tree_node **children;    
    int children_count;            
    int children_capacity;        

    grammar_rules rule;
    tree_node_type_t type;

    // Terminal
    Symbol *symbol;
    Token *token;

    // Nonterminal
    nonterminal_types nonterm_type;
} tree_node_t;



tree_node_t *tree_create_nonterminal(nonterminal_types nonterminal_type, grammar_rules rule);
tree_node_t *tree_create_terminal(Token *token);
char *grammar_rule_to_string(grammar_rules rule);

/**
 * @brief Initialize a tree to empty.
 * @param tree Double pointer to the root node.
 */
void tree_init(tree_node_t **tree);

/**
 * @brief Insert a key with associated content into the tree.
 * If the key exists, update its content; otherwise append a new child.
 * @param tree Pointer to pointer to the root of the tree.
 * @param value Content to store in the node.
 * @return True on successful insertion/update; false on memory allocation failure.
 */
bool tree_insert(tree_node_t **tree, tree_node_t *node);

/**
 * @brief Search for a node by key in the tree.
 * Performs a depth-first search of all children.
 * @param tree Pointer to the root node of the tree.
 * @param key Key to search for.
 * @param value Output pointer to the node's content (if found).
 * @return True if key is found; false otherwise.
 */
bool tree_search(tree_node_t *tree, int index, tree_node_t **result);

/**
 * @brief Delete a node (and its entire subtree) with the given key.
 * Frees all associated memory and shifts remaining children.
 * @param tree Pointer to pointer to the root of the tree.
 * @param key Key of the node to delete.
 */
void tree_delete(tree_node_t **tree, int index);

/**
 * @brief Dispose of the entire tree, freeing all memory.
 * @param tree Pointer to pointer to the root of the tree.
 */
void tree_dispose(tree_node_t **tree);

/**
 * @brief Print the entire tree structure in a readable indented format.
 * @param tree Pointer to the root of the tree.
 * @param depth Current depth level (use 0 when calling from outside).
 */
void tree_print_tree(const tree_node_t *node, const char *prefix, int is_last);

/**
 * @brief Add a child node to a given parent node.
 *
 * Dynamically resizes the parent's children array if necessary.
 * If the parent or child is NULL, the function returns false.
 *
 * @param parent Pointer to the parent node.
 * @param child Pointer to the child node to add.
 * @return True if the child was successfully added; false otherwise.
 */
bool tree_insert_child(tree_node_t *parent, tree_node_t *child);
void tree_print_node(tree_node_t *node);
void tree_print_node_with_children(tree_node_t *node);


#endif