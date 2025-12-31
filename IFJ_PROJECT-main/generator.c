#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generator.h"
#include "symbol.h"
#include "symtable.h"
#include "token.h"
#include "tree.h"
#include "utils.h"

// Initialize generator
Generator *init_generator(Symtable *symtable) {
  Generator *gen = malloc(sizeof(Generator));
  if (!gen) {
    return NULL;
  }

  gen->symtable = symtable;         // Symtable instance
  gen->label_counter = 0;           // Label counter
  gen->temp_var_counter = 0;        // Temporary variable counter
  gen->current_scope = 0;           // Current scope
  gen->current_function = NULL;     // Current function
  gen->output = NULL;               // Output file if NULL, stdout is used
  gen->error = 0;                   // Error code
  gen->in_function = 0;             // Flag to check if we are in a function
  gen->is_global = false;           // Flag to check if the variable is global
  gen->is_called = false;           // Flag to check if the function, getter or setter is called
  gen->is_float = false;            // Flag to check if the variable is float
  gen->has_return = false;          // Flag to check if the function has a return statement
  gen->in_while_loop = 0;           // Flag to check if we are in a while loop
  gen->global_vars = NULL;          // Array of global variables
  gen->global_count = 0;            // Number of global variables
  gen->tf_created = false;          // Flag to check if the temporary frame is created
  gen->function_params = NULL;      // Array of function parameters
  gen->function_param_count = 0;    // Number of function parameters

  return gen;
}

/*
 * 0 - FUNCTION
 * 1 - GETTER
 * 2 - SETTER
 */
 // check if the node is a function, getter or setter
int diverse_function(Generator *generator, tree_node_t *node) {
  (void)generator; // unused parameter
  if (node->children_count >= 2 &&
      node->children[1]->rule == GR_GETTER_DECLARATION) {
    return 1;
  } else if (node->children_count >= 2 &&
             node->children[1]->rule == GR_SETTER_DECLARATION) {
    return 2;
  } else {
    return 0;
  }
}

// store all global variables in an array
char **generate_global_vars(Generator *generator) {
  if (!generator || !generator->symtable) {
    return NULL;
  }

  Symtable *symtable = generator->symtable;
  int capacity = 10;
  int count = 0;
  char **global_vars = malloc(capacity * sizeof(char *));

  if (!global_vars) {
    generator->error = ERR_T_MALLOC_ERR;
    return NULL;
  }
  // handling reapeating global variables, prevents duplicates
  for (int i = 0; i < symtable->symtable_size; i++) {
    if (symtable->symtable_rows[i].symbol != NULL) {
      Symbol *sym = symtable->symtable_rows[i].symbol;
      if (sym->is_global && sym->sym_identif_type == IDENTIF_T_VARIABLE) {
        int exists = 0;
        for (int k = 0; k < count; k++) {
            if (strcmp(global_vars[k], sym->sym_lexeme) == 0) {
                exists = 1;
                break;
            }
        }
        // if the variable is not in the array, add it
        if (!exists) {
            if (count >= capacity) {
              capacity *= 2;
              char **temp = realloc(global_vars, capacity * sizeof(char *));
              if (!temp) {
                free(global_vars);
                generator->error = ERR_T_MALLOC_ERR;
                return NULL;
              }
              global_vars = temp;
            }
            global_vars[count++] = sym->sym_lexeme;
        }
      }
    }
  }

  generator->global_count = count;
  generator->global_vars = global_vars;
  return global_vars;
}


// Free generator
void generator_free(Generator *generator) {
  if (generator) {
    if (generator->current_function) {
      free(generator->current_function);
    }
    if (generator->global_vars) {
      free(generator->global_vars);
    }
    free(generator);
  }
}

// Generate unique label
char *get_next_label(Generator *generator) {
  char *label = malloc(32);
  if (!label) {
    generator->error = ERR_T_MALLOC_ERR;
    return NULL;
  }
  sprintf(label, "LABEL_%d", generator->label_counter++); 
  return label;
}

// get a temporary variable
char *get_temp_var(Generator *generator) {
  char *temp = malloc(32);
  if (!temp) {
    generator->error = ERR_T_MALLOC_ERR;
    return NULL;
  }
  // if the temporary frame is already created
  if (generator->tf_created) {
    sprintf(temp, "TF@tmp_%d", generator->temp_var_counter++);
  } else { // if the temporary frame is not created yet
    generator_emit(generator, "CREATEFRAME");
    generator->tf_created = true;
    sprintf(temp, "TF@tmp_%d", generator->temp_var_counter++);
  }
  return temp;
}

// emit instruction
void generator_emit(Generator *generator, const char *format, ...) {
  FILE *out = generator->output ? generator->output : stdout; // if the output file is not set, use stdout

  va_list args;
  va_start(args, format);
  vfprintf(out, format, args);
  va_end(args);

  fprintf(out, "\n");
}

// Convert float to hexadecimal format
static char *convert_float_to_hex_format(const char *float_str) {
  if (!float_str)
    return NULL;

  double value = strtod(float_str, NULL);

  char *hex_str = malloc(64);
  if (!hex_str)
    return NULL;

  snprintf(hex_str, 64, "%a", value);

  return hex_str;
}

// Convert string to one with escape sequences
static char *convert_string(const char *str) {
  if (!str)
    return NULL;

  int len = strlen(str);
  char *result = malloc(len * 4 + 1);
  if (!result)
    return NULL;

  int pos = 0;
  int i = 0;

  while (i < len) {
    unsigned char c = (unsigned char)str[i];

    if (c == '\\' && i + 1 < len) {
      char next = str[i + 1];
      switch (next) {
      case 'n':
        result[pos++] = '\\';
        result[pos++] = '0';
        result[pos++] = '1';
        result[pos++] = '0';
        i += 2;
        continue;
      case 't':
        result[pos++] = '\\';
        result[pos++] = '0';
        result[pos++] = '0';
        result[pos++] = '9';
        i += 2;
        continue;
      case 'r':
        result[pos++] = '\\';
        result[pos++] = '0';
        result[pos++] = '1';
        result[pos++] = '3';
        i += 2;
        continue;
      case '\\':
        result[pos++] = '\\';
        result[pos++] = '0';
        result[pos++] = '9';
        result[pos++] = '2';
        i += 2;
        continue;
      case '"':
        result[pos++] = '\\';
        result[pos++] = '0';
        result[pos++] = '3';
        result[pos++] = '4';
        i += 2;
        continue;
      case 'x':
        if (i + 3 < len && isxdigit(str[i + 2]) && isxdigit(str[i + 3])) {
          result[pos++] = '\\';
          result[pos++] = 'x';
          result[pos++] = str[i + 2];
          result[pos++] = str[i + 3];
          i += 4;
          continue;
        }
        break;
      }
    }

    if (c == ' ') {
      result[pos++] = '\\';
      result[pos++] = '0';
      result[pos++] = '3';
      result[pos++] = '2';
    } else if (c == '\n') {
      result[pos++] = '\\';
      result[pos++] = '0';
      result[pos++] = '1';
      result[pos++] = '0';
    } else if (c == '#') {
      result[pos++] = '\\';
      result[pos++] = '0';
      result[pos++] = '3';
      result[pos++] = '5';
    } else if (c == '\\') {
      result[pos++] = '\\';
      result[pos++] = '0';
      result[pos++] = '9';
      result[pos++] = '2';
    } else if (c < 32 || c > 126) {
      result[pos++] = '\\';
      int ascii = (int)c;
      if (ascii < 10) {
        result[pos++] = '0';
        result[pos++] = '0';
        result[pos++] = '0' + ascii;
      } else if (ascii < 100) {
        result[pos++] = '0';
        result[pos++] = '0' + (ascii / 10);
        result[pos++] = '0' + (ascii % 10);
      } else {
        result[pos++] = '0' + (ascii / 100);
        result[pos++] = '0' + ((ascii / 10) % 10);
        result[pos++] = '0' + (ascii % 10);
      }
    } else {
      result[pos++] = c;
    }

    i++;
  }

  result[pos] = '\0';
  return result;
}

// Get instruction for operator
const char *get_operator_instruction(const char *operator) {
  if (strcmp(operator, "+") == 0)
    return "ADDS";
  if (strcmp(operator, "-") == 0)
    return "SUBS";
  if (strcmp(operator, "*") == 0)
    return "MULS";
  if (strcmp(operator, "/") == 0)
    return "DIVS";
  if (strcmp(operator, "==") == 0)
    return "EQS";
  if (strcmp(operator, "!=") == 0)
    return "NEQS";
  if (strcmp(operator, "<") == 0)
    return "LTS";
  if (strcmp(operator, ">") == 0)
    return "GTS";
  if (strcmp(operator, "<=") == 0)
    return "LTEQS";
  if (strcmp(operator, ">=") == 0)
    return "GTEQS";
  if (strcmp(operator, "is") == 0)
    return "EQS";
  return NULL;
}

// Search for getter/setter with prefix
static Symbol *search_prefixed_symbol(Symtable *symtable, const char *base_name, const char *prefix, IDENTIF_TYPE target_type) {
  if (!symtable || !base_name || !prefix) {
    return NULL;
  }

  size_t prefix_len = strlen(prefix);
  size_t base_len = strlen(base_name);
  char *prefixed_name = malloc(prefix_len + base_len + 1);
  if (!prefixed_name) {
    return NULL;
  }
  sprintf(prefixed_name, "%s%s", prefix, base_name);

  Symbol *result = NULL;
  for (int i = 0; i < symtable->symtable_size; i++) {
    Symbol *sym = symtable->symtable_rows[i].symbol;
    if (!sym || !sym->sym_lexeme) continue;

    if (strcmp(sym->sym_lexeme, prefixed_name) == 0) {
      if (sym->sym_identif_type == target_type) {
        result = sym;
        break;
      }
    }
  }

  free(prefixed_name);
  return result;
}

