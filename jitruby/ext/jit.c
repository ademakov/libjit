#ifndef RUBY_DEFINES_GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef NEED_WINDOWS_H
#include <windows.h>
#endif

#include <ruby.h>

#include <stdio.h>

#include <jit/jit.h>
#include <jit/jit-dump.h>

#include "rubyjit.h"
#include "method_data.h"

#ifdef NEED_MINIMAL_NODE
#include "minimal_node.h"
#endif

#ifndef RARRAY_LEN
#define RARRAY_LEN(a) RARRAY(a)->len
#endif

#ifndef RARRAY_PTR
#define RARRAY_PTR(a) RARRAY(a)->ptr
#endif

static VALUE rb_mJIT;
static VALUE rb_cContext;
static VALUE rb_cFunction;
static VALUE rb_cType;
static VALUE rb_mABI;
static VALUE rb_cValue;
static VALUE rb_cLabel;
static VALUE rb_mCall;
static VALUE rb_cClosure;

jit_type_t jit_type_VALUE;
jit_type_t jit_type_ID;
jit_type_t jit_type_Function_Ptr;

static jit_type_t ruby_vararg_signature;

typedef void (*Void_Function_Ptr)();

struct Closure
{
  VALUE function;
  Void_Function_Ptr function_ptr;
};

#ifdef VALUE_IS_PTR
/* Rubinius */
typedef jit_ptr jit_VALUE;
#define jit_underlying_type_VALUE jit_type_void_ptr
#define SET_CONSTANT_VALUE(c, v) (c.un.ptr_value = v)

#elif SIZEOF_VALUE == 4
/* 32-bit */
typedef jit_uint jit_VALUE;
#define jit_underlying_type_VALUE jit_type_uint
#define SET_CONSTANT_VALUE(c, v) (c.un.uint_value = v)

#elif SIZEOF_VALUE == 8
/* 64-bit */
typedef jit_ulong jit_VALUE;
#define jit_underlying_type_VALUE jit_type_ulong
#define SET_CONSTANT_VALUE(c, v) (c.un.ulong_value = v)

#else
#error "Unsupported size for VALUE"
#endif

#if SIZEOF_ID == 4
/* 32-bit */
typedef jit_uint jit_ID;
#define jit_underlying_type_ID jit_type_uint
#define SET_CONSTANT_ID(c, v) (c.un.uint_value = v)
#elif SIZEOF_ID == 8
/* 64-bit */
typedef jit_ulong jit_ID;
#define jit_underlying_type_ID jit_type_ulong
#define SET_CONSTANT_ID(c, v) (c.un.ulong_value = v)
#else
#error "Unsupported size for ID"
#endif

typedef jit_ptr jit_Function_Ptr;
#define jit_underlying_type_void_ptr jit_type_void_ptr
#define SET_FUNCTION_POINTER_VALUE(c, v) (c.un.ptr_value = v)

#ifdef HAVE_RB_ERRINFO
#define ruby_errinfo rb_errinfo()
#endif

/* ---------------------------------------------------------------------------
 * Utility functions
 * ---------------------------------------------------------------------------
 */

static VALUE lookup_const(VALUE module, VALUE symbol)
{
  if(SYMBOL_P(symbol))
  {
    return rb_const_get(module, SYM2ID(symbol));
  }
  else
  {
    return symbol;
  }
}

static void check_type(char const * param_name, VALUE expected_klass, VALUE val)
{
  if(!rb_obj_is_kind_of(val, expected_klass))
  {
    rb_raise(
        rb_eTypeError,
        "Wrong type for %s; expected %s but got %s",
        param_name,
        rb_class2name(expected_klass),
        rb_class2name(CLASS_OF(val)));
  }
}

void raise_memory_error_if_zero(void * v)
{
  if(!v)
  {
    rb_raise(rb_eNoMemError, "Out of memory");
  }
}

/* ---------------------------------------------------------------------------
 * Context
 * ---------------------------------------------------------------------------
 */

static void context_mark(jit_context_t context)
{
  VALUE functions = (VALUE)jit_context_get_meta(context, RJT_FUNCTIONS);
  rb_gc_mark(functions);
}

/* 
 * call-seq:
 *   context = Context.new
 *
 * Create a new context.
 */
static VALUE context_s_new(VALUE klass)
{
  jit_context_t context = jit_context_create();
  jit_context_set_meta(context, RJT_FUNCTIONS, (void*)rb_ary_new(), 0);
  return Data_Wrap_Struct(rb_cContext, context_mark, jit_context_destroy, context);
}

/* 
 * call-seq:
 *   context.build { ... }
 *
 * Acquire a lock on the context so it can be used to build a function.
 */
static VALUE context_build(VALUE self)
{
  jit_context_t context;
  Data_Get_Struct(self, struct _jit_context, context);
  jit_context_build_start(context);
#ifdef HAVE_RB_ENSURE
  return rb_ensure(
      rb_yield,
      self,
      RUBY_METHOD_FUNC(jit_context_build_end),
      (VALUE)context);
#else
  /* Rubinius does not yet have rb_ensure */
  rb_yield(self);
  jit_context_build_end(context);
#endif
}

/*
 * call-seq:
 *   Context.build { |context| ... }
 *
 * Create a context and acquire a lock on it, then yield the context to
 * the block.
 */
static VALUE context_s_build(VALUE klass)
{
  return context_build(context_s_new(klass));
}

/* ---------------------------------------------------------------------------
 * Closure
 * ---------------------------------------------------------------------------
 */

static void mark_closure(struct Closure * closure)
{
  rb_gc_mark(closure->function);
}

VALUE closure_to_int(VALUE self)
{
  struct Closure * closure;
  Data_Get_Struct(self, struct Closure, closure);
  VALUE v = ULONG2NUM((unsigned long)closure->function_ptr);
  return v;
}

