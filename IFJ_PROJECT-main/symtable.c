#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "symtable.h"
#include "symbol.h"

Symtable *init_sym_table(){
    Symtable *symtable = malloc(sizeof(Symtable));
    if(symtable == NULL) return NULL;

    symtable->number_of_entries = 0;
    symtable->symtable_size = 100;

    symtable->symtable_rows = malloc(sizeof(Symtable_row) * symtable->symtable_size);
    if (symtable->symtable_rows == NULL) {
        free(symtable);
        return NULL;
    }

    for (int i = 0; i < symtable->symtable_size; i++) {
        symtable->symtable_rows[i].key = 0;
        symtable->symtable_rows[i].symbol = NULL;
    }

    return symtable;
}

int symtable_key_gen(char *seed){
    unsigned long hash = 5381;
    int c;

    while ((c = *seed++)) {
        hash = ((hash << 5) + hash) + c; 
    }

    return (int)(hash & 0x7FFFFFFF);
}

int symtable_index_gen(Symtable *symtable, int key){
    return key % symtable->symtable_size;
}

void insert_into_symtable(Symtable *symtable, Symbol *symbol){
    int sym_key = symtable_key_gen(symbol->sym_lexeme);
    int index = symtable_index_gen(symtable, sym_key);

    // RESIZE
    if(symtable->number_of_entries > symtable->symtable_size/2){
        symtable->symtable_size = symtable->symtable_size * 2;
        
        symtable->symtable_rows = realloc(symtable->symtable_rows, symtable->symtable_size * sizeof(Symtable_row));
        for (int i = symtable->symtable_size/2; i < symtable->symtable_size; i++) {
            symtable->symtable_rows[i].key = 0;
            symtable->symtable_rows[i].symbol = NULL;
        }
    }

    // INSERTION LOGIC
    if(symtable->symtable_rows[index].symbol == NULL){ // we have an empty slot,
        symtable->symtable_rows[index].key = sym_key;
        symtable->symtable_rows[index].symbol = symbol;
        symtable->number_of_entries++;
    } // we have a duplicate in the scope
    else if(symtable->symtable_rows[index].symbol != NULL && 
        strcmp(symtable->symtable_rows[index].symbol->sym_lexeme, symbol->sym_lexeme) == 0 && 
        (symbol->sym_identif_used_at_scope_arr[0] == symtable->symtable_rows[index].symbol->sym_identif_used_at_scope_arr[0])
    ){

        add_symbol_occurence(symtable->symtable_rows[index].symbol, symbol->sym_identif_used_at_line_arr[0], symbol->sym_identif_used_at_col_arr[0], symbol->sym_identif_used_at_scope_arr[0]);
    
    }
    else { // linear probing - also checking for duplicates
        while(symtable->symtable_rows[index].symbol != NULL){
            index = (index + 1) % symtable->symtable_size;
            if(symtable->symtable_rows[index].symbol != NULL && 
                strcmp(symtable->symtable_rows[index].symbol->sym_lexeme, symbol->sym_lexeme) == 0 && 
                (symbol->sym_identif_used_at_scope_arr[0] == symtable->symtable_rows[index].symbol->sym_identif_used_at_scope_arr[0])
            ){
                add_symbol_occurence(symtable->symtable_rows[index].symbol, symbol->sym_identif_used_at_line_arr[0], symbol->sym_identif_used_at_col_arr[0], symbol->sym_identif_used_at_scope_arr[0]);
                return;
            }
        }
        symtable->symtable_rows[index].key = sym_key;
        symtable->symtable_rows[index].symbol = symbol;
        symtable->number_of_entries++;
    }

    

}

void print_symtable(Symtable *symtable) {
    if (!symtable) return;
    printf("============================================\n");
    printf("==== Symbol table (%d entries, size %d): ===\n", symtable->number_of_entries, symtable->symtable_size);
    printf("============================================\n");
    for (int i = 0; i < symtable->symtable_size; i++) {
        if (symtable->symtable_rows[i].symbol != NULL) {
            printf("-------- Index %d --------\n", i);
            print_symbol(symtable->symtable_rows[i].symbol);
        }
    }
}