// Generate strcmp comparison
void generate_strcmp_comparison(Generator *generator, tree_node_t *node) {
  if (!generator || !node)
    return;

  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
    
    if (child->type == NODE_T_TERMINAL)
        continue;

    if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
        child->nonterm_type == NONTERMINAL_T_FUN_PARAM) {
      generate_expression(generator, child);
    }
  }

  char *temp_var1 = get_temp_var(generator);
  char *temp_var2 = get_temp_var(generator);

  if (!temp_var1 || !temp_var2) {
    if (temp_var1)
      free(temp_var1);
    if (temp_var2)
      free(temp_var2);
    return;
  }
  // Assembly code for strcmp, returns -1, 0 or 1
  generator_emit(generator, "DEFVAR %s", temp_var2);
  generator_emit(generator, "POPS %s", temp_var2);
  generator_emit(generator, "DEFVAR %s", temp_var1);
  generator_emit(generator, "POPS %s", temp_var1);

  char *label_equal = get_next_label(generator);
  char *label_greater = get_next_label(generator);
  char *label_end = get_next_label(generator);

  generator_emit(generator, "PUSHS %s", temp_var1);
  generator_emit(generator, "PUSHS %s", temp_var2);
  generator_emit(generator, "LTS");
  generator_emit(generator, "PUSHS bool@false");
  generator_emit(generator, "JUMPIFEQS %s", label_equal);
  generator_emit(generator, "PUSHS int@-1");
  generator_emit(generator, "JUMP %s", label_end);

  generator_emit(generator, "LABEL %s", label_equal);
  generator_emit(generator, "PUSHS %s", temp_var1);
  generator_emit(generator, "PUSHS %s", temp_var2);
  generator_emit(generator, "EQS");
  generator_emit(generator, "PUSHS bool@false");
  generator_emit(generator, "JUMPIFEQS %s", label_greater);
  generator_emit(generator, "PUSHS int@0");
  generator_emit(generator, "JUMP %s", label_end);

  generator_emit(generator, "LABEL %s", label_greater);
  generator_emit(generator, "PUSHS int@1");

  generator_emit(generator, "LABEL %s", label_end);

  free(temp_var1);
  free(temp_var2);
  free(label_equal);
  free(label_greater);
  free(label_end);
}


// Get scope suffix for local variable for correct declaration in various code blocks
static int get_scope_suffix(Generator *generator, Token *token) {
  if (!token) return generator->current_scope;
  
  // Collect all symbols with matching name
  Symbol *candidates[32];
  int candidate_count = 0;
  for (int i = 0; i < generator->symtable->symtable_size && candidate_count < 32; i++) {
      Symbol *s = generator->symtable->symtable_rows[i].symbol;
      if (!s || !s->sym_lexeme) continue;
      // can not be global or parameter
      if (strcmp(s->sym_lexeme, token->token_lexeme) == 0 && 
          s->sym_identif_type == IDENTIF_T_VARIABLE && !s->is_global && !s->is_parameter) {
          candidates[candidate_count++] = s;
      }
  }
  
  if (candidate_count == 0) {
    return generator->current_scope; // if no candidates, return current scope
  }

  int token_scope = token->scope;
  int token_line = token->token_line_number;
  
  // Check if declared in current scope BEFORE the token
  // this ensures variables are declared before they are used
  for (int c = 0; c < candidate_count; c++) {
      Symbol *s = candidates[c];
      if (!s->sym_identif_declaration_count) continue;
      for (int i = 0; i < s->sym_identif_declaration_count; i++) {
          if (s->sym_identif_declared_at_scope_arr[i] == token_scope) {
              int decl_line = s->sym_identif_declared_at_line_arr[i];
              if (decl_line < token_line) {
                  // Found declaration in current scope before usage - use it
                  return token_scope;
              }
          }
      }
  }
  
  // Not declared in current scope before usage - find declaration in outer scopes
  // Build list of parent (outer) scopes
  int parent_scopes[64];
  int parent_count = 0;
  
  if (token->previous_scope_arr && token->scope_count > 0) {
      for (int j = 0; j < token->scope_count; j++) {
          parent_scopes[parent_count++] = token->previous_scope_arr[j];
      }
  }
  
  // Also check generator->current_scope if it's different and might be a parent
  if (generator->current_scope != token_scope) {
      // Check if it's already in parent_scopes
      bool found = false;
      for (int j = 0; j < parent_count; j++) {
          if (parent_scopes[j] == generator->current_scope) {
              found = true;
              break;
          }
      }
      if (!found) {
          parent_scopes[parent_count++] = generator->current_scope;
      }
  }
  
  // Find the highest scope number among parent scopes that has a declaration
  int best_parent_scope = -1; // best parent scope is the highest scope number among parent scopes that has a declaration
  for (int c = 0; c < candidate_count; c++) {
      Symbol *s = candidates[c];
      if (!s->sym_identif_declaration_count) continue;
      
      for (int i = 0; i < s->sym_identif_declaration_count; i++) {
          int decl_scope = s->sym_identif_declared_at_scope_arr[i];
          int decl_line = s->sym_identif_declared_at_line_arr[i];
          
          // Skip declarations that occur AFTER the usage line
          if (decl_line >= token_line) {
              continue;
          }
          
          // Check if this declaration scope is in parent scopes
          for (int p = 0; p < parent_count; p++) {
              if (parent_scopes[p] == decl_scope) {
                  // Found declaration in a parent scope BEFORE usage
                  // Prefer the highest scope number (most recent/closest parent)
                  if (best_parent_scope < 0 || decl_scope > best_parent_scope) {
                      best_parent_scope = decl_scope;
                  }
              }
          }
      }
  }
  
  if (best_parent_scope >= 0) {
      return best_parent_scope;
  }
  
  // Fallback: return the most recent declaration from first candidate
  Symbol *sym = candidates[0];
  if (sym && sym->sym_identif_declaration_count > 0) {
      return sym->sym_identif_declared_at_scope_arr[sym->sym_identif_declaration_count - 1];
  }
  
  // if nothing found, return current scope
  return generator->current_scope;
}

// Check if variable is function parameter
static bool is_function_parameter(Generator *generator, const char *var_name) {
  if (!generator->in_function || !var_name || !generator->function_params) {
    return false;
  }
  
  // Check stored parameter names
  for (int i = 0; i < generator->function_param_count; i++) {
    if (generator->function_params[i] && strcmp(generator->function_params[i], var_name) == 0) {
      return true;
    }
  }
  
  return false;
}

// Format local variable name
static void format_local_var(Generator *generator, Token *token, char *buffer, size_t buffer_size) {
  if (!token) {
    snprintf(buffer, buffer_size, "LF@");
    return;
  }
  
  if (generator->in_function && is_function_parameter(generator, token->token_lexeme)) {
    snprintf(buffer, buffer_size, "LF@%s", token->token_lexeme);
    return;
  }
  
  int scope = get_scope_suffix(generator, token); // get scope suffix for local variable
  snprintf(buffer, buffer_size, "LF@%s$%d", token->token_lexeme, scope);
}


// Generate terminal
void generate_terminal(Generator *generator, tree_node_t *node) {
  if (!node || !node->token) {
    return;
  }
  Token *token = node->token;

  switch (token->token_type) {
  case TOKEN_T_NUM: { // a case with number
    char *hex_str = convert_float_to_hex_format(token->token_lexeme);
    if (hex_str) {
      generator->is_float = true;
      generator_emit(generator, "PUSHS float@%s", hex_str);
      free(hex_str);
    } else {
      generator->error = ERR_T_MALLOC_ERR;
    }
    break;
  }
  case TOKEN_T_STRING: { // a case with string
    char *str_value = token->token_lexeme;
    char *clean_str = NULL;

    // remove quotes at the beginning and end
    if (str_value[0] == '"' && str_value[strlen(str_value) - 1] == '"') {
      int len = strlen(str_value) - 2;
      clean_str = malloc(len + 1);
      if (clean_str) {
        strncpy(clean_str, str_value + 1, len);
        clean_str[len] = '\0';
      }
    } else {
      // copy of the original string
      clean_str = malloc(strlen(str_value) + 1);
      if (clean_str) {
        strcpy(clean_str, str_value);
      }
    }

    if (clean_str) {
      // convert to string with escape sequences
      char *converted = convert_string(clean_str);
      if (converted) {
        generator_emit(generator, "PUSHS string@%s", converted);
        free(converted);
      } else {
        generator->error = ERR_T_MALLOC_ERR;
        generator_emit(generator, "PUSHS string@%s", clean_str);
      }
      free(clean_str);
    } else {
      generator->error = ERR_T_MALLOC_ERR;
      generator_emit(generator, "PUSHS string@%s", str_value);
    }
    break;
  }
  case TOKEN_T_KEYWORD: { // a case with a keyword
    if (strcmp(token->token_lexeme, "true") == 0) {
      generator_emit(generator, "PUSHS bool@true");
    } else if (strcmp(token->token_lexeme, "false") == 0) {
      generator_emit(generator, "PUSHS bool@false");
    } else if (strcmp(token->token_lexeme, "null") == 0) {
      generator_emit(generator, "PUSHS nil@nil");
    }
    break;
  }
  case TOKEN_T_IDENTIFIER: // a case with either identifier or global variable
  case TOKEN_T_GLOBAL_VAR: {
    Symbol *sym = search_table(token, generator->symtable);
    
    // always try to find getter
    int is_getter = 0;
    Symbol *getter_sym = search_prefixed_symbol(generator->symtable, token->token_lexeme, "getter+", IDENTIF_T_GETTER);
    if (getter_sym) {
        sym = getter_sym;
        is_getter = 1;
    } else if (sym && sym->sym_identif_type == IDENTIF_T_GETTER) {
        is_getter = 1;
    }

    if (sym) {
      // if it is a getter, call it as a function without parameters
      if (sym->sym_identif_type == IDENTIF_T_GETTER || is_getter) {
        generator->is_called = true;
        generator_emit(generator, "CALL %s_", token->token_lexeme);
        generator->is_called = false;  // Reset after call
      } else if (sym->is_global) {
        generator_emit(generator, "PUSHS GF@%s", token->token_lexeme);
      } else {
        char var_name[256]; // format local variable name with scope suffix
        format_local_var(generator, token, var_name, sizeof(var_name));
        generator_emit(generator, "PUSHS %s", var_name);
      }
    } else {
      char var_name[256];
      format_local_var(generator, token, var_name, sizeof(var_name));
      generator_emit(generator, "PUSHS %s", var_name);
    }
    break;
  }
  case TOKEN_T_OPERATOR: // a case with an operator
    break;
  default:
    break;
  }
}