VALUE closure_to_s(VALUE self)
{
  struct Closure * closure;
  VALUE args[4];
  Data_Get_Struct(self, struct Closure, closure);
  args[0] = rb_str_new2("#<JIT::Closure:0x%x function=%s function_ptr=0x%x>");
  args[1] = ULONG2NUM((unsigned long)self);
  args[2] = rb_any_to_s(closure->function);
  args[3] = ULONG2NUM((unsigned long)closure->function_ptr);
  return rb_f_sprintf(sizeof(args)/sizeof(args[0]), args);
}

VALUE closure_inspect(VALUE self)
{
  return closure_to_s(self);
}

/* ---------------------------------------------------------------------------
 * Function
 * ---------------------------------------------------------------------------
 */

static void mark_function(jit_function_t function)
{
  rb_gc_mark((VALUE)jit_function_get_meta(function, RJT_VALUE_OBJECTS));
  rb_gc_mark((VALUE)jit_function_get_meta(function, RJT_CONTEXT));
}

static VALUE create_function(int argc, VALUE * argv, VALUE klass)
{
  VALUE context_v;
  VALUE signature_v;
  VALUE parent_function_v;

  jit_function_t function;
  jit_function_t parent_function;
  jit_context_t context;
  jit_type_t signature;
  jit_type_t untagged_signature;

  VALUE function_v;
  VALUE functions;

  int signature_tag;

  rb_scan_args(argc, argv, "21", &context_v, &signature_v, &parent_function_v);

  Data_Get_Struct(context_v, struct _jit_context, context);
  Data_Get_Struct(signature_v, struct _jit_type, signature);

  signature_tag = jit_type_get_kind(signature);

  /* If this signature was tagged, get the untagged type */
  if((untagged_signature = jit_type_get_tagged_type(signature)))
  {
    signature = untagged_signature;
  }

  if(RTEST(parent_function_v))
  {
    /* If this function has a parent, then it is a nested function */
    Data_Get_Struct(parent_function_v, struct _jit_function, parent_function);
    function = jit_function_create_nested(context, signature, parent_function);
  }
  else
  {
    /* Otherwise, it's a standalone function */
    function = jit_function_create(context, signature);
  }

  /* Make sure the function is around as long as the context is */
  if(!jit_function_set_meta(function, RJT_VALUE_OBJECTS, (void *)rb_ary_new(), 0, 0))
  {
    rb_raise(rb_eNoMemError, "Out of memory");
  }

  /* Remember the signature's tag for later */
  if(!jit_function_set_meta(function, RJT_TAG_FOR_SIGNATURE, (void *)signature_tag, 0, 0))
  {
    rb_raise(rb_eNoMemError, "Out of memory");
  }

  /* And remember the function's context for later */
  if(!jit_function_set_meta(function, RJT_CONTEXT, (void *)context_v, 0, 0))
  {
    rb_raise(rb_eNoMemError, "Out of memory");
  }

  function_v = Data_Wrap_Struct(rb_cFunction, mark_function, 0, function);

  /* Add this function to the context's list of functions */
  functions = (VALUE)jit_context_get_meta(context, RJT_FUNCTIONS);
  rb_ary_push(functions, function_v);

  return function_v;
}

/*
 * call-seq:
 *   function.compile()
 *
 * Begin compiling a function.
 */
static VALUE function_compile(VALUE self)
{
  jit_function_t function;
  Data_Get_Struct(self, struct _jit_function, function);
  if(!jit_function_compile(function))
  {
    rb_raise(rb_eRuntimeError, "Unable to compile function");
  }
  return self;
}

/*
 * call-seq:
 *   function = Function.new(context, signature, [parent])
 *
 * Create a new function.
 */
static VALUE function_s_new(int argc, VALUE * argv, VALUE klass)
{
  if(rb_block_given_p())
  {
    rb_raise(rb_eArgError, "Function.new does not take a block");
  }

  return create_function(argc, argv, klass);
}

static VALUE function_abandon_if_exception(VALUE function_v)
{
  if(ruby_errinfo)
  {
    jit_function_t function;
    Data_Get_Struct(function_v, struct _jit_function, function);
    jit_function_abandon(function);
  }
  return Qnil;
}

/*
 * call-seq:
 *   function = Function.new(context, signature, [parent]) { |function| ... }
 *
 * Create a new function, begin compiling it, and pass the function to
 * the block.
 */
static VALUE function_s_compile(int argc, VALUE * argv, VALUE klass)
{
  VALUE function = create_function(argc, argv, klass);
  rb_yield(function);
  function_compile(function);
#ifdef HAVE_RB_ENSURE
  rb_ensure(
      function_compile,
      function,
      function_abandon_if_exception,
      function);
#else
  /* Rubinius does not yet have rb_ensure */
  function_compile(function);
  function_abandon_if_exception(function);
#endif
  return function;
}

/*
 * Get the value that corresponds to a specified function parameter.
 *
 * call-seq:
 *   value = function.get_param(index)
 */
static VALUE function_get_param(VALUE self, VALUE idx)
{
  jit_function_t function;
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_function, function);
  value = jit_value_get_param(function, NUM2INT(idx));
  raise_memory_error_if_zero(value);
  return Data_Wrap_Struct(rb_cValue, 0, 0, value);
}

#include "insns.inc"

static VALUE function_value_klass(VALUE self, VALUE type_v, VALUE klass)
{
  jit_function_t function;
  jit_type_t type;
  jit_value_t value;

  Data_Get_Struct(self, struct _jit_function, function);

  type_v = lookup_const(rb_cType, type_v);
  check_type("type", rb_cType, type_v);
  Data_Get_Struct(type_v, struct _jit_type, type);

  /* TODO: When we wrap a value, we should inject a reference to the
   * function in the object, so the function stays around as long as the
   * value does */
  value = jit_value_create(function, type);
  return Data_Wrap_Struct(klass, 0, 0, value);
}