void print_symtable_lexemes(Symtable *symtable) {
    if (!symtable) return;
    printf("============================================\n");
    printf("==== Symbol table (%d entries, size %d): ===\n", symtable->number_of_entries, symtable->symtable_size);
    printf("============================================\n");
    for (int i = 0; i < symtable->symtable_size; i++) {
        if (symtable->symtable_rows[i].symbol != NULL) {
            printf("%s\n", symtable->symtable_rows[i].symbol->sym_lexeme);
        }
    }
}

Symbol *search_table(Token *token, Symtable *symtable) {

    for (int i = 0; i < symtable->symtable_size; i++) {
        Symbol *sym = symtable->symtable_rows[i].symbol;
        if (!sym || !sym->sym_lexeme) continue;

        for(int j = 0; j < token->scope_count; j++){
            if (strcmp(sym->sym_lexeme, token->token_lexeme) == 0 && token->previous_scope_arr[j] == token->scope) {
                return sym;
            }
        }
    }

    return NULL;
}

Symbol *search_table_for_setter_or_getter(Token *token, Symtable *symtable) {

    for (int i = 0; i < symtable->symtable_size; i++) {
        Symbol *sym = symtable->symtable_rows[i].symbol;
        if (!sym || !sym->sym_lexeme) continue;

        if (strcmp(sym->sym_lexeme, token->token_lexeme) == 0 ) {
            return sym;
        }
    }

    return NULL;
}

int identif_declared_at_least_once(Token *token, Symtable *symtable, bool is_param){
    for (int i = 0; i < symtable->symtable_size; i++) {
        Symbol *sym = symtable->symtable_rows[i].symbol;
        if (!sym || !sym->sym_lexeme) continue;

        // lexeme matches
        if (strcmp(sym->sym_lexeme, token->token_lexeme) == 0) {
            if (is_param) {
                return 1;
            }

            // check if symbol was previously declared in previous scopes
            for(int j = token->scope_count - 1; j >= 0 ; j--){
                for(int k = 0; k < sym->sym_identif_declaration_count; k++){
                    if(token->previous_scope_arr[j] == sym->sym_identif_declared_at_scope_arr[k]){
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

void symtable_add_declaration_info(Symbol *symbol, int line, int col, int scope) {
    symbol->sym_identif_declaration_count++;

    symbol->sym_identif_declared_at_line_arr = realloc(
        symbol->sym_identif_declared_at_line_arr,
        sizeof(int) * symbol->sym_identif_declaration_count
    );

    symbol->sym_identif_declared_at_col_arr = realloc(
        symbol->sym_identif_declared_at_col_arr,
        sizeof(int) * symbol->sym_identif_declaration_count
    );

    symbol->sym_identif_declared_at_scope_arr = realloc(
        symbol->sym_identif_declared_at_scope_arr,
        sizeof(int) * symbol->sym_identif_declaration_count
    );

    int idx = symbol->sym_identif_declaration_count - 1;
    symbol->sym_identif_declared_at_line_arr[idx] = line;
    symbol->sym_identif_declared_at_col_arr[idx] = col;
    symbol->sym_identif_declared_at_scope_arr[idx] = scope;
}

Symbol *search_table_in_scope_hierarchy(Token *token, Symtable *symtable) {
    // First try exact scope match
    Symbol *result = search_table(token, symtable);
    if (result) return result;
    
    // If not found, search in all parent scopes for parameters
    for (int i = 0; i < symtable->symtable_size; i++) {
        Symbol *sym = symtable->symtable_rows[i].symbol;
        if (!sym || !sym->sym_lexeme) continue;
        
        if (strcmp(sym->sym_lexeme, token->token_lexeme) == 0 && sym->is_parameter) {
            // Check if this parameter's declaration scope is in the token's scope hierarchy
            for (int j = 0; j < token->scope_count; j++) {
                for (int k = 0; k < sym->sym_identif_declaration_count; k++) {
                    if (token->previous_scope_arr[j] == sym->sym_identif_declared_at_scope_arr[k]) {
                        return sym;
                    }
                }
            }
        }
    }
    
    return NULL;
}