// Generate type check for [ is ] operator
static void generate_type_check(Generator *generator, tree_node_t *node) {
  if (!node || node->children_count < 2) // if the node is not valid or has less than 2 children
    return;

  generate_expression(generator, node->children[0]); // generate expression for left operand

  tree_node_t *right_operand = node->children[1];
  // if the right operand is a terminal and has a token and the token type is a keyword
  if (right_operand && right_operand->type == NODE_T_TERMINAL &&
      right_operand->token &&
      right_operand->token->token_type == TOKEN_T_KEYWORD) {
    // if the right operand is a keyword "Num"
    if (strcmp(right_operand->token->token_lexeme, "Num") == 0) {
      generator_emit(generator, "TYPES");
      
      char *type_var = get_temp_var(generator);
      generator_emit(generator, "DEFVAR %s", type_var);
      generator_emit(generator, "POPS %s", type_var);

      generator_emit(generator, "PUSHS %s", type_var);
      generator_emit(generator, "PUSHS string@int");
      generator_emit(generator, "EQS"); 

      generator_emit(generator, "PUSHS %s", type_var);
      generator_emit(generator, "PUSHS string@float");
      generator_emit(generator, "EQS"); 

      generator_emit(generator, "ORS");
      
      free(type_var);
      // if the right operand is a keyword "String"
    } else if (strcmp(right_operand->token->token_lexeme, "String") == 0) {
      generator_emit(generator, "TYPES");
      generator_emit(generator, "PUSHS string@string");
      generator_emit(generator, "EQS");
      // if the right operand is a keyword "Null"
    } else if (strcmp(right_operand->token->token_lexeme, "Null") == 0) {
      generator_emit(generator, "PUSHS nil@nil");
      generator_emit(generator, "EQS");
    } else { // Fallback
        generate_expression(generator, node->children[1]);
        generator_emit(generator, "TYPES");
        generator_emit(generator, "EQS");
    }
  } else { // if the right operand is not a keyword
    generate_expression(generator, node->children[1]);
    generator_emit(generator, "TYPES");
    generator_emit(generator, "EQS");
  }
}
// Forward declarations
static bool is_number_type(Generator *generator, tree_node_t *node);

// Check if node evaluates to string
static bool is_string_type(Generator *generator, tree_node_t *node) {
  if (!node) return false;

  // returns true if the node evaluates to a string or a string variable
  if (node->type == NODE_T_TERMINAL) {
    if (node->token) {
      if (node->token->token_type == TOKEN_T_STRING) return true;
      if (node->token->token_type == TOKEN_T_IDENTIFIER || node->token->token_type == TOKEN_T_GLOBAL_VAR) {
        Symbol *sym = search_table(node->token, generator->symtable);
        if (sym) {
          if (sym->sym_variable_type == VAR_T_STRING) return true;
        }
      }
    }
    return false;
  }

  // [ + ] operator
  if (node->token && strcmp(node->token->token_lexeme, "+") == 0 && node->children_count == 2) {
    // If either operand is a string andthe result is a string (concatenation)
    return is_string_type(generator, node->children[0]) || is_string_type(generator, node->children[1]);
  }

  // [ * ] operator with string
  if (node->token && strcmp(node->token->token_lexeme, "*") == 0 && node->children_count == 2) {
    // If left is string and right is number, result is string
    if (is_string_type(generator, node->children[0]) && is_number_type(generator, node->children[1])) {
      return true;
    }
  }

  // Function calls that return strings
  if (node->nonterm_type == NONTERMINAL_T_FUN_CALL || node->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN) {
    if (node->children_count > 0 && node->children[0]->token) {
      char *name = node->children[0]->token->token_lexeme;
      if (strcmp(name, "read_str") == 0 || strcmp(name, "str") == 0 || 
          strcmp(name, "substring") == 0 || strcmp(name, "chr") == 0) {
        return true;
      }
    }
  }
  
  return false;
}

// Check if node is a number
static bool is_number_type(Generator *generator, tree_node_t *node) {
  if (!node) return false;

  if (node->type == NODE_T_TERMINAL) {
    if (node->token) {
      if (node->token->token_type == TOKEN_T_NUM) return true;
      if (node->token->token_type == TOKEN_T_IDENTIFIER || node->token->token_type == TOKEN_T_GLOBAL_VAR) {
        Symbol *sym = search_table(node->token, generator->symtable);
        if (sym && sym->sym_variable_type == VAR_T_NUM) return true;
      }
    }
    return false;
  }

  // arithmetic operations with numbers
  if (node->token && node->children_count == 2) {
    const char *op = node->token->token_lexeme;
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || 
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
      // Check both operands
      if (is_number_type(generator, node->children[0]) && is_number_type(generator, node->children[1])) {
        return true;
      }
    }
  }

  // Function calls that return numbers
  if (node->nonterm_type == NONTERMINAL_T_FUN_CALL || node->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN) {
    if (node->children_count > 0 && node->children[0]->token) {
      char *name = node->children[0]->token->token_lexeme;
      if (strcmp(name, "read_num") == 0 || strcmp(name, "floor") == 0 || 
          strcmp(name, "length") == 0 || strcmp(name, "ord") == 0) {
        return true;
      }
    }
  }
  
  return false;
}

// Convert float to int before WRITE if needed
static void convert_float_to_int_before_write(Generator *generator, const char *temp) {
  if (!generator || !temp) return;
  
  char *type_var = get_temp_var(generator);
  char *skip_convert_label = get_next_label(generator);
  generator_emit(generator, "DEFVAR %s", type_var);
  generator_emit(generator, "PUSHS %s", temp);
  generator_emit(generator, "TYPES");
  generator_emit(generator, "POPS %s", type_var);
  generator_emit(generator, "PUSHS %s", type_var);
  generator_emit(generator, "PUSHS string@float");
  generator_emit(generator, "EQS");
  generator_emit(generator, "PUSHS bool@false");
  generator_emit(generator, "JUMPIFEQS %s", skip_convert_label);
  // Convert float to int
  generator_emit(generator, "PUSHS %s", temp);
  generator_emit(generator, "FLOAT2INTS");
  generator_emit(generator, "POPS %s", temp);
  generator_emit(generator, "LABEL %s", skip_convert_label);
  
  free(type_var);
  free(skip_convert_label);
}

// Get operand string for CONCAT
static char *get_operand_for_concat(Generator *generator, tree_node_t *node) {
  if (node->type == NODE_T_TERMINAL) {
    Token *token = node->token;
    if (token->token_type == TOKEN_T_STRING) {
      char *str_value = token->token_lexeme;
      char *clean_str = NULL;
      // remove quotes at the beginning and end
      if (str_value[0] == '"' && str_value[strlen(str_value) - 1] == '"') {
        int len = strlen(str_value) - 2;
        clean_str = malloc(len + 1);
        if (clean_str) {
          strncpy(clean_str, str_value + 1, len);
          clean_str[len] = '\0';
        }
      } else {
        clean_str = malloc(strlen(str_value) + 1);
        if (clean_str) strcpy(clean_str, str_value);
      }
      // convert to string with escape sequences
      char *converted = convert_string(clean_str);
      free(clean_str);
      
      if (!converted) {
        return NULL;
      }
      // allocate memory for the result
      char *result = malloc(strlen(converted) + 10);
      if (!result) {
        free(converted);
        return NULL;
      }
      // format the result
      sprintf(result, "string@%s", converted);
      free(converted);
      return result;
    } else if (token->token_type == TOKEN_T_IDENTIFIER || token->token_type == TOKEN_T_GLOBAL_VAR) {
       Symbol *sym = search_table(token, generator->symtable);
       
       bool is_getter = false; // flag to check if the symbol is a getter
       Symbol *getter_sym = search_prefixed_symbol(generator->symtable, token->token_lexeme, "getter+", IDENTIF_T_GETTER);
       if (getter_sym) {
         is_getter = true;
       } else if (sym && sym->sym_identif_type == IDENTIF_T_GETTER) {
         is_getter = true;
       }
       
       if (!is_getter) {
         char *result = malloc(strlen(token->token_lexeme) + 30);
         if (!result) {
           return NULL;
         }
         if (sym && sym->is_global) {
           sprintf(result, "GF@%s", token->token_lexeme);
         } else {
          format_local_var(generator, token, result, strlen(token->token_lexeme) + 30);
         }
         return result;
       }
    }
  }
  
  generate_expression(generator, node);
  char *temp = get_temp_var(generator);
  if (!temp) {
    return NULL;
  }
  // define the temporary variable
  generator_emit(generator, "DEFVAR %s", temp);
  // pop the result from the stack
  generator_emit(generator, "POPS %s", temp);
  return temp; // return the temporary variable
}