static VALUE coerce_to_jit(VALUE function, VALUE type_v, VALUE value_v);

/*
 * call-seq:
 *   value = function.value(type)
 *   value = function.value(type, initial_value)
 *
 * Create a value (placeholder/variable) with the given type.
 */
static VALUE function_value(int argc, VALUE * argv, VALUE self)
{
  VALUE type_v = Qnil;
  VALUE initial_value_v = Qnil;

  VALUE new_value = Qnil;

  rb_scan_args(argc, argv, "11", &type_v, &initial_value_v);

  new_value = function_value_klass(self, type_v, rb_cValue);

  if(argc > 1)
  {
    function_insn_store(
        self,
        new_value,
        coerce_to_jit(self, type_v, initial_value_v));
  }

  return new_value;
}

static jit_value_t create_const(jit_function_t function, jit_type_t type, VALUE constant)
{
  jit_constant_t c;
  int kind = jit_type_get_kind(type);

  switch(kind)
  {
    case JIT_TYPE_INT:
    {
      c.type = type;
      c.un.int_value = NUM2INT(constant);
      break;
    }

    case JIT_TYPE_UINT:
    {
      c.type = type;
      c.un.int_value = NUM2UINT(constant);
      break;
    }

    case JIT_TYPE_FLOAT32:
    {
      c.type = type;
      c.un.float32_value = NUM2DBL(constant);
      break;
    }

    case JIT_TYPE_FLOAT64:
    {
      c.type = type;
      c.un.float64_value = NUM2DBL(constant);
      break;
    }

    case JIT_TYPE_PTR:
    {
      c.type = type;
      c.un.ptr_value = (void *)NUM2ULONG(constant);
      break;
    }

    case JIT_TYPE_FIRST_TAGGED + RJT_OBJECT:
    {
      VALUE value_objects = (VALUE)jit_function_get_meta(function, RJT_VALUE_OBJECTS);

      c.type = type;
      SET_CONSTANT_VALUE(c, constant);

      /* Make sure the object gets marked as long as the function is
       * around */
      rb_ary_push(value_objects, constant);
      break;
    }

    case JIT_TYPE_FIRST_TAGGED + RJT_ID:
    {
      c.type = type;
      SET_CONSTANT_ID(c, SYM2ID(constant));
      break;
    }

    case JIT_TYPE_FIRST_TAGGED + RJT_FUNCTION_PTR:
    {
      c.type = type;
      SET_FUNCTION_POINTER_VALUE(
          c,
          (Void_Function_Ptr)NUM2ULONG(rb_to_int(constant)));
      break;
    }

    default:
      rb_raise(rb_eTypeError, "Unsupported type");
  }

  return jit_value_create_constant(function, &c);
}

/*
 * call-seq:
 *   value = function.const(type, constant_value)
 *
 * Create a constant value with the given type.
 */
static VALUE function_const(VALUE self, VALUE type_v, VALUE constant)
{
  jit_function_t function;
  jit_type_t type;
  jit_value_t value;

  Data_Get_Struct(self, struct _jit_function, function);

  type_v = lookup_const(rb_cType, type_v);
  check_type("type", rb_cType, type_v);
  Data_Get_Struct(type_v, struct _jit_type, type);

  value = create_const(function, type, constant);
  return Data_Wrap_Struct(rb_cValue, 0, 0, value);
}

static VALUE coerce_to_jit(VALUE function, VALUE type_v, VALUE value_v)
{
  if(rb_obj_is_kind_of(value_v, rb_cValue))
  {
    return value_v;
  }
  else
  {
    return function_const(function, type_v, value_v);
  }
}

static void convert_call_args(jit_function_t function, jit_value_t * args, VALUE args_v, jit_type_t signature)
{
  int j;

  for(j = 0; j < RARRAY_LEN(args_v); ++j)
  {
    VALUE value = RARRAY_PTR(args_v)[j];
    jit_value_t arg;

    jit_type_t type = jit_type_get_param(signature, j);
    if(!type)
    {
      rb_raise(rb_eArgError, "Type missing for param %d", j);
    }

    if(rb_obj_is_kind_of(value, rb_cValue))
    {
      Data_Get_Struct(value, struct _jit_value, arg);
      if(!arg)
      {
        rb_raise(rb_eArgError, "Argument %d is invalid", j);
      }
      args[j] = arg;
    }
    else
    {
      args[j] = create_const(function, type, value);
    }
  }
}

/* 
 * call-seq:
 *   value = function.call(name, called_function, flags, [arg1 [, ... ]])
 *
 * Generate an instruction to call the specified function.
 */
static VALUE function_insn_call(int argc, VALUE * argv, VALUE self)
{
  jit_function_t function;

  VALUE name_v;
  VALUE called_function_v;
  VALUE args_v;
  VALUE flags_v = Qnil;

  char const * name;
  jit_function_t called_function;
  jit_type_t signature;
  jit_value_t * args;
  jit_value_t retval;
  int flags;
  size_t num_args;

  rb_scan_args(argc, argv, "3*", &name_v, &called_function_v, &flags_v, &args_v);

  Data_Get_Struct(self, struct _jit_function, function);

  name = STR2CSTR(name_v);

  check_type("called function", rb_cFunction, called_function_v);
  Data_Get_Struct(called_function_v, struct _jit_function, called_function);

  num_args = RARRAY_LEN(args_v);
  args = ALLOCA_N(jit_value_t, num_args);

  signature = jit_function_get_signature(function);
  convert_call_args(function, args, args_v, signature);

  flags = NUM2INT(flags_v);

  retval = jit_insn_call(
      function, name, called_function, 0, args, num_args, flags);
  return Data_Wrap_Struct(rb_cValue, 0, 0, retval);
}

/*
 * call-seq:
 *   value = function.call(name, flags, [arg1 [, ... ]])
 *
 * Generate an instruction to call a native function.
 */
