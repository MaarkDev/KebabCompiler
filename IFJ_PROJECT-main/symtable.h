#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "symbol.h"

typedef struct symtable_row {
    int key;
    Symbol *symbol;
} Symtable_row;

typedef struct symtable {
    int number_of_entries;
    int symtable_size;
    Symtable_row *symtable_rows;
} Symtable;



Symtable *init_sym_table();
int symtable_key_gen(char *seed);
int symtable_index_gen(Symtable *symtable, int key);
void insert_into_symtable(Symtable *symtable, Symbol *symbol);
void print_symtable(Symtable *symtable);
void print_symtable_lexemes(Symtable *symtable);
Symbol *search_table(Token *token, Symtable *symtable);
void symtable_add_declaration_info(Symbol *symbol, int line, int col, int scope);
void add_function_param(Symbol *symbol, Token *token);
int identif_declared_at_least_once(Token *token, Symtable *symtable, bool is_param);
Symbol *search_table_for_setter_or_getter(Token *token, Symtable *symtable);
Symbol *search_table_in_scope_hierarchy(Token *token, Symtable *symtable);
void copy_symbol_usage_info(Symbol *dest, Symbol *source);
#endif