// Generate expression
void generate_expression(Generator *generator, tree_node_t *node) {
  if (!node) {
    return;
  }
  
  // if the node is a terminal and not an operator
  if (node->type == NODE_T_TERMINAL && 
      !(node->token && node->token->token_type == TOKEN_T_OPERATOR && node->children_count > 0)) {
    generate_terminal(generator, node);
    return;
  }

  if (node->type == NODE_T_NONTERMINAL &&
      node->nonterm_type == NONTERMINAL_T_PREDICATE) {
    if (node->children_count > 0) {
      generate_expression(generator, node->children[0]);
    }
    return;
  }

  if (node->children_count > 0) {
    if (node->token && node->token->token_type == TOKEN_T_OPERATOR) {
      // handling minus
      if (strcmp(node->token->token_lexeme, "-") == 0 &&
          node->children_count == 1) {
        generate_expression(generator, node->children[0]);
        generator_emit(generator, "NEGS");
        return;
      }
      // handling not
      if (strcmp(node->token->token_lexeme, "!") == 0 &&
          node->children_count == 1) {
        generate_expression(generator, node->children[0]);
        generator_emit(generator, "NOTS");
        return;
      }

      if (node->children_count >= 2) {
        // handling is operator
        if (strcmp(node->token->token_lexeme, "is") == 0) {
          generate_type_check(generator, node);
          return;
        }

        if (strcmp(node->token->token_lexeme, "+") == 0) {
             // If either operand is a string, use CONCAT
             if (is_string_type(generator, node->children[0]) || is_string_type(generator, node->children[1])) {
                 char *op1 = get_operand_for_concat(generator, node->children[0]);
                 char *op2 = get_operand_for_concat(generator, node->children[1]);
                 
                 char *temp_res = get_temp_var(generator);
                 generator_emit(generator, "DEFVAR %s", temp_res);
                 generator_emit(generator, "CONCAT %s %s %s", temp_res, op1, op2);
                 generator_emit(generator, "PUSHS %s", temp_res);
                 
                 free(op1);
                 free(op2);
                 free(temp_res);
                 return;
             }
        }

        if (strcmp(node->token->token_lexeme, "*") == 0) {
             // If left operand is string and right is number, repeat string
             if (is_string_type(generator, node->children[0]) && is_number_type(generator, node->children[1])) {
                 // Generate string operand
                 char *str_op = get_operand_for_concat(generator, node->children[0]);
                 if (!str_op) {
                   generator->error = ERR_T_MALLOC_ERR;
                   return;
                 }
                 
                 // Generate number operand
                 generate_expression(generator, node->children[1]);
                 
                 // Convert to int if needed (FLOAT2INTS)
                 generator_emit(generator, "FLOAT2INTS");
                 
                 // Variables for the loop
                 char *str_var = get_temp_var(generator);
                 char *count_var = get_temp_var(generator);
                 char *result_var = get_temp_var(generator);
                 char *i_var = get_temp_var(generator);
                 char *cond_var = get_temp_var(generator);
                 
                 if (!str_var || !count_var || !result_var || !i_var || !cond_var) {
                   if (str_var) free(str_var);
                   if (count_var) free(count_var);
                   if (result_var) free(result_var);
                   if (i_var) free(i_var);
                   if (cond_var) free(cond_var);
                   free(str_op);
                   generator->error = ERR_T_MALLOC_ERR;
                   return;
                 }
                 
                 int label_idx = generator->label_counter++;
                 
                 // Define variables
                 generator_emit(generator, "DEFVAR %s", str_var);
                 generator_emit(generator, "DEFVAR %s", count_var);
                 generator_emit(generator, "DEFVAR %s", result_var);
                 generator_emit(generator, "DEFVAR %s", i_var);
                 generator_emit(generator, "DEFVAR %s", cond_var);
                 
                 // Pop operands: count (top)
                 generator_emit(generator, "POPS %s", count_var);
                 
                 // Load string operand (can be literal or variable reference)
                 generator_emit(generator, "MOVE %s %s", str_var, str_op);
                 
                 // Initialize result as empty string
                 generator_emit(generator, "MOVE %s string@", result_var);
                 
                 // Initialize counter i = 0
                 generator_emit(generator, "MOVE %s int@0", i_var);
                 
                 // Loop label
                 generator_emit(generator, "LABEL strmul_loop_%d", label_idx);
                 
                 // Check if i < count
                 generator_emit(generator, "LT %s %s %s", cond_var, i_var, count_var);
                 generator_emit(generator, "JUMPIFEQ strmul_end_%d %s bool@false", label_idx, cond_var);
                 
                 // Concatenate: result = result + str
                 generator_emit(generator, "CONCAT %s %s %s", result_var, result_var, str_var);
                 
                 // Increment i
                 generator_emit(generator, "ADD %s %s int@1", i_var, i_var);
                 
                 // Jump back to loop
                 generator_emit(generator, "JUMP strmul_loop_%d", label_idx);
                 
                 // End label
                 generator_emit(generator, "LABEL strmul_end_%d", label_idx);
                 
                 // Push result on stack
                 generator_emit(generator, "PUSHS %s", result_var);
                 
                 // Free variables
                 free(str_var);
                 free(count_var);
                 free(result_var);
                 free(i_var);
                 free(cond_var);
                 free(str_op);
                 return;
             }
        }

        generate_expression(generator, node->children[0]);
        generate_expression(generator, node->children[1]);

        const char *op_inst = get_operator_instruction(node->token->token_lexeme);
        if (op_inst) {
          generator_emit(generator, "%s", op_inst);
        } else {
          generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_OPERAND_TYPES;
        }
        return;
      }
    } else {
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child && child->type == NODE_T_NONTERMINAL) {
          if (child->nonterm_type == NONTERMINAL_T_FUN_CALL ||
              child->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN) {
            generator_generate(generator, child);
          } else {
            generate_expression(generator, child);
          }
        } else {
          generate_expression(generator, child);
        }
      }
    }
  }
}

// Generate main code
int generator_start(Generator *generator, tree_node_t *tree) {
  if (!generator || !tree) {
    return ERR_T_SYNTAX_ERR;
  }

  generator_emit(generator, ".IFJcode25");

  // Jump to main at the start to ensure execution begins at main
  generator_emit(generator, "JUMP main");

  // iterate through the tree
  for (int i = 0; i < tree->children_count; i++) {
    generator_generate(generator, tree->children[i]);
  }

  return generator->error;
}

// Main generation function
void generator_generate(Generator *generator, tree_node_t *node) {
  if (!generator || !node) {
    return;
  }

  // if the node is a terminal, skip -> handle with generate_terminal()
  if (node->type == NODE_T_TERMINAL) { 
    return;
  }

  switch (node->nonterm_type) {
  case NONTERMINAL_T_CODE_BLOCK: // a case with a code block
    generate_code_block(generator, node);
    break;
  case NONTERMINAL_T_SEQUENCE: // a case with a sequence
    generate_sequence(generator, node);
    break;
  case NONTERMINAL_T_INSTRUCTION: // a case with an instruction
    generate_instruction(generator, node);
    break;
  case NONTERMINAL_T_EXPRESSION: // a case with an expression
    generate_expression(generator, node);
    break;
  case NONTERMINAL_T_EXPRESSION_OR_FN: // a case with an expression or a function call
    for (int i = 0; i < node->children_count; i++) {
      generator_generate(generator, node->children[i]);
    }
    break;
  case NONTERMINAL_T_ASSIGNMENT: // a case with an assignment
    generate_assignment(generator, node);
    break;
  case NONTERMINAL_T_DECLARATION: // a case with a declaration
    if (node->rule == GR_FUN_DECLARATION) {
      generate_function_declaration(generator, node);

    } else {
      generate_declaration(generator, node);
    }
    break;
  case NONTERMINAL_T_FUN_CALL: // a case with a function call
    generate_function_call(generator, node);
    break;
  case NONTERMINAL_T_IF: // a case with an if statement
    generate_if(generator, node);
    break;
  case NONTERMINAL_T_WHILE: // a case with a while statement
    generate_while(generator, node);
    break;
  case NONTERMINAL_T_RETURN: // a case with a return statement
    generate_return(generator, node);
    break;
  case NONTERMINAL_T_PREDICATE: // a case with a predicate
    generate_expression(generator, node);
    break;
  default:
    for (int i = 0; i < node->children_count; i++) {
      generator_generate(generator, node->children[i]);
    }
    break;
  }
}
// Generate code block
void generate_code_block(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  generator->current_scope++;

  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];

    // skip terminals like {, }, \n
    if (child->type == NODE_T_TERMINAL) {
      Token *token = child->token;
      if (token &&
          (strcmp(token->token_lexeme, "{") == 0 ||
           strcmp(token->token_lexeme, "}") == 0 ||
           strcmp(token->token_lexeme, "n") == 0 ||
           strcmp(token->token_lexeme, "\n") == 0)) {
        continue;
      }
    }

    generator_generate(generator, child);
  }

  generator->current_scope--;
}

// Generate sequence
void generate_sequence(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];

    if (child->type == NODE_T_TERMINAL) {
      Token *token = child->token;
      if (token && (strcmp(token->token_lexeme, "n") == 0 || strcmp(token->token_lexeme, "\n") == 0)) {
        continue;
      }
    }

    generator_generate(generator, child);
  }
}

// Generate instruction
void generate_instruction(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  for (int i = 0; i < node->children_count; i++) {
    generator_generate(generator, node->children[i]);
  }
}

// Generate assignment
void generate_assignment(Generator *generator, tree_node_t *node) {
  if (!node || node->children_count < 1)
    return;

  tree_node_t *id_node = node->children[0];
  if (!id_node || !id_node->token)
    return;

  Token *id_token = id_node->token;

  tree_node_t *expr_node = NULL;
  for (int i = 1; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
    // skip terminal node "="
    if (child->type == NODE_T_TERMINAL && child->token &&
        strcmp(child->token->token_lexeme, "=") == 0) {
      continue;
    }
    if (child->type == NODE_T_NONTERMINAL &&
        (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
         child->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN ||
         child->nonterm_type == NONTERMINAL_T_FUN_CALL)) {
      expr_node = child;
      break;
    } else if (child->type == NODE_T_TERMINAL) {
      expr_node = child;
      break;
    }
  }

  if (expr_node) {
    Symbol *sym = search_table(id_token, generator->symtable);
    
    Symbol *setter_sym = search_prefixed_symbol(generator->symtable, id_token->token_lexeme, "setter+", IDENTIF_T_SETTER);

    // save old value (if exists)
    bool old_is_global = generator->is_global;

    // set before generating expression, to handle functions inside the expression
    if ((sym && sym->sym_identif_type != IDENTIF_T_SETTER) && !setter_sym) {
      generator->is_global = (sym && sym->is_global) ? true : false;
    }

    if (expr_node->nonterm_type == NONTERMINAL_T_FUN_CALL ||
               expr_node->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN) {
      generator_generate(generator, expr_node);
    } else {
      generate_expression(generator, expr_node);
    }

    if (setter_sym && setter_sym->sym_identif_type == IDENTIF_T_SETTER) {
      generator->is_called = true;
      generator_emit(generator, "CALL %s__", id_token->token_lexeme); // call setter function with double underscore
      generator->is_global = old_is_global;
    } else if (sym && sym->sym_identif_type == IDENTIF_T_SETTER) {
       generator->is_called = true;
       generator_emit(generator, "CALL %s__", id_token->token_lexeme); // call setter function with double underscore
       generator->is_global = old_is_global;
    } else if (sym && sym->is_global) {
      generator_emit(generator, "POPS GF@%s", id_token->token_lexeme); // pop global variable from the stack
    } else {
      char var_name[256];
      format_local_var(generator, id_token, var_name, sizeof(var_name));
      generator_emit(generator, "POPS %s", var_name);
    }
  }
}

