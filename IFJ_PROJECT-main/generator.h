#ifndef GENERATOR_H
#define GENERATOR_H

#include "symbol.h"
#include "symtable.h"
#include "token.h"
#include "tree.h"
#include <stdio.h>

typedef struct generator {
  Symtable *symtable; // Symbolová tabuľka - na vyhľadávanie premenných/funkcií
  int label_counter;  // Počítadlo labelov (LABEL_0, LABEL_1, ...)
  int temp_var_counter;   // Počítadlo dočasných premenných
  int current_scope;      // Aktuálny scope
  char *current_function; // Názov aktuálnej funkcie
  FILE *output;           // Výstupný súbor (NULL = stdout)
  int error;              // Chybový kód
  int in_function;        // Flag či sme vnútri funkcie
  bool is_global;
  bool is_called;
  char**global_vars;
  int global_count;
  bool is_float;
  bool has_return;  // Flag to track if return statement was generated
  bool tf_created;
  int in_while_loop;  // Flag to track if inside while loop body
  char **function_params;  // Array of parameter names for current function
  int function_param_count;  // Number of parameters in current function	
} Generator;

// Inicializácia a základné funkcie
Generator *init_generator(Symtable *symtable);
int generator_start(Generator *generator, tree_node_t *tree);
void generator_generate(Generator *generator, tree_node_t *node);
void generator_free(Generator *generator);

// Pomocné funkcie
char *get_next_label(Generator *generator);
char *get_temp_var(Generator *generator);
void generator_emit(Generator *generator, const char *format, ...);

// Generovanie pre jednotlivé typy uzlov
void generate_code_block(Generator *generator, tree_node_t *node);
void generate_sequence(Generator *generator, tree_node_t *node);
void generate_instruction(Generator *generator, tree_node_t *node);
void generate_expression(Generator *generator, tree_node_t *node);
void generate_assignment(Generator *generator, tree_node_t *node);
void generate_declaration(Generator *generator, tree_node_t *node);
void generate_function_declaration(Generator *generator, tree_node_t *node);
void generate_function_call(Generator *generator, tree_node_t *node);
void generate_if(Generator *generator, tree_node_t *node);
void generate_while(Generator *generator, tree_node_t *node);
void generate_return(Generator *generator, tree_node_t *node);
void generate_terminal(Generator *generator, tree_node_t *node);

char **generate_global_vars(Generator *generator);

void generate_strcmp_comparison(Generator *generator, tree_node_t *node);
int diverse_function(Generator *generator, tree_node_t *node);

#endif