static VALUE function_insn_call_native(int argc, VALUE * argv, VALUE self)
{
  jit_function_t function;

  VALUE name_v;
  VALUE args_v;
  VALUE function_ptr_v;
  VALUE signature_v;
  VALUE flags_v;

  char const * name;
  jit_value_t * args;
  jit_value_t retval;
  void * function_ptr;
  jit_type_t signature;
  int flags;
  size_t num_args;

  rb_scan_args(argc, argv, "4*", &name_v, &function_ptr_v, &signature_v, &flags_v, &args_v);

  Data_Get_Struct(self, struct _jit_function, function);
  
  if(SYMBOL_P(name_v))
  {
    name = rb_id2name(SYM2ID(name_v));
  }
  else
  {
    name = StringValuePtr(name_v);
  }

  function_ptr = (void *)NUM2ULONG(function_ptr_v);

  Data_Get_Struct(signature_v, struct _jit_type, signature);

  num_args = RARRAY_LEN(args_v);
  args = ALLOCA_N(jit_value_t, num_args);

  if(num_args != jit_type_num_params(signature))
  {
    rb_raise(
        rb_eArgError,
        "Wrong number of arguments passed for %s (expecting %d but got %d)", 
        name,
        jit_type_num_params(signature),
        num_args);
  }

  convert_call_args(function, args, args_v, signature);

  flags = NUM2INT(flags_v);

  retval = jit_insn_call_native(
      function, name, function_ptr, signature, args, num_args, flags);
  return Data_Wrap_Struct(rb_cValue, 0, 0, retval);
}

/*
 * call-seq:
 *   function.insn_return()
 *   function.insn_return(value)
 *
 * Emit an instruction to return from the function.  If value is
 * specified, return the given value, otherwise return void.
 */
static VALUE function_insn_return(int argc, VALUE * argv, VALUE self)
{
  jit_function_t function;
  jit_value_t value = 0;
  VALUE value_v = Qnil;

  rb_scan_args(argc, argv, "01", &value_v);

  if(value_v != Qnil)
  {
    Data_Get_Struct(value_v, struct _jit_value, value);
  }

  Data_Get_Struct(self, struct _jit_function, function);
  jit_insn_return(function, value);

  return Qnil;
}

/*
 * call-seq:
 *   function.apply(arg1 [, arg2 [, ... ]])
 *
 * Call a compiled function.  Each argument passed in will be converted
 * to the type specified by the function's signature.
 *
 * If the function's signature is Type::RUBY_VARARG_SIGNATURE, then the
 * arguments will be passed in with the first parameter the count of the
 * number of arguments, the second parameter a pointer to an array
 * containing the second through the last argument, and the third
 * parameter the explicit self (that is, the first argument passed to
 * apply).
 */
static VALUE function_apply(int argc, VALUE * argv, VALUE self)
{
  jit_function_t function;
  jit_type_t signature;
  int j, n;
  void * * args;
  char * arg_data;
  int signature_tag;

  Data_Get_Struct(self, struct _jit_function, function);
  signature = jit_function_get_signature(function);
  n = jit_type_num_params(signature);

  /* void pointers to each of the arguments */
  args = ALLOCA_N(void *, n);

  /* the actual data */
  /* TODO: we need to allocate the proper size (but 8 bytes per arg
   * should be sufficient for now) */
  arg_data = (char *)ALLOCA_N(char, 8 * n);

  signature_tag = (int)jit_function_get_meta(function, RJT_TAG_FOR_SIGNATURE);
  if(signature_tag == JIT_TYPE_FIRST_TAGGED + RJT_RUBY_VARARG_SIGNATURE)
  {
    jit_VALUE result;
    int f_argc = argc - 1;
    VALUE f_self = *(VALUE *)argv;
    VALUE * f_argv = ((VALUE *)argv) + 1;
    void * f_args[3];
    f_args[0] = &f_argc;
    f_args[1] = &f_argv;
    f_args[2] = &f_self;
    jit_function_apply(function, f_args, &result);
    return result;
  }

  if(argc != n)
  {
    rb_raise(
        rb_eArgError,
        "Wrong number of arguments (expected %d but got %d)",
        n,
        argc);
  }

  for(j = 0; j < n; ++j)
  {
    jit_type_t arg_type = jit_type_get_param(signature, j);
    int kind = jit_type_get_kind(arg_type);
    switch(kind)
    {
      case JIT_TYPE_INT:
      {
        *(int *)arg_data = NUM2INT(argv[j]);
        args[j] = (int *)arg_data;
        arg_data += sizeof(int);
        break;
      }

      case JIT_TYPE_UINT:
      {
        *(int *)arg_data = NUM2UINT(argv[j]);
        args[j] = (int *)arg_data;
        arg_data += sizeof(int);
        break;
      }

      case JIT_TYPE_FIRST_TAGGED + RJT_OBJECT:
      {
        *(VALUE *)arg_data = argv[j];
        args[j] = (VALUE *)arg_data;
        arg_data += sizeof(VALUE);
        break;
      }

      case JIT_TYPE_FIRST_TAGGED + RJT_ID:
      {
        *(ID *)arg_data = SYM2ID(argv[j]);
        args[j] = (ID *)arg_data;
        arg_data += sizeof(ID);
        break;
      }

      case JIT_TYPE_FIRST_TAGGED + RJT_FUNCTION_PTR:
      {
        *(Void_Function_Ptr *)arg_data =
          (Void_Function_Ptr)NUM2ULONG(rb_to_int(argv[j]));
        args[j] = (Void_Function_Ptr *)(arg_data);
        arg_data += sizeof(Void_Function_Ptr);
        break;
      }

      default:
        rb_raise(rb_eTypeError, "Unsupported type %d", kind);
    }
  }

  {
    jit_type_t return_type = jit_type_get_return(signature);
    int return_kind = jit_type_get_kind(return_type);
    switch(return_kind)
    {
      case JIT_TYPE_INT:
      {
        jit_int result;
        jit_function_apply(function, args, &result);
        return INT2NUM(result);
      }

      case JIT_TYPE_FLOAT32:
      {
        jit_float32 result;
        jit_function_apply(function, args, &result);
        return rb_float_new(result);
      }

      case JIT_TYPE_FLOAT64:
      {
        jit_float64 result;
        jit_function_apply(function, args, &result);
        return rb_float_new(result);
      }

      case JIT_TYPE_FIRST_TAGGED + RJT_OBJECT:
      {
        jit_VALUE result;
        jit_function_apply(function, args, &result);
        return result;
      }

      case JIT_TYPE_FIRST_TAGGED + RJT_ID:
      {
        jit_ID result;
        jit_function_apply(function, args, &result);
        return ID2SYM(result);
      }

      default:
        rb_raise(rb_eTypeError, "Unsupported return type %d", return_kind);
    }
  }
}