// Generate declaration
void generate_declaration(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  // check grammar rule
  if (node->rule == GR_FUN_DECLARATION) {
    generate_function_declaration(generator, node);
  } else if (node->rule == GR_DECLARATION) {
    if (node->children_count > 0) {
      tree_node_t *id_node = node->children[0];
      if (id_node && id_node->token) {
        Token *id_token = id_node->token;

        Symbol *sym = search_table(id_token, generator->symtable);

        if (sym && sym->is_global) {
        } else if (!generator->in_while_loop) {
          int scope = id_token->scope;
          generator_emit(generator, "DEFVAR LF@%s$%d", id_token->token_lexeme, scope);
        }
        // iterate through the children
        for (int i = 1; i < node->children_count; i++) {
          tree_node_t *child = node->children[i];
          if (child->nonterm_type == NONTERMINAL_T_EXPRESSION) {
            generate_expression(generator, child);
            if (sym && sym->is_global) {
              generator_emit(generator, "POPS GF@%s", id_token->token_lexeme);
            } else {
              char var_name[256];
              format_local_var(generator, id_token, var_name, sizeof(var_name));
              generator_emit(generator, "POPS %s", var_name);
            }
            break;
          } else if (child->nonterm_type == NONTERMINAL_T_ASSIGNMENT) {
            // For assignment nodes, use generator_generate to handle it properly
            generator_generate(generator, child);
            break;
          }
        }
      }
    }
  }
}

// Generate function declaration
void generate_function_declaration(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  tree_node_t *func_name_node = NULL;
  // iterate through the children   
  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
    if (child->type == NODE_T_TERMINAL && child->token &&
        (child->token->token_type == TOKEN_T_IDENTIFIER)) {
      func_name_node = child;
      break;
    }
  }

  if (!func_name_node || !func_name_node->token)
    return;

  char *func_name = func_name_node->token->token_lexeme;

  if (generator->current_function) {
    free(generator->current_function);
  }
  generator->current_function = malloc(strlen(func_name) + 1);
  if (generator->current_function) {
    strcpy(generator->current_function, func_name);
  }
  generator->in_function = 1;
  generator->has_return = false;  // Reset return flag for each function

  // Clear parameter list at the start of each function
  // (will be used if function has parameters)
  if (generator->function_params) {
      for (int i = 0; i < generator->function_param_count; i++) {
          if (generator->function_params[i]) {
              free(generator->function_params[i]);
          }
      }
      free(generator->function_params);
      generator->function_params = NULL;
      generator->function_param_count = 0;
  }

  Symbol *sym = search_table(func_name_node->token, generator->symtable);

  int type = diverse_function(generator, node);
  
  // Strip prefix for label generation if needed
  char *label_name = func_name;
  if (strncmp(func_name, "getter+", 7) == 0) {
      label_name = func_name + 7;
  } else if (strncmp(func_name, "setter+", 7) == 0) {
      label_name = func_name + 7;
  }

  // generate label for the getter
  if (type == 1) {
    generator_emit(generator, "LABEL %s_", label_name);

  } else if (type == 2) {
    // generate label for the setter
    generator_emit(generator, "LABEL %s__", label_name);
  } else {
    if (sym && sym->sym_identif_declaration_count > 1) {
        // find overload index
        int overload_index = 0;
        if (func_name_node && func_name_node->token) {
            for (int i = 0; i < sym->sym_identif_declaration_count; i++) {
                if (sym->sym_identif_declared_at_line_arr[i] == func_name_node->token->token_line_number) {
                    overload_index = i;
                    break;
                }
            }
        }
        generator_emit(generator, "LABEL %s$%d", func_name, overload_index); // generate label for the function with overload index
    } else {
        generator_emit(generator, "LABEL %s", func_name); // generate label for the function
    }

  }
  // create and push frame
  generator_emit(generator, "CREATEFRAME");
  generator_emit(generator, "PUSHFRAME");
  generator->tf_created = false;
  if(strcmp(func_name, "main") == 0){
    for(int i = 0; i < generator->global_count; i++) {
      generator_emit(generator, "DEFVAR GF@%s", generator->global_vars[i]);
      generator_emit(generator, "PUSHS nil@nil");
      generator_emit(generator, "POPS GF@%s", generator->global_vars[i]);
  
    }
  }

  Symbol *func_sym = NULL;
  if (func_name_node && func_name_node->token && generator->symtable) {
    func_sym = search_table(func_name_node->token, generator->symtable);
  }

  if (type == 2) { // setter parameter generation
      if (node->children_count >= 2 && node->children[1]->children_count >= 1) {
          tree_node_t *param_node = node->children[1]->children[0];
          if (param_node && param_node->token && param_node->token->token_type == TOKEN_T_IDENTIFIER) {
              char *param_name = param_node->token->token_lexeme;
              
              // Store parameter name for later reference checking
              if (generator->function_params) {
                  for (int i = 0; i < generator->function_param_count; i++) {
                      if (generator->function_params[i]) {
                          free(generator->function_params[i]);
                      }
                  }
                  free(generator->function_params);
              }
              generator->function_param_count = 1;
              generator->function_params = malloc(sizeof(char *));
              generator->function_params[0] = strdup(param_name);
              
              generator_emit(generator, "DEFVAR LF@%s", param_name);
              generator_emit(generator, "POPS LF@%s", param_name);
          }
      }
  }

  if (func_sym && func_sym->sym_identif_type == IDENTIF_T_FUNCTION) {
    // use AST to find parameter names
    tree_node_t *param_node = NULL;
    for (int i = 0; i < node->children_count; i++) {
        if (node->children[i]->rule == GR_FUN_PARAM) {
            param_node = node->children[i];
            break;
        }
    }

    if (param_node) {
        // collect parameters first to handle order
        // using max 32 params for simplicity
        Token *params[32];
        int p_count = 0;
        
        tree_node_t *current = param_node;
        while (current && p_count < 32) {
            // Check children for identifier
            for (int i = 0; i < current->children_count; i++) {
                tree_node_t *child = current->children[i];
                if (child->type == NODE_T_TERMINAL && child->token && child->token->token_type == TOKEN_T_IDENTIFIER) {
                     params[p_count++] = child->token;
                }
            }
            
            // Find next GR_FUN_PARAM (prime)
            tree_node_t *next = NULL;
            for (int i = 0; i < current->children_count; i++) {
                 if (current->children[i]->rule == GR_FUN_PARAM) {
                     next = current->children[i];
                     break;
                 }
            }
            current = next;
        }

        // Store parameter names for later reference checking
        // Free old parameter list if any
        if (generator->function_params) {
            for (int i = 0; i < generator->function_param_count; i++) {
                if (generator->function_params[i]) {
                    free(generator->function_params[i]);
                }
            }
            free(generator->function_params);
        }
        generator->function_param_count = p_count;
        if (p_count > 0) {
            generator->function_params = malloc(sizeof(char *) * p_count);
            for (int i = 0; i < p_count; i++) {
                generator->function_params[i] = strdup(params[i]->token_lexeme);
            }
        } else {
            generator->function_params = NULL;
        }

        // DEFVAR for all params
        // Parameters use no suffix - they're declared as LF@param (not LF@param$N)
        // Local variables in the function body will get suffixes to avoid conflicts
        for (int i = 0; i < p_count; i++) {
            generator_emit(generator, "DEFVAR LF@%s", params[i]->token_lexeme);
        }

        // POPS in reverse order (last param is on top of stack)
        for (int i = p_count - 1; i >= 0; i--) {
            generator_emit(generator, "POPS LF@%s", params[i]->token_lexeme);
        }
    }
  }

  tree_node_t *code_block = NULL;
  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
      if (child && child->type == NODE_T_NONTERMINAL) {
        if (child->nonterm_type == NONTERMINAL_T_CODE_BLOCK) {
          code_block = child;
          break;
      }
      for (int j = 0; j < child->children_count; j++) {
        tree_node_t *grandchild = child->children[j];
          if (grandchild && grandchild->type == NODE_T_NONTERMINAL &&
              grandchild->nonterm_type == NONTERMINAL_T_CODE_BLOCK) {
            code_block = grandchild;
            break;
        }
      }
      if (code_block)
        break;
    }
  }

  if (code_block) {
    generate_code_block(generator, code_block);
  }

  // pop frame before return
  // Implicit return nil if execution reaches here (only if no explicit return was generated)
  if (strcmp(generator->current_function, "main") == 0) {
      generator_emit(generator, "POPFRAME");
      generator->tf_created = false;
  } else if (!generator->has_return) {
      generator_emit(generator, "PUSHS nil@nil");
      generator_emit(generator, "POPFRAME");
      generator_emit(generator, "RETURN");
  }

  generator->in_function = 0;
  
  // Free parameter list
  if (generator->function_params) {
      for (int i = 0; i < generator->function_param_count; i++) {
          if (generator->function_params[i]) {
              free(generator->function_params[i]);
          }
      }
      free(generator->function_params);
      generator->function_params = NULL;
      generator->function_param_count = 0;
  }
  
  if (generator->current_function) {
    free(generator->current_function);
    generator->current_function = NULL;
  }
}

