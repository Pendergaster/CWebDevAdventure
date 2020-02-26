#include <stdio.h>
#include <libtcc.h>
#include <malloc.h>

char* program = "#include <stdio.h> \n int main() { printf(\"hello world\n\"); }";

static void compile_and_run_string(const char* code) {

    TCCState* state = tcc_new();
    if (!state) {
        printf("could not create state\n");
        return; 
    }
    // tcc_set_lib_path(state, "./");
    // tcc_add_include_path(state, "./");
    tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
    tcc_set_options(state, "-m64 -std=c99 -bench");

    tcc_compile_string(state, code);

    int size = tcc_relocate(state, 0);
    printf("code size: %i kb, %i b\n", size / 1024, size);
    void* mem = malloc(size);
    tcc_relocate(state, mem);

    void (*main)(int argc, char** argv) = tcc_get_symbol(state, "main");
    if (!main) {
        printf("no main found\n");
    } else {
        main(0, 0);
    }
    
    tcc_delete(state);
    free(mem);
}

main(int argc, char** argv) {

    if (argc > 1) {
       printf("compile\n");
       compile_and_run_string(argv[1]);
       printf("succ\n");
    }
    else {
        compile_and_run_string(program);
    }
    return 0;
}


