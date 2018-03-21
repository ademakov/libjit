/*
 * Written by David Meyer <pdox@fb.com>
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <jit/jit.h>
#include <jit/jit-dump.h>

#define CONST(n)     jit_value_create_nint_constant(func, jit_type_int, (n))
#define PARAM(i)     jit_value_get_param(func, (i))
#define ADD(x, y)    jit_insn_add(func, (x), (y))
#define STORE(d, s)  jit_insn_store(func, (d), (s))
#define LABEL(p)     jit_insn_label(func, (p))
#define BRANCH(p)    jit_insn_branch(func, (p))

typedef int EntrySig(int, int, int);

jit_label_t make_dummy_table(jit_function_t func, size_t nlabels) {
    jit_label_t dummy = jit_label_undefined;
    size_t i;
    LABEL(&dummy);
    jit_label_t *labels = (jit_label_t*)malloc(sizeof(jit_label_t) * nlabels);
    for (i = 0; i < nlabels; i++) {
        labels[i] = jit_label_undefined;
        LABEL(&labels[i]);
        jit_insn_return(func, CONST(0));
    }
    jit_insn_jump_table(func, PARAM(0), labels, nlabels);
    free(labels);
    return dummy;
}

void trial(size_t nlabels) {
    jit_context_t context = jit_context_create();
    jit_context_build_start(context);
    jit_type_t args[] = {jit_type_int, jit_type_int, jit_type_int};
    jit_type_t sig = jit_type_create_signature(jit_abi_cdecl, jit_type_int, args, 3, 1);
    jit_function_t func = jit_function_create(context, sig);

    /* Define labels upfront */
    jit_label_t bottom = jit_label_undefined;
    jit_label_t back_to_top = jit_label_undefined;
    jit_label_t finale = jit_label_undefined;

    /* The goal is to make 'v' a global register */
    jit_value_t v = jit_value_create(func, jit_type_int);
    BRANCH(&bottom);
    LABEL(&back_to_top);
    STORE(v, ADD(v, CONST(0)));
    BRANCH(&finale);

    /* Create a large dummy jumptable to trigger an out-of-memory
       and codegen restart. This code never actually runs. */
    jit_label_t dummy = make_dummy_table(func, nlabels);

    LABEL(&bottom);
    STORE(v, PARAM(0));
    STORE(v, ADD(v, PARAM(1)));
    STORE(v, ADD(v, PARAM(2)));
    BRANCH(&back_to_top);

    LABEL(&finale);
    /* Fake branch to keep the dummy code alive. Won't be taken. */
    jit_insn_branch_if(func, jit_insn_eq(func, PARAM(0), CONST(100)), &dummy);
    jit_insn_return(func, v);

    jit_function_set_optimization_level(func, 0);
    int ok = jit_function_compile(func);
    assert(ok);
    void *entry = jit_function_to_closure(func);
    int result = ((EntrySig*)entry)(1,2,3);
    if (result != 6) {
        printf("CORRUPTION! Return value is %d != 6\n", result);
        printf("Dumping object to /tmp/minimal.dump\n");
        FILE *fp = fopen("/tmp/minimal.dump", "w");
        jit_dump_function(fp, func, "minimal");
        fclose(fp);
        exit(1);
    }
    jit_context_destroy(context);
}

int main() {
    size_t n;

    /* Expand the jumptable until the miscompilation occurs */
    for (n = 1000; n < 100000; n += 100) {
        printf("Trying n = %d\n", (int)n);
        trial(n);
    }
    return 0;
}