/*
 * call-seq:
 *   level = function.optimization_level()
 *
 * Get the optimization level for a function.
 */
static VALUE function_optimization_level(VALUE self)
{
  jit_function_t function;
  Data_Get_Struct(self, struct _jit_function, function);
  return INT2NUM(jit_function_get_optimization_level(function));
}

/*
 * call-seq:
 *   function.optimization_level = level
 *
 * Set the optimization level for a function.
 */
static VALUE function_set_optimization_level(VALUE self, VALUE level)
{
  jit_function_t function;
  Data_Get_Struct(self, struct _jit_function, function);
  jit_function_set_optimization_level(function, NUM2INT(level));
  return level;
}

/*
 * call-seq:
 *   level = function.max_optimization_level()
 *
 * Get the maximum optimization level (which should be the same for any
 * function).
 */
static VALUE function_max_optimization_level(VALUE klass)
{
  return INT2NUM(jit_function_get_max_optimization_level());
}

/*
 * call-seq:
 *   str = function.dump()
 *
 * Dump the instructions in a function to a string.
 */
static VALUE function_dump(VALUE self)
{
#ifdef HAVE_FMEMOPEN
  jit_function_t function;
  char buf[16*1024]; /* TODO: big enough? */
  FILE * fp = fmemopen(buf, sizeof(buf), "w");
  Data_Get_Struct(self, struct _jit_function, function);
  jit_dump_function(fp, function, 0);
  fclose(fp);
  return rb_str_new2(buf);
#else
  rb_raise(rb_eNotImpError, "Not implemented: missing fmemopen");
#endif
}

/*
 * call-seq:
 *   ptr = function.to_closure()
 *
 * Return a pointer to a closure for a function.  This pointer can be
 * passed into other functions as a function pointer.
 */
static VALUE function_to_closure(VALUE self)
{
  jit_function_t function;
  struct Closure * closure;
  VALUE closure_v = Data_Make_Struct(
      rb_cClosure, struct Closure, mark_closure, free, closure);
  Data_Get_Struct(self, struct _jit_function, function);
  closure->function = self;
  closure->function_ptr =
    (Void_Function_Ptr)jit_function_to_closure(function);
  return closure_v;
}

/*
 * call-seq:
 *   context = function.context()
 *
 * Get a function's context.
 */
static VALUE function_get_context(VALUE self)
{
  jit_function_t function;
  Data_Get_Struct(self, struct _jit_function, function);
  return (VALUE)jit_function_get_meta(function, RJT_CONTEXT);
}

/*
 * call-seq:
 *   is_compiled = function.compiled?
 *
 * Determine whether a function is compiled.
 */
static VALUE function_is_compiled(VALUE self)
{
  jit_function_t function;
  Data_Get_Struct(self, struct _jit_function, function);
  return jit_function_is_compiled(function) ? Qtrue : Qfalse;
}

/* ---------------------------------------------------------------------------
 * Type
 * ---------------------------------------------------------------------------
 */

/* This function does not increment the reference count.  It is assumed
 * that the caller will increment the reference count and that the newly
 * wrapped object will take ownership. */
static VALUE wrap_type_with_klass(jit_type_t type, VALUE klass)
{
  return Data_Wrap_Struct(klass, 0, jit_type_free, type);
}

static VALUE wrap_type(jit_type_t type)
{
  return wrap_type_with_klass(type, rb_cType);
}

/*
 * call-seq:
 *   type = Type.create_signature(abi, return_type, array_of_param_types)
 *
 * Create a new signature.
 */
static VALUE type_s_create_signature(
    VALUE klass, VALUE abi_v, VALUE return_type_v, VALUE params_v)
{
  jit_abi_t abi;
  jit_type_t return_type;
  jit_type_t * params;
  jit_type_t signature;
  int j;
  int len;

  return_type_v = lookup_const(rb_cType, return_type_v);
  check_type("return type", rb_cType, return_type_v);
  Data_Get_Struct(return_type_v, struct _jit_type, return_type);

  Check_Type(params_v, T_ARRAY);
  len = RARRAY_LEN(params_v);
  params = ALLOCA_N(jit_type_t, len);
  for(j = 0; j < len; ++j)
  {
    VALUE param = RARRAY_PTR(params_v)[j];
    param = lookup_const(rb_cType, param);
    check_type("param", rb_cType, param);
    Data_Get_Struct(param, struct _jit_type, params[j]);
  }

  abi_v = lookup_const(rb_mABI, abi_v);
  abi = NUM2INT(abi_v);

  signature = jit_type_create_signature(abi, return_type, params, len, 1);
  return wrap_type(signature);
}

/*
 * call-seq:
 *   type = Type.create_struct(array_of_field_types)
 *
 * Create a new struct type.
 */
