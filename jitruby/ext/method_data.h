#ifndef METHOD_DATA_H
#define METHOD_DATA_H

#include <ruby.h>

void define_method_with_data(
    VALUE klass, ID id, VALUE (*cfunc)(ANYARGS), int arity, VALUE data);

VALUE get_method_data();

#endif // METHOD_DATA_H
