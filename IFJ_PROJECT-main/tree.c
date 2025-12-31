#include "tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "symbol.h"


tree_node_t *tree_create_nonterminal(nonterminal_types nonterminal_type, grammar_rules rule) {
    tree_node_t *node;
    tree_init(&node);

    node->type = NODE_T_NONTERMINAL;
    node->nonterm_type = nonterminal_type;
    node->rule = rule;
    node->children_count = 0;

    node->token = malloc(sizeof(Token));
    if (!node->token) return NULL;

    const char *rule_str = grammar_rule_to_string(rule);
    node->token->token_lexeme = malloc(strlen(rule_str) + 1);
    if (!node->token->token_lexeme) {
        free(node->token);
        return NULL;
    }
    strcpy(node->token->token_lexeme, rule_str);

    return node;
}

tree_node_t *tree_create_terminal(Token *token){
    tree_node_t *node;
    tree_init(&node);
    node->type = NODE_T_TERMINAL;

    node->token = token;    
    node->children_count = 0;

    if(strcmp(token->token_lexeme, "\n") == 0){
        strcpy(node->token->token_lexeme, "n");
    }

    return node;
}

char *grammar_rule_to_string(grammar_rules rule) {
    switch (rule) {
        case K1:                             return "K1";
        case K2:                             return "K2";
        case GR_SEQUENCE:                    return "GR_SEQUENCE";
        case GR_SEQUENCE_2:                  return "GR_SEQUENCE_2";
        case GR_ALLOWED_EOL:                 return "GR_ALLOWED_EOL";
        case GR_INSTRUCTION_RETURN:          return "GR_INSTRUCTION_RETURN";
        case GR_INSTRUCTION_IF:              return "GR_INSTRUCTION_IF";
        case GR_INSTRUCTION_WHILE:           return "GR_INSTRUCTION_WHILE";
        case GR_INSTRUCTION_DECLARATION:     return "GR_INSTRUCTION_DECLARATION";
        case GR_INSTRUCTION_EXPRESSION:      return "GR_INSTRUCTION_EXPRESSION";
        case GR_CODE_BLOCK:                  return "GR_CODE_BLOCK";
        case GR_EXPRESSION:                  return "GR_EXPRESSION";
        case GR_EXPRESSION_OPERATOR:         return "GR_EXPRESSION_OPERATOR";
        case GR_EXPRESSION_IDENTIF:          return "GR_EXPRESSION_IDENTIF";
        case GR_TERM:                        return "GR_TERM";
        case GR_PREDICATE_RELATOR:           return "GR_PREDICATE_RELATOR";
        case GR_PREDICATE_PARENTH:           return "GR_PREDICATE_PARENTH";
        case GR_ASSIGNMENT:                  return "GR_ASSIGNMENT";
        case GR_SETTER_CALL:                 return "GR_SETTER_CALL";
        case GR_DECLARATION:                 return "GR_DECLARATION";
        case GR_FUN_DECLARATION:             return "GR_FUN_DECLARATION";
        case GR_FUN_PARAM:                   return "GR_FUN_PARAM";
        case GR_FUN_PARAM_COMMA:             return "GR_FUN_PARAM_COMMA";
        case GR_FUN_CALL:                    return "GR_FUN_CALL";
        case GR_FUN_CALL_BUILTIN:            return "GR_FUN_CALL_BUILTIN";
        case GR_GETTER_DECLARATION:          return "GR_GETTER_DECLARATION";
        case GR_SETTER_DECLARATION:          return "GR_SETTER_DECLARATION";
        case GR_IF:                          return "GR_IF";
        case GR_RETURN:                      return "GR_RETURN";
        case GR_WHILE:                       return "GR_WHILE";
        case GR_EXPRESSION_OR_FN:            return "GR_EXPRESSION_OR_FN";
        case GR_NONE:                        return "GR_NONE";
        default:                             return "<UNKNOWN_RULE>";
    }
}

/**
 * @brief Initialize a tree to empty (NULL).
 */
