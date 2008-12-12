#ifndef minimal_node_h
#define minimal_node_h

#ifdef NEED_MINIMAL_NODE

#include "ruby.h"

typedef struct RNode {
  unsigned long flags;
  void * reserved;
  union {
    struct RNode * node;
    VALUE (*cfunc)(ANYARGS);
  } u1;
  union {
    struct RNode * node;
    VALUE value;
  } u2;
  union {
    struct RNode * node;
  } u3;
} NODE;

#define nd_cfnc u1.cfunc
#define nd_rval u2.value

#define NEW_NODE(t,a0,a1,a2) rb_node_newnode((t),(VALUE)(a0),(VALUE)(a1),(VALUE)(a2))

/* TODO: No way to know the correct size of node_type */
enum node_type {
  NODE_FOO,
};

void rb_add_method(VALUE, ID, NODE *, int);
NODE *rb_node_newnode(enum node_type, VALUE, VALUE, VALUE);

extern int NODE_MEMO;
extern int NODE_METHOD;
extern int NODE_FBODY;
extern int NODE_CFUNC;

void Init_minimal_node();

#define NOEX_PUBLIC 0x0

#define NEW_METHOD(n,x,v) NEW_NODE(NODE_METHOD,x,n,v)
#define NEW_FBODY(n,i) NEW_NODE(NODE_FBODY,i,n,0)
#define NEW_CFUNC(f,c) NEW_NODE(NODE_CFUNC,f,c,0)

#endif

#endif