// Check built-in function parameters
void check_builtin_params(Generator *generator, char *func_name, tree_node_t *node) {
    if (!func_name || !node) return;

    char *suffix = func_name + 4; // Skip "Ifj."

    // Define expected types
    // 0: string, 1: int, 2: float
    int expected_types[3] = {-1,-1,-1}; 
    int expected_count = 0;

    if (strcmp(suffix, "length") == 0 || strcmp(suffix, "ord") == 0) {
        expected_types[0] = 0; // string
        expected_count = 1;
    } else if (strcmp(suffix, "chr") == 0) {
        expected_types[0] = 1; // int
        expected_count = 1;
    } else if (strcmp(suffix, "floor") == 0) {
        expected_types[0] = 2; // float
        expected_count = 1;
    } else if (strcmp(suffix, "substring") == 0) {
        expected_types[0] = 0; // string
        expected_types[1] = 1; // int
        expected_types[2] = 1; // int
        expected_count = 3;
    } else if (strcmp(suffix, "strcmp") == 0) {
        expected_types[0] = 0; // string
        expected_types[1] = 0; // string
        expected_count = 2;
    } else {
        return; // Skip write, read, etc.
    }

    int arg_index = 0; // index of the argument
    for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        
        // Skip function name and parentheses/commas
        if (child->type == NODE_T_TERMINAL && child->token &&
            (child->token->token_type == TOKEN_T_KEYWORD || // Ifj
             strcmp(child->token->token_lexeme, "(") == 0 ||
             strcmp(child->token->token_lexeme, ")") == 0 ||
             strcmp(child->token->token_lexeme, ",") == 0 ||
             (child->token->token_type == TOKEN_T_IDENTIFIER && strstr(func_name, child->token->token_lexeme)))) {
            continue;
        }
        if (arg_index >= expected_count) {
             // Too many arguments
             generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
             return;
             continue; 
        }

        int expected_type = expected_types[arg_index];
        
        // Check literal types
        Token *token_to_check = NULL;
        tree_node_t *current = child;
        
        // find the terminal node
        while (current && current->type == NODE_T_NONTERMINAL) {
            if (current->children_count > 0) {
                tree_node_t *next = NULL;
                for(int k=0; k<current->children_count; k++) {
                    tree_node_t *c = current->children[k];
                    if (c->token && (c->token->token_type == TOKEN_T_NUM || 
                                     c->token->token_type == TOKEN_T_STRING || 
                                     c->token->token_type == TOKEN_T_IDENTIFIER ||
                                     c->token->token_type == TOKEN_T_GLOBAL_VAR)) {
                        next = c;
                        break;
                    }
                    if (c->type == NODE_T_NONTERMINAL) {
                        next = c;
                        break; 
                    }
                }
                current = next;
            } else {
                current = NULL;
            }
        }
        
        if (current && current->token) {
            token_to_check = current->token;
        }

        if (token_to_check) {
            if (expected_type == 0) { // Expect string
                if (token_to_check->token_type == TOKEN_T_NUM) { // int or float
                     generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
                     return;
                }
            } else if (expected_type == 1) { // Expect int
                if (token_to_check->token_type == TOKEN_T_STRING) {
                     generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
                     return;
                }
                if (token_to_check->token_type == TOKEN_T_NUM) {
                    // Check if it looks like a float
                    if (strchr(token_to_check->token_lexeme, '.') || strchr(token_to_check->token_lexeme, 'e') || strchr(token_to_check->token_lexeme, 'E')) {
                         generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
                         return;
                    }
                }
            } else if (expected_type == 2) { // Expect float
                if (token_to_check->token_type == TOKEN_T_STRING) {
                     generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
                     return;
                }
                if (token_to_check->token_type == TOKEN_T_NUM) {
                    // Check if it looks like an int (no dot or exponent)
                    if (!strchr(token_to_check->token_lexeme, '.') && !strchr(token_to_check->token_lexeme, 'e') && !strchr(token_to_check->token_lexeme, 'E')) {
                         // Strict check: Int passed where float expected
                         generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
                         return;
                    }
                }
            }
        }
        
        // Check variable types from symbol table
        if (child->type == NODE_T_TERMINAL && child->token && 
            (child->token->token_type == TOKEN_T_IDENTIFIER || child->token->token_type == TOKEN_T_GLOBAL_VAR)) {
            Symbol *sym = search_table(child->token, generator->symtable);
            if (sym) {
                if (expected_type == 0) { // Expect string
                    if (sym->sym_variable_type == VAR_T_NUM) { // Assuming VAR_T_NUM covers both int and float
                         generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
                         return;
                    }
                } else if (expected_type == 1) { // Expect int
                    if (sym->sym_variable_type == VAR_T_STRING) {
                         generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
                         return;
                    }
                } else if (expected_type == 2) { // Expect float
                    if (sym->sym_variable_type == VAR_T_STRING) {
                         generator->error = ERR_T_SEMANTIC_ERR_BUILTIN_FN_BAD_PARAM;
                         return;
                    }
                }
            }
        }

        arg_index++;
    }
}