static VALUE type_s_create_struct(
    VALUE klass, VALUE fields_v)
{
  jit_type_t * fields;
  jit_type_t struct_type;
  int len;
  int j;

  Check_Type(fields_v, T_ARRAY);
  len = RARRAY_LEN(fields_v);
  fields = ALLOCA_N(jit_type_t, len);
  for(j = 0; j < len; ++j)
  {
    VALUE field = RARRAY_PTR(fields_v)[j];
    check_type("field", rb_cType, field);
    Data_Get_Struct(field, struct _jit_type, fields[j]);
  }

  struct_type = jit_type_create_struct(fields, RARRAY_LEN(fields_v), 1);
  return wrap_type_with_klass(struct_type, klass);
}

/*
 * call-seq:
 *   type = Type.create_pointer(pointed_to_type)
 *
 * Create a new pointer type, pointing to the given type.
 */
static VALUE type_s_create_pointer(
    VALUE klass, VALUE type_v)
{
  jit_type_t type;
  jit_type_t pointer_type;
  Data_Get_Struct(type_v, struct _jit_type, type);
  pointer_type = jit_type_create_pointer(type, 1);
  return wrap_type_with_klass(pointer_type, klass);
}

/*
 * call-seq:
 *   offset = struct_type.get_offset(index)
 *
 * Get the offset of the nth field in a struct.
 */
static VALUE type_get_offset(VALUE self, VALUE field_index_v)
{
  int field_index = NUM2INT(field_index_v);
  jit_type_t type;
  Data_Get_Struct(self, struct _jit_type, type);
  return INT2NUM(jit_type_get_offset(type, field_index));
}

/*
 * call-seq:
 *   offset = struct_type.set_offset(index, offset)
 *
 * Set the offset of the nth field in a struct.
 */
static VALUE type_set_offset(VALUE self, VALUE field_index_v, VALUE offset_v)
{
  int field_index = NUM2INT(field_index_v);
  int offset = NUM2UINT(offset_v);
  jit_type_t type;
  Data_Get_Struct(self, struct _jit_type, type);
  jit_type_set_offset(type, field_index, offset);
  return Qnil;
}

/*
 * call-seq:
 *   offset = struct_type.size
 *
 * Get the size of a struct or union type.
 */
static VALUE type_size(VALUE self)
{
  jit_type_t type;
  Data_Get_Struct(self, struct _jit_type, type);
  return INT2NUM(jit_type_get_size(type));
}

/* ---------------------------------------------------------------------------
 * Value
 * ---------------------------------------------------------------------------
 */

static VALUE value_s_new_value(VALUE klass, VALUE function, VALUE type)
{
  return function_value_klass(function, type, klass);
}

/*
 * call-seq:
 *   str = value.to_s
 *
 * Return a string representation of the value.
 */
static VALUE value_to_s(VALUE self)
{
#ifdef HAVE_FMEMOPEN
  char buf[1024];
  FILE * fp = fmemopen(buf, sizeof(buf), "w");
  jit_value_t value;
  jit_function_t function;
  Data_Get_Struct(self, struct _jit_value, value);
  function = jit_value_get_function(value);
  jit_dump_value(fp, function, value, 0);
  fclose(fp);
  return rb_str_new2(buf);
#else
  rb_raise(rb_eNotImpError, "Not implemented: missing fmemopen");
#endif
}

/*
 * call-seq:
 *   str = value.inspect
 *
 * Return a string representation of a value with additional
 * information about the value.
 */
static VALUE value_inspect(VALUE self)
{
  jit_value_t value;
  jit_type_t type;
  char const * cname = rb_obj_classname(self);
  VALUE args[6];
  Data_Get_Struct(self, struct _jit_value, value);
  type = jit_value_get_type(value);
  args[0] = rb_str_new2("#<%s:0x%x %s ptr=0x%x type=0x%x>");
  args[1] = rb_str_new2(cname);
  args[2] = ULONG2NUM((unsigned long)self);
  args[3] = value_to_s(self);
  args[4] = ULONG2NUM((unsigned long)value);
  args[5] = ULONG2NUM((unsigned long)type);
  return rb_f_sprintf(sizeof(args)/sizeof(args[0]), args);
}

/*
 * call-seq:
 *   is_valid = value.valid?
 *
 * Determine if a value is valid (non-zero).
 */
static VALUE value_is_valid(VALUE self)
{
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_value, value);
  return (value != 0) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   is_temporary = value.temporary?
 *
 * Determine if a value represents a temporary.
 */
static VALUE value_is_temporary(VALUE self)
{
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_value, value);
  return jit_value_is_temporary(value) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   is_local = value.local?
 *
 * Determine if a value represents a local.
 */
static VALUE value_is_local(VALUE self)
{
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_value, value);
  return jit_value_is_local(value) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   is_local = value.constant?
 *
 * Determine if a value represents a constant.
 */
static VALUE value_is_constant(VALUE self)
{
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_value, value);
  return jit_value_is_constant(value) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   is_volatile = value.volatile?
 *
 * Determine if a value is volatile (that is, the contents must be
 * reloaded from memory each time it is used).
 */
static VALUE value_is_volatile(VALUE self)
{
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_value, value);
  return jit_value_is_volatile(value) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   value.volatile = is_volatile
 *
 * Make a value volatile (that is, ensure that its contents are reloaded
 * from memory each time it is used).
 */
static VALUE value_set_volatile(VALUE self)
{
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_value, value);
  jit_value_set_volatile(value);
  return Qnil;
}

/*
 * call-seq:
 *   is_addressable = value.addressable?
 *
 * Determine if a value is addressable.
 */
static VALUE value_is_addressable(VALUE self)
{
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_value, value);
  return jit_value_is_addressable(value) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   value.addressable = is_addressable
 *
 * Make a value addressable.
 */
static VALUE value_set_addressable(VALUE self)
{
  jit_value_t value;
  Data_Get_Struct(self, struct _jit_value, value);
  jit_value_set_addressable(value);
  return Qnil;
}

