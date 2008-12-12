#ifdef NEED_MINIMAL_NODE

#include "minimal_node.h"
#include <string.h>

int NODE_MEMO = 0;
int NODE_METHOD = 0;
int NODE_FBODY = 0;
int NODE_CFUNC = 0;

char const * ruby_node_name(int node);

static int node_value(char const * name)
{
  /* TODO: any way to end the block? */
  int j;
  for(j = 0; ; ++j)
  {
    if(!strcmp(name, ruby_node_name(j)))
    {
      return j;
    }
  }
}

void Init_minimal_node()
{
  NODE_MEMO = node_value("NODE_MEMO");
  NODE_METHOD = node_value("NODE_METHOD");
  NODE_FBODY = node_value("NODE_FBODY");
  NODE_CFUNC = node_value("NODE_CFUNC");
}

#endif