// Generate function call
void generate_function_call(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  tree_node_t *func_name_node = NULL;
  char *func_name = NULL;
  char full_func_name[256] = {0};
  int has_ifj_prefix = 0;

  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
    if (child->type == NODE_T_TERMINAL && child->token &&
        child->token->token_lexeme) {
      char *lexeme = child->token->token_lexeme;
      if (child->token->token_type == TOKEN_T_IDENTIFIER) {
        func_name_node = child;
        func_name = lexeme;
      } else if (child->token->token_type == TOKEN_T_KEYWORD &&
                 strcmp(lexeme, "Ifj") == 0) {
        has_ifj_prefix = 1;
      }
    }
  }

  if (func_name && has_ifj_prefix) {
    snprintf(full_func_name, sizeof(full_func_name), "Ifj.%s", func_name);
    func_name = full_func_name;
  } else if (func_name) {
    // Check if it's a built-in function without Ifj prefix
    if (strcmp(func_name, "write") == 0 ||
        strcmp(func_name, "read_num") == 0 ||
        strcmp(func_name, "read_str") == 0 ||
        strcmp(func_name, "length") == 0 ||
        strcmp(func_name, "floor") == 0 ||
        strcmp(func_name, "str") == 0 ||
        strcmp(func_name, "ord") == 0 ||
        strcmp(func_name, "substring") == 0 ||
        strcmp(func_name, "strcmp") == 0 ||
        strcmp(func_name, "chr") == 0) {
        snprintf(full_func_name, sizeof(full_func_name), "Ifj.%s", func_name);
        func_name = full_func_name;
    }
  }

  if (!func_name) {
    for (int i = 0; i < node->children_count; i++) {
      tree_node_t *child = node->children[i];
      if (child->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN) {
        generator_generate(generator, child);
        return;
      }
    }
    return;
  }

  // Check for built-in functions
  if (strncmp(func_name, "Ifj.", 4) == 0) {
    
    check_builtin_params(generator, func_name, node);
    if (generator->error != 0) return;

    char *suffix = func_name + 4;

    if (strcmp(suffix, "write") == 0) {
      // WRITE - iterate over arguments
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child == func_name_node)
          continue;
        // skip tokens like (, ), Ifj	
        if (child->type == NODE_T_TERMINAL && child->token &&
            (strcmp(child->token->token_lexeme, "(") == 0 ||
             strcmp(child->token->token_lexeme, ")") == 0 ||
             strcmp(child->token->token_lexeme, "Ifj") == 0))
          continue;

        // check if the child is an expression, function parameter, expression or function call, or a terminal
        if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
            child->nonterm_type == NONTERMINAL_T_FUN_PARAM ||
            child->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN ||
            (child->type == NODE_T_TERMINAL && child->token &&
             (child->token->token_type == TOKEN_T_NUM ||
              child->token->token_type == TOKEN_T_STRING ||
              child->token->token_type == TOKEN_T_IDENTIFIER ||
              child->token->token_type == TOKEN_T_GLOBAL_VAR))) {

          generate_expression(generator, child);
          char *temp = get_temp_var(generator);
          generator_emit(generator, "DEFVAR %s", temp);
          generator_emit(generator, "POPS %s", temp);
          
          convert_float_to_int_before_write(generator, temp);
          
          generator_emit(generator, "WRITE %s", temp);
          free(temp);
        }
      }
      generator_emit(generator, "PUSHS nil@nil"); // Return nil
      return;
    } else if (strcmp(suffix, "read_num") == 0) {
      char *temp = get_temp_var(generator);
      generator_emit(generator, "DEFVAR %s", temp);
      generator_emit(generator, "READ %s float", temp);
      generator_emit(generator, "PUSHS %s", temp);
      free(temp);
      return;
    } else if (strcmp(suffix, "read_str") == 0) {
      char *temp = get_temp_var(generator);
      generator_emit(generator, "DEFVAR %s", temp);
      generator_emit(generator, "READ %s string", temp);
      generator_emit(generator, "PUSHS %s", temp);
      free(temp);
      return;
    } else if (strcmp(suffix, "length") == 0) {
      // STRLEN
      // Expect 1 argument
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child == func_name_node)
          continue;
        if (child->type == NODE_T_TERMINAL)
          continue; // Skip tokens
        if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
            child->nonterm_type == NONTERMINAL_T_FUN_PARAM) {
          generate_expression(generator, child);
          char *temp_s = get_temp_var(generator);
          char *temp_len = get_temp_var(generator);
          generator_emit(generator, "DEFVAR %s", temp_s);
          generator_emit(generator, "POPS %s", temp_s);
          generator_emit(generator, "DEFVAR %s", temp_len);
          generator_emit(generator, "STRLEN %s %s", temp_len, temp_s);
          generator_emit(generator, "PUSHS %s", temp_len);
          generator_emit(generator, "INT2FLOATS");
          free(temp_s);
          free(temp_len);
          return;
        }
      }
      return;
    } else if (strcmp(suffix, "floor") == 0) {
      // FLOAT2INT
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child == func_name_node)
          continue;
        if (child->type == NODE_T_TERMINAL)
          continue;
        if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
            child->nonterm_type == NONTERMINAL_T_FUN_PARAM) {
          generate_expression(generator, child);
          generator_emit(generator, "FLOAT2INTS");
          return;
        }
      }
      return;
    } else if (strcmp(suffix, "substring") == 0) {
      // SUBSTRING
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child == func_name_node)
          continue;
        if (child->type == NODE_T_TERMINAL)
          continue;
        if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
            child->nonterm_type == NONTERMINAL_T_FUN_PARAM) {
          generate_expression(generator, child);
        }
      }
      
      // Inline implementation of substring
      char *s = get_temp_var(generator);
      char *p1 = get_temp_var(generator);
      char *p2 = get_temp_var(generator);
      char *res = get_temp_var(generator);
      char *len = get_temp_var(generator);
      char *cond = get_temp_var(generator);
      char *char_val = get_temp_var(generator);
      
      int label_idx = generator->label_counter++;
      
      generator_emit(generator, "DEFVAR %s", s);
      generator_emit(generator, "DEFVAR %s", p1);
      generator_emit(generator, "DEFVAR %s", p2);
      generator_emit(generator, "DEFVAR %s", res);
      generator_emit(generator, "DEFVAR %s", len);
      generator_emit(generator, "DEFVAR %s", cond);
      generator_emit(generator, "DEFVAR %s", char_val);

      // Pop arguments: s, p1, p2 (top)
      generator_emit(generator, "POPS %s", p2);
      generator_emit(generator, "POPS %s", p1);
      generator_emit(generator, "POPS %s", s);

      // Check p1 < 0
      generator_emit(generator, "LT %s %s int@0", cond, p1);
      generator_emit(generator, "JUMPIFEQ substring_nil_%d %s bool@true", label_idx, cond);
      
      // Check p2 < 0
      generator_emit(generator, "LT %s %s int@0", cond, p2);
      generator_emit(generator, "JUMPIFEQ substring_nil_%d %s bool@true", label_idx, cond);

      // Check p1 > p2
      generator_emit(generator, "GT %s %s %s", cond, p1, p2);
      generator_emit(generator, "JUMPIFEQ substring_nil_%d %s bool@true", label_idx, cond);

      // len = length(s)
      generator_emit(generator, "STRLEN %s %s", len, s);

      // Check p1 >= len
      generator_emit(generator, "LT %s %s %s", cond, p1, len);
      generator_emit(generator, "JUMPIFEQ substring_nil_%d %s bool@false", label_idx, cond);

      // Check p2 >= len
      generator_emit(generator, "LT %s %s %s", cond, p2, len);
      generator_emit(generator, "JUMPIFEQ substring_nil_%d %s bool@false", label_idx, cond);

      // Initialize res = ""
      generator_emit(generator, "MOVE %s string@", res);
      
      // Loop
      generator_emit(generator, "LABEL substring_loop_%d", label_idx);
      
      // if p1 > p2 break
      generator_emit(generator, "GT %s %s %s", cond, p1, p2);
      generator_emit(generator, "JUMPIFEQ substring_end_%d %s bool@true", label_idx, cond);

      // char = s[p1]
      generator_emit(generator, "GETCHAR %s %s %s", char_val, s, p1);
      // res = res + char
      generator_emit(generator, "CONCAT %s %s %s", res, res, char_val);
      // p1++
      generator_emit(generator, "ADD %s %s int@1", p1, p1);
      generator_emit(generator, "JUMP substring_loop_%d", label_idx);

      generator_emit(generator, "LABEL substring_end_%d", label_idx);
      generator_emit(generator, "PUSHS %s", res);
      generator_emit(generator, "JUMP substring_done_%d", label_idx);

      generator_emit(generator, "LABEL substring_nil_%d", label_idx);
      generator_emit(generator, "PUSHS nil@nil");

      generator_emit(generator, "LABEL substring_done_%d", label_idx);

      free(s); free(p1); free(p2); free(res); free(len); free(cond); free(char_val);
      return;
    } else if (strcmp(suffix, "str") == 0) {
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child == func_name_node)
          continue;
        if (child->type == NODE_T_TERMINAL)
          continue;
        if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
            child->nonterm_type == NONTERMINAL_T_FUN_PARAM) {
          generate_expression(generator, child);
          if(generator->is_float){
            generator_emit(generator, "FLOAT2STRS");
            generator->is_float = false;
          } else {
            generator_emit(generator, "INT2STRS");
          }
          return;
        }
      }
      return;
      // STRCMP
    } else if (strcmp(suffix, "strcmp") == 0) {
      generate_strcmp_comparison(generator, node);
      return;
      // ORD
    } else if (strcmp(suffix, "ord") == 0){
      // Expect 2 arguments: string s, int index
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child == func_name_node)
          continue;
        if (child->type == NODE_T_TERMINAL)
          continue;
        if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
            child->nonterm_type == NONTERMINAL_T_FUN_PARAM) {
          generate_expression(generator, child);
        }
      }

      char *s = get_temp_var(generator);
      char *idx = get_temp_var(generator);
      char *len = get_temp_var(generator);
      char *cond = get_temp_var(generator);
      char *res = get_temp_var(generator);
      
      int label_idx = generator->label_counter++;

      generator_emit(generator, "DEFVAR %s", s);
      generator_emit(generator, "DEFVAR %s", idx);
      generator_emit(generator, "DEFVAR %s", len);
      generator_emit(generator, "DEFVAR %s", cond);
      generator_emit(generator, "DEFVAR %s", res);

      // Pop arguments: index (top), s
      generator_emit(generator, "POPS %s", idx);
      generator_emit(generator, "POPS %s", s);

      // Check index < 0
      generator_emit(generator, "LT %s %s int@0", cond, idx);
      generator_emit(generator, "JUMPIFEQ ord_zero_%d %s bool@true", label_idx, cond);

      // Get length
      generator_emit(generator, "STRLEN %s %s", len, s);

      // Check index >= length
      generator_emit(generator, "LT %s %s %s", cond, idx, len);
      // If index < length is false (i.e. index >= length), jump to zero
      generator_emit(generator, "JUMPIFEQ ord_zero_%d %s bool@false", label_idx, cond);

      // Valid index: use STRI2INT
      generator_emit(generator, "STRI2INT %s %s %s", res, s, idx);
      generator_emit(generator, "PUSHS %s", res);
      generator_emit(generator, "INT2FLOATS");
      generator_emit(generator, "JUMP ord_done_%d", label_idx);

      // Return 0 case
      generator_emit(generator, "LABEL ord_zero_%d", label_idx);
      generator_emit(generator, "PUSHS int@0");
      generator_emit(generator, "INT2FLOATS");

      generator_emit(generator, "LABEL ord_done_%d", label_idx);

      free(s); free(idx); free(len); free(cond); free(res);
      return;
    } else if (strcmp(suffix, "chr") == 0) {
      // Expect 1 argument: int num
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child == func_name_node)
          continue;
        if (child->type == NODE_T_TERMINAL)
          continue;
        if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
            child->nonterm_type == NONTERMINAL_T_FUN_PARAM) {
          generate_expression(generator, child);
        }
      }

      char *num = get_temp_var(generator);
      char *res = get_temp_var(generator);
      
      generator_emit(generator, "DEFVAR %s", num);
      generator_emit(generator, "DEFVAR %s", res);

      // Pop argument
      generator_emit(generator, "POPS %s", num);

      // Convert int to char
      generator_emit(generator, "INT2CHAR %s %s", res, num);
      generator_emit(generator, "PUSHS %s", res);

      free(num); free(res);
      return;
    }
  }

  // Normal function call
  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
    if (child == func_name_node)
      continue;
    if (child->type == NODE_T_TERMINAL && child->token) {
      if (strcmp(child->token->token_lexeme, "(") == 0 ||
          strcmp(child->token->token_lexeme, ")") == 0 ||
          strcmp(child->token->token_lexeme, "Ifj") == 0) {
        continue;
      }
    }
    if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
        child->nonterm_type == NONTERMINAL_T_FUN_PARAM ||
        (child->type == NODE_T_TERMINAL && child->token &&
         (child->token->token_type == TOKEN_T_NUM ||
          child->token->token_type == TOKEN_T_STRING ||
          child->token->token_type == TOKEN_T_IDENTIFIER ||
          child->token->token_type == TOKEN_T_GLOBAL_VAR))) {
      generate_expression(generator, child);
    }
  }

  generator->is_called = true;
  
  // Search for the function symbol specifically
  Symbol *sym = NULL;
  for (int i = 0; i < generator->symtable->symtable_size; i++) {
      Symbol *s = generator->symtable->symtable_rows[i].symbol;
      if (s && s->sym_lexeme && strcmp(s->sym_lexeme, func_name) == 0) {
          if (s->sym_identif_type == IDENTIF_T_FUNCTION) {
              sym = s;
              break;
          }
      }
  }

  if (sym && sym->sym_identif_type == IDENTIF_T_FUNCTION && sym->sym_identif_declaration_count > 1) {
      // Resolve overload based on argument count
      int arg_count = 0;
      for (int i = 0; i < node->children_count; i++) {
        tree_node_t *child = node->children[i];
        if (child == func_name_node) continue;
        if (child->type == NODE_T_TERMINAL && child->token) {
             if (strcmp(child->token->token_lexeme, "(") == 0 ||
                 strcmp(child->token->token_lexeme, ")") == 0 ||
                 strcmp(child->token->token_lexeme, "Ifj") == 0) {
                 continue;
             }
        }
        // Count expressions/params
        if (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
            child->nonterm_type == NONTERMINAL_T_FUN_PARAM ||
            (child->type == NODE_T_TERMINAL && child->token &&
             (child->token->token_type == TOKEN_T_NUM ||
              child->token->token_type == TOKEN_T_STRING ||
              child->token->token_type == TOKEN_T_IDENTIFIER ||
              child->token->token_type == TOKEN_T_GLOBAL_VAR))) {
            arg_count++;
        }
      }

      int overload_index = -1;
      for (int i = 0; i < sym->sym_identif_declaration_count; i++) {
          if (sym->sym_function_number_of_params[i] == arg_count) {
              overload_index = i;
              break;
          }
      }
      
      if (overload_index != -1) {
          generator_emit(generator, "CALL %s$%d", func_name, overload_index);
      } else {
          generator_emit(generator, "CALL %s", func_name);
      }
  } else {
      generator_emit(generator, "CALL %s", func_name);
  }
  
  generator->is_called = false;
}