/*
 * call-seq:
 *   function = value.function()
 *
 * Get a value's function.
 */
static VALUE value_function(VALUE self)
{
  jit_value_t value;
  jit_function_t function;
  Data_Get_Struct(self, struct _jit_value, value);
  function = jit_value_get_function(value);
  return Data_Wrap_Struct(rb_cFunction, mark_function, 0, function);
}

/*
 * call-seq:
 *   function = value.type()
 *
 * Get a value's type.
 */
static VALUE value_type(VALUE self)
{
  jit_value_t value;
  jit_type_t type;
  Data_Get_Struct(self, struct _jit_value, value);
  type = jit_value_get_type(value);
  type = jit_type_copy(type);
  return wrap_type(type);
}

/*
 * call-seq:
 *   value1, value2 = value1.coerce(value2)
 *
 * If the given value is a JIT::Value, return an +[ self, value ]+,
 * otherwise coerce the value to the same type as self and return
 * +[ self, coerced_value ]+.
 */
static VALUE value_coerce(VALUE self, VALUE value)
{
  return rb_assoc_new(
      self,
      coerce_to_jit(
          value_function(self),
          value_type(self),
          value));
}

/* ---------------------------------------------------------------------------
 * Label
 * ---------------------------------------------------------------------------
 */

/*
 * call-seq:
 *   label = Label.new
 *
 * Create a new label.
 */
static VALUE label_s_new(VALUE klass)
{
  jit_label_t * label;
  VALUE labelval = Data_Make_Struct(rb_cLabel, jit_label_t, 0, xfree, label);
  *label = jit_label_undefined;
  return labelval;
}

/* ---------------------------------------------------------------------------
 * Module
 * ---------------------------------------------------------------------------
 */

/*
 * call-seq:
 *   module.define_jit_method(name, function)
 *
 * Use a Function to define an instance method on a module.  The
 * function should have one of two signatures:
 *
 * * The first parameter to the function should be an OBJECT that
 *   represents the self parameter and the rest of the parameters, or
 *
 * * The function's signature should be Type::RUBY_VARARG_SIGNATURE.
 */
static VALUE module_define_jit_method(VALUE klass, VALUE name_v, VALUE function_v)
{
  char const * name;
  jit_function_t function;
  jit_type_t signature;
  int signature_tag;
  int arity;
  VALUE closure_v;
  struct Closure * closure;

  if(SYMBOL_P(name_v))
  {
    name = rb_id2name(SYM2ID(name_v));
  }
  else
  {
    name = STR2CSTR(name_v);
  }

  Data_Get_Struct(function_v, struct _jit_function, function);

  signature = jit_function_get_signature(function);
  signature_tag = (int)jit_function_get_meta(function, RJT_TAG_FOR_SIGNATURE);
  if(signature_tag == JIT_TYPE_FIRST_TAGGED + RJT_RUBY_VARARG_SIGNATURE)
  {
    arity = -1;
  }
  else
  {
    /* TODO: check the types to make sure they are OBJECT */
    arity = jit_type_num_params(signature) - 1;
  }

  closure_v = function_to_closure(function_v);
  Data_Get_Struct(closure_v, struct Closure, closure);
  define_method_with_data(
      klass, rb_intern(name), RUBY_METHOD_FUNC(closure->function_ptr),
      arity, closure_v);
  return Qnil;
}

/* ---------------------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------------------
 */

