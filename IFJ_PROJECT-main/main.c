#include "lexer.h"
#include "symtable.h"
#include "syntactic.h"
#include "semantic.h"
#include "generator.h"
#include "utils.h"



int main() {
    Symtable *symtable;
    symtable = init_sym_table();

    // the lexer enters the initial state of the FSM
    Lexer *lexer = init_lexer(symtable);
    lexer_start(lexer);

    if(lexer->error != 0){
        return lexer->error;
    }
    
    // Syntactic analyser
    Syntactic *syntactic = init_syntactic(symtable);
    syntactic_start(syntactic, lexer);

    print_symtable(syntactic->symtable);

    int main_declared = check_main_function(syntactic->symtable);
    if(main_declared != 0){
        return main_declared;
    } 
    
    if(syntactic->error != 0){
         return syntactic->error;
    }

    Semantic *semantic = init_semantic(syntactic->symtable);
    traverse_tree(syntactic->tree->children[0], syntactic->symtable, semantic);
    if(semantic->error != 0){
         return semantic->error;
    } 

    Generator *generator = init_generator(symtable);
    if (!generator) {
        return ERR_T_MALLOC_ERR;
    }
    generate_global_vars(generator);
    
    int gen_error = generator_start(generator, syntactic->tree);
    if (gen_error != 0) {
        generator_free(generator);
        return gen_error;
    }
    
    generator_free(generator);
    

    return 0;
}