// Generate if statement
void generate_if(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  char *else_label = get_next_label(generator);
  char *end_label = get_next_label(generator);

  tree_node_t *predicate_node = NULL;
  tree_node_t *then_block = NULL;
  tree_node_t *else_block = NULL;

  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
    if (child->nonterm_type == NONTERMINAL_T_PREDICATE) {
      predicate_node = child;
    } else if (child->nonterm_type == NONTERMINAL_T_CODE_BLOCK) {
      if (!then_block) {
        then_block = child;
      } else {
        else_block = child;
      }
    }
  }

  if (predicate_node) {
    generator_generate(generator, predicate_node);

    int has_neq_operator = 0; // check if the predicate contains a != operator
    if (predicate_node->children_count > 0) {
      tree_node_t *expr = predicate_node->children[0];
      if (expr && expr->token && expr->token->token_type == TOKEN_T_OPERATOR &&
          strcmp(expr->token->token_lexeme, "!=") == 0) {
        has_neq_operator = 1;
      }
    }

    if (has_neq_operator) {
      generator_emit(generator, "JUMPIFNEQS %s", else_label);
    } else {
      int has_operator = 0;
      if (predicate_node->children_count > 0) {
        tree_node_t *expr = predicate_node->children[0];
        if (expr->token && expr->token->token_type == TOKEN_T_OPERATOR) {
          has_operator = 1;
        }
      }
      
      char *cond_res = get_temp_var(generator);
      generator_emit(generator, "DEFVAR %s", cond_res);
      generator_emit(generator, "POPS %s", cond_res);

      // Check if nil
      generator_emit(generator, "PUSHS %s", cond_res);
      generator_emit(generator, "PUSHS nil@nil");
      generator_emit(generator, "JUMPIFEQS %s", else_label);

      // Only check false if there's an operator (expression returns boolean)
      // Single variables don't need false check - not nil = truthy
      if (has_operator) {
        generator_emit(generator, "PUSHS %s", cond_res);
        generator_emit(generator, "PUSHS bool@false");
        generator_emit(generator, "JUMPIFEQS %s", else_label);
      }
      
      free(cond_res);
    }
  }

  if (then_block) {
    generate_code_block(generator, then_block);
  }

  generator_emit(generator, "JUMP %s", end_label);

  generator_emit(generator, "LABEL %s", else_label);

  if (else_block) {
    generate_code_block(generator, else_block);
  }

  generator_emit(generator, "LABEL %s", end_label);

  free(else_label);
  free(end_label);
}

// collect local variable declarations in subtree
void collect_local_vars_in_subtree(tree_node_t *node, Token ***tokens, int *count, Symtable *symtable) {
  if (!node) return;

  // Check if this is a variable declaration
  if (node->rule == GR_DECLARATION && node->children_count > 0) {
    tree_node_t *id_node = node->children[0];
    if (id_node && id_node->token && id_node->token->token_type == TOKEN_T_IDENTIFIER) {
      char *var_name = id_node->token->token_lexeme;
      
      // skip temporary variables
      if (strncmp(var_name, "tmp_", 4) == 0) {
        return;
      }
      
      // check if it's a local variable (not global)
      Symbol *sym = search_table(id_node->token, symtable);
      if (sym && !sym->is_global) {
        // check if already in the list
        int found = 0;
        for (int i = 0; i < *count; i++) {
          if ((*tokens)[i] && strcmp((*tokens)[i]->token_lexeme, var_name) == 0) {
            found = 1;
            break;
          }
        }
        
        // add to list if not found
        if (!found) {
          *tokens = realloc(*tokens, (*count + 1) * sizeof(Token*));
          if (*tokens) {
            (*tokens)[*count] = id_node->token;
            (*count)++;
          }
        }
      }
    }
  }

  // recursively process children
  for (int i = 0; i < node->children_count; i++) {
    collect_local_vars_in_subtree(node->children[i], tokens, count, symtable);
  }
}

// generate while loop
void generate_while(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  char *loop_label = get_next_label(generator);
  char *end_label = get_next_label(generator);

  // Find the body block first to collect variables
  tree_node_t *predicate_node = NULL;
  tree_node_t *body_block = NULL;

  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
    if (child->nonterm_type == NONTERMINAL_T_PREDICATE) {
      predicate_node = child;
    } else if (child->nonterm_type == NONTERMINAL_T_CODE_BLOCK) {
      body_block = child;
    }
  }

  // collect all local variable declarations in the loop body
  Token **local_var_tokens = NULL;
  int var_count = 0;
  if (body_block) {
    collect_local_vars_in_subtree(body_block, &local_var_tokens, &var_count, generator->symtable);
  }

  // generate DEFVAR for all collected variables BEFORE the loop label
  for (int i = 0; i < var_count; i++) {
    if (local_var_tokens[i]) {
      int scope = get_scope_suffix(generator, local_var_tokens[i]);
      generator_emit(generator, "DEFVAR LF@%s$%d", local_var_tokens[i]->token_lexeme, scope);
    }
  }

  // generate loop label
  generator_emit(generator, "LABEL %s", loop_label);
  
  // clear TF at the start of each iteration
  generator_emit(generator, "CREATEFRAME");
  generator->tf_created = true;

  if (predicate_node) {
    generator_generate(generator, predicate_node);

    int has_neq_operator = 0;
    if (predicate_node->children_count > 0) {
      tree_node_t *expr = predicate_node->children[0];
      if (expr && expr->token && expr->token->token_type == TOKEN_T_OPERATOR &&
          strcmp(expr->token->token_lexeme, "!=") == 0) {
        has_neq_operator = 1;
      }
    }

    if (has_neq_operator) {
      generator_emit(generator, "JUMPIFNEQS %s", end_label);
    } else {
      generator_emit(generator, "PUSHS bool@false");
      generator_emit(generator, "JUMPIFEQS %s", end_label);
    }
  }

  if (body_block) {
    generator->in_while_loop = 1;  // Set flag before generating body
    generate_code_block(generator, body_block);
    generator->in_while_loop = 0;  // Reset flag after body
  }

  generator_emit(generator, "JUMP %s", loop_label);

  // generate end label
  generator_emit(generator, "LABEL %s", end_label);

  // free allocated memory
  if (local_var_tokens) {
    free(local_var_tokens);
  }

  free(loop_label);
  free(end_label);
}

// generate return statement
void generate_return(Generator *generator, tree_node_t *node) {
  if (!node)
    return;

  tree_node_t *expr_node = NULL;
  for (int i = 0; i < node->children_count; i++) {
    tree_node_t *child = node->children[i];
    if (child->token && (child->token->token_type == TOKEN_T_NUM ||
                         child->token->token_type == TOKEN_T_STRING ||
                         child->token->token_type == TOKEN_T_IDENTIFIER ||
                         child->token->token_type == TOKEN_T_GLOBAL_VAR ||
                         child->token->token_type == TOKEN_T_KEYWORD)) {
      expr_node = child;
      break;
    } else if (child->type == NODE_T_NONTERMINAL &&
        (child->nonterm_type == NONTERMINAL_T_EXPRESSION ||
         child->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN)) {
      expr_node = child;
      break;
    } else if (child->type == NODE_T_TERMINAL) {
      expr_node = child;
      break;
    } else if (child->token && child->token->token_type == TOKEN_T_OPERATOR && child->children_count > 0) {
      expr_node = child;
      break;
    }
  }

  if (expr_node) { // if the expression node is found
    // check if the expression node is a terminal
    if (expr_node->token && (expr_node->token->token_type == TOKEN_T_STRING ||
                             expr_node->token->token_type == TOKEN_T_NUM ||
                             expr_node->token->token_type == TOKEN_T_IDENTIFIER ||
                             expr_node->token->token_type == TOKEN_T_GLOBAL_VAR ||
                             expr_node->token->token_type == TOKEN_T_KEYWORD)) {
      generate_terminal(generator, expr_node);
    } else if (expr_node->token && expr_node->token->token_type == TOKEN_T_OPERATOR && expr_node->children_count > 0) {
      generate_expression(generator, expr_node);
    } else if (expr_node->type == NODE_T_TERMINAL) {
      generate_terminal(generator, expr_node);
    } else if (expr_node->nonterm_type == NONTERMINAL_T_EXPRESSION_OR_FN) {
      generator_generate(generator, expr_node);
    } else {
      generate_expression(generator, expr_node);
    }
  } else {
    for (int i = 0; i < node->children_count; i++) {
      tree_node_t *child = node->children[i];
      if (child->type == NODE_T_TERMINAL && child->token && 
          strcmp(child->token->token_lexeme, "return") == 0) {
        continue;
      }
      if (child->token || child->children_count > 0) {
        if (child->token && child->token->token_type == TOKEN_T_OPERATOR) {
          expr_node = child;
          generate_expression(generator, expr_node);
          break;
        }
        if (child->children_count > 0) {
          for (int j = 0; j < child->children_count; j++) {
            tree_node_t *grandchild = child->children[j];
            if (grandchild->token && grandchild->token->token_type == TOKEN_T_OPERATOR) {
              expr_node = grandchild;
              generate_expression(generator, expr_node);
              goto found_expr;
            }
          }
        }
        expr_node = child;
        generate_expression(generator, expr_node);
        break;
      }
    }
    found_expr:;
  }

  // return instruction
  if (!generator->is_called) {
    if (!expr_node) {
        generator_emit(generator, "PUSHS nil@nil"); // default return value if no expression
    }
    generator_emit(generator, "POPFRAME");
    generator_emit(generator, "RETURN");
    generator->has_return = true;  // mark that return was generated
  }
}