void Init_jit()
{
  jit_init();

  rb_mJIT = rb_define_module("JIT");

  rb_cContext = rb_define_class_under(rb_mJIT, "Context", rb_cObject);
  rb_define_singleton_method(rb_cContext, "new", context_s_new, 0);
  rb_define_method(rb_cContext, "build", context_build, 0);
  rb_define_singleton_method(rb_cContext, "build", context_s_build, 0);

  rb_cClosure = rb_define_class_under(rb_mJIT, "Closure", rb_cObject);
  rb_define_method(rb_cClosure, "to_int", closure_to_int, 0);
  rb_define_method(rb_cClosure, "to_s", closure_to_s, 0);
  rb_define_method(rb_cClosure, "inspect", closure_inspect, 0);

  rb_cFunction = rb_define_class_under(rb_mJIT, "Function", rb_cObject);
  rb_define_singleton_method(rb_cFunction, "new", function_s_new, -1);
  rb_define_method(rb_cFunction, "compile", function_compile, 0);
  rb_define_singleton_method(rb_cFunction, "compile", function_s_compile, -1);
  rb_define_method(rb_cFunction, "get_param", function_get_param, 1);
  init_insns();
  rb_define_method(rb_cFunction, "insn_call", function_insn_call, -1);
  rb_define_method(rb_cFunction, "insn_call_native", function_insn_call_native, -1);
  rb_define_method(rb_cFunction, "insn_return", function_insn_return, -1);
  rb_define_method(rb_cFunction, "apply", function_apply, -1);
  rb_define_alias(rb_cFunction, "call", "apply");
  rb_define_method(rb_cFunction, "value", function_value, -1);
  rb_define_method(rb_cFunction, "const", function_const, 2);
  rb_define_method(rb_cFunction, "optimization_level", function_optimization_level, 0);
  rb_define_method(rb_cFunction, "optimization_level=", function_set_optimization_level, 1);
  rb_define_singleton_method(rb_cFunction, "max_optimization_level", function_max_optimization_level, 0);
  rb_define_method(rb_cFunction, "dump", function_dump, 0);
  rb_define_method(rb_cFunction, "to_closure", function_to_closure, 0);
  rb_define_method(rb_cFunction, "context", function_get_context, 0);
  rb_define_method(rb_cFunction, "compiled?", function_is_compiled, 0);

  rb_cType = rb_define_class_under(rb_mJIT, "Type", rb_cObject);
  rb_define_singleton_method(rb_cType, "create_signature", type_s_create_signature, 3);
  rb_define_singleton_method(rb_cType, "create_struct", type_s_create_struct, 1);
  rb_define_singleton_method(rb_cType, "create_pointer", type_s_create_pointer, 1);
  rb_define_method(rb_cType, "get_offset", type_get_offset, 1);
  rb_define_method(rb_cType, "set_offset", type_set_offset, 2);
  rb_define_method(rb_cType, "size", type_size, 0);
  rb_define_const(rb_cType, "VOID", wrap_type(jit_type_void));
  rb_define_const(rb_cType, "SBYTE", wrap_type(jit_type_sbyte));
  rb_define_const(rb_cType, "UBYTE", wrap_type(jit_type_ubyte));
  rb_define_const(rb_cType, "SHORT", wrap_type(jit_type_short));
  rb_define_const(rb_cType, "USHORT", wrap_type(jit_type_ushort));
  rb_define_const(rb_cType, "INT", wrap_type(jit_type_int));
  rb_define_const(rb_cType, "UINT", wrap_type(jit_type_uint));
  rb_define_const(rb_cType, "NINT", wrap_type(jit_type_nint));
  rb_define_const(rb_cType, "NUINT", wrap_type(jit_type_nuint));
  rb_define_const(rb_cType, "LONG", wrap_type(jit_type_long));
  rb_define_const(rb_cType, "ULONG", wrap_type(jit_type_ulong));
  rb_define_const(rb_cType, "FLOAT32", wrap_type(jit_type_float32));
  rb_define_const(rb_cType, "FLOAT64", wrap_type(jit_type_float64));
  rb_define_const(rb_cType, "NFLOAT", wrap_type(jit_type_nfloat));
  rb_define_const(rb_cType, "VOID_PTR", wrap_type(jit_type_void_ptr));

  jit_type_VALUE = jit_type_create_tagged(jit_underlying_type_VALUE, RJT_OBJECT, 0, 0, 1);
  rb_define_const(rb_cType, "OBJECT", wrap_type(jit_type_VALUE));

  jit_type_ID = jit_type_create_tagged(jit_underlying_type_ID, RJT_ID, 0, 0, 1);
  rb_define_const(rb_cType, "ID", wrap_type(jit_type_ID));

  jit_type_Function_Ptr = jit_type_create_tagged(jit_underlying_type_ID, RJT_FUNCTION_PTR, 0, 0, 1);
  rb_define_const(rb_cType, "FUNCTION_PTR", wrap_type(jit_type_Function_Ptr));

  {
    jit_type_t ruby_vararg_param_types[3];
    jit_type_t ruby_vararg_signature_untagged;
    ruby_vararg_param_types[0] = jit_type_int;
    ruby_vararg_param_types[1] = jit_type_void_ptr;
    ruby_vararg_param_types[2] = jit_type_VALUE;
    ruby_vararg_signature_untagged = jit_type_create_signature(
          jit_abi_cdecl,
          jit_type_VALUE,
          ruby_vararg_param_types,
          3,
          1);
    ruby_vararg_signature = jit_type_create_tagged(ruby_vararg_signature_untagged, RJT_RUBY_VARARG_SIGNATURE, 0, 0, 1);
  }
  rb_define_const(rb_cType, "RUBY_VARARG_SIGNATURE", wrap_type(ruby_vararg_signature));

  rb_mABI = rb_define_module_under(rb_mJIT, "ABI");
  rb_define_const(rb_mABI, "CDECL", INT2NUM(jit_abi_cdecl));
  rb_define_const(rb_mABI, "VARARG", INT2NUM(jit_abi_vararg));
  rb_define_const(rb_mABI, "STDCALL", INT2NUM(jit_abi_stdcall));
  rb_define_const(rb_mABI, "FASTCALL", INT2NUM(jit_abi_fastcall));

  rb_cValue = rb_define_class_under(rb_mJIT, "Value", rb_cObject);
  rb_define_singleton_method(rb_cValue, "new_value", value_s_new_value, 2);
  rb_define_method(rb_cValue, "to_s", value_to_s, 0);
  rb_define_method(rb_cValue, "inspect", value_inspect, 0);
  rb_define_method(rb_cValue, "valid?", value_is_valid, 0);
  rb_define_method(rb_cValue, "temporary?", value_is_temporary, 0);
  rb_define_method(rb_cValue, "local?", value_is_local, 0);
  rb_define_method(rb_cValue, "constant?", value_is_constant, 0);
  rb_define_method(rb_cValue, "volatile?", value_is_volatile, 0);
  rb_define_method(rb_cValue, "set_volatile", value_set_volatile, 0);
  rb_define_method(rb_cValue, "addressable?", value_is_addressable, 0);
  rb_define_method(rb_cValue, "addressable=", value_set_addressable, 0);
  rb_define_method(rb_cValue, "function", value_function, 0);
  rb_define_method(rb_cValue, "type", value_type, 0);
  rb_define_method(rb_cValue, "coerce", value_coerce, 1);

  rb_cLabel = rb_define_class_under(rb_mJIT, "Label", rb_cObject);
  rb_define_singleton_method(rb_cLabel, "new", label_s_new, 0);

  rb_mCall = rb_define_module_under(rb_mJIT, "Call");
  rb_define_const(rb_mCall, "NOTHROW", INT2NUM(JIT_CALL_NOTHROW));
  rb_define_const(rb_mCall, "NORETURN", INT2NUM(JIT_CALL_NORETURN));
  rb_define_const(rb_mCall, "TAIL", INT2NUM(JIT_CALL_TAIL));

  /* VALUE rb_cModule = rb_define_module(); */
  rb_define_method(rb_cModule, "define_jit_method", module_define_jit_method, 2);

#ifdef NEED_MINIMAL_NODE
  Init_minimal_node();
#endif
}

