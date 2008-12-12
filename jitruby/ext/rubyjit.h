#ifndef RUBYJIT_H
#define RUBYJIT_H

enum Ruby_Libjit_Tag
{
  RJT_OBJECT,
  RJT_ID,
  RJT_FUNCTION_PTR,
  RJT_RUBY_VARARG_SIGNATURE,
  RJT_VALUE_OBJECTS,
  RJT_FUNCTIONS,
  RJT_CONTEXT,
  RJT_TAG_FOR_SIGNATURE
};

extern jit_type_t jit_type_VALUE;
extern jit_type_t jit_type_ID;
extern jit_type_t jit_type_Function_Ptr;

#endif