void tree_init(tree_node_t **tree) {
    *tree = (tree_node_t *)malloc(sizeof(tree_node_t));
    (*tree)->symbol_symtable_index = 0;
    (*tree)->parent = NULL;
    (*tree)->children = NULL;
    (*tree)->children_count = 0;
    (*tree)->children_capacity = 0;
    (*tree)->rule = GR_NONE;
    (*tree)->symbol = NULL;
    (*tree)->token = NULL;
}

/**
 * @brief Search for a node with a given symbol index in the tree.
 *        Performs a depth-first search over all children.
 * @param tree Pointer to the current node (root of subtree).
 * @param index Symbol table index to search for.
 * @param result Output pointer to the found node (if any).
 * @return True if a node with the given index is found; false otherwise.
 */
bool tree_search(tree_node_t *tree, int index, tree_node_t **result) {
    if (tree == NULL) {
        return false;
    }

    if (tree->symbol_symtable_index == index) {
        *result = tree;
        return true;
    }

    for (int i = 0; i < tree->children_count; i++) {
        if (tree_search(tree->children[i], index, result)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Add a child node to a given parent node.
 *
 * Dynamically grows the parent's children array if it reaches capacity.
 * Also sets the child's parent pointer.
 * @param parent Pointer to the parent node.
 * @param child Pointer to the child node to add.
 * @return True if successful; false otherwise.
 */
bool tree_insert_child(tree_node_t *parent, tree_node_t *child) {
    if (parent == NULL || child == NULL) {
        return false;
    }

    // Set parent pointer of child
    child->parent = parent;

    // Initialize children array if empty
    if (parent->children_capacity == 0) {
        parent->children_capacity = 1;
        parent->children = (tree_node_t **)malloc(sizeof(tree_node_t *) * parent->children_capacity);
        if (parent->children == NULL) {
            parent->children_capacity = 0;
            return false;
        }
    }

    // Resize array if full
    if (parent->children_count >= parent->children_capacity) {
        int new_capacity = parent->children_capacity * 2;
        tree_node_t **tmp = (tree_node_t **)realloc(parent->children, sizeof(tree_node_t *) * new_capacity);
        if (tmp == NULL) {
            return false;
        }
        parent->children = tmp;
        parent->children_capacity = new_capacity;
    }

    // Add the child node
    parent->children[parent->children_count++] = child;
    return true;
}

/**
 * @brief Dispose (free) the entire tree from this node downward.
 *        Frees all descendants and the node itself.
 */
void tree_dispose(tree_node_t **tree) {
    if (*tree == NULL) {
        return;
    }

    // Recursively free children first
    for (int i = 0; i < (*tree)->children_count; i++) {
        tree_dispose(&((*tree)->children[i]));
    }

    // Free children array and node itself
    free((*tree)->children);
    free(*tree);
    *tree = NULL;
}

/**
 * @brief Delete the node (and its subtree) with the given symbol index.
 *        Frees memory and shifts remaining siblings to fill the gap.
 * @param tree Double pointer to the root of the tree (or subtree).
 * @param index Symbol table index of the node to delete.
 */
void tree_delete(tree_node_t **tree, int index) {
    if (*tree == NULL) {
        return;
    }

    // If the root node itself matches, dispose it entirely
    if ((*tree)->symbol_symtable_index == index) {
        tree_dispose(tree);
        return;
    }

    tree_node_t *node = *tree;
    for (int i = 0; i < node->children_count; i++) {
        // If a direct child matches, delete it
        if (node->children[i] != NULL && node->children[i]->symbol_symtable_index == index) {
            tree_dispose(&(node->children[i]));
            // Shift remaining children left
            for (int j = i; j < node->children_count - 1; j++) {
                node->children[j] = node->children[j+1];
            }
            node->children_count--;
            return;
        }
        // Otherwise recurse into the child
        tree_delete(&(node->children[i]), index);
        // If the child pointer became NULL after deletion, remove its entry
        if (node->children[i] == NULL) {
            for (int j = i; j < node->children_count - 1; j++) {
                node->children[j] = node->children[j+1];
            }
            node->children_count--;
            return;
        }
    }
}

/**
 * @brief Recursively print the entire tree structure with indentation.
 *
 * Prints either terminal or nonterminal information.
 * @param tree Pointer to the current node of the tree.
 * @param depth Current recursion depth; use 0 when calling from main.
 */

void tree_print_tree(const tree_node_t *node, const char *prefix, int is_last) {
    if (!node) return;

    // Print the current node
    printf("%s", prefix);
    printf(is_last ? "└── " : "├── ");
    if (node->token && node->token->token_lexeme)
        printf("%s\n", node->token->token_lexeme);
    else
        printf("(null)\n");

    // Prepare prefix for children
    char new_prefix[256];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");

    // Print children
    for (int i = 0; i < node->children_count; i++) {
        int last = (i == node->children_count - 1);
        tree_print_tree(node->children[i], new_prefix, last);
    }
}

void tree_print_node_with_children(tree_node_t *node) {
    if (!node) {
        printf("Node: NULL\n");
        return;
    }

    printf("╔════════════════════════════════════════\n");
    printf("║ Current Node:\n");
    printf("╠════════════════════════════════════════\n");
    printf("║ Type: %s\n", node->type == NODE_T_TERMINAL ? "TERMINAL" : "NONTERMINAL");
    printf("║ Rule: %s\n", grammar_rule_to_string(node->rule));
    
    if (node->token && node->token->token_lexeme) {
        printf("║ Token: '%s'\n", node->token->token_lexeme);
    }
    
    if (node->symbol && node->symbol->sym_lexeme) {
        printf("║ Symbol: '%s'\n", node->symbol->sym_lexeme);
    }
    
    printf("╠════════════════════════════════════════\n");
    printf("║ Children (%d):\n", node->children_count);
    
    for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        printf("║   [%d] ", i);
        
        if (!child) {
            printf("NULL\n");
            continue;
        }
        
        printf("%s - ", grammar_rule_to_string(child->rule));
        
        if (child->token && child->token->token_lexeme) {
            printf("'%s'", child->token->token_lexeme);
        } else {
            printf("(no token)");
        }
        printf("\n");
    }
    
    printf("╚════════════════════════════════════════\n\n");
}

void tree_print_node(tree_node_t *node) {
    if (!node) {
        printf("Node: NULL\n");
        return;
    }

    printf("========================================\n");
    printf("Node Information:\n");
    printf("========================================\n");
    
    // Node type
    printf("Node Type: ");
    if (node->type == NODE_T_TERMINAL) {
        printf("TERMINAL\n");
    } else if (node->type == NODE_T_NONTERMINAL) {
        printf("NONTERMINAL\n");
        printf("Nonterminal Type: %d\n", node->nonterm_type);
    }
    
    // Grammar rule
    printf("Grammar Rule: %s (%d)\n", grammar_rule_to_string(node->rule), node->rule);
    
    // Symbol table index
    printf("Symbol Table Index: %d\n", node->symbol_symtable_index);
    
    // Children info
    printf("Children Count: %d\n", node->children_count);
    printf("Children Capacity: %d\n", node->children_capacity);
    
    // Parent info
    if (node->parent) {
        printf("Has Parent: YES (rule: %s)\n", 
               grammar_rule_to_string(node->parent->rule));
    } else {
        printf("Has Parent: NO (root node)\n");
    }
    
    // Token information
    printf("\n--- Token Information ---\n");
    if (node->token) {
        printf("Token exists: YES\n");
        print_token(node->token);
    } else {
        printf("Token exists: NO\n");
    }
    
    // Symbol information
    printf("\n--- Symbol Information ---\n");
    if (node->symbol) {
        printf("Symbol exists: YES\n");
        print_symbol(node->symbol);
    } else {
        printf("Symbol exists: NO\n");
    }
    
    printf("========================================\n\n");
}