/** @file traceback.c
 *  @brief The traceback function
 *
 *  This file contains the traceback function for the traceback library
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @bug Unimplemented
 */

#include "traceback_internal.h"
#include "ebp_helper.h"
#include <string.h>
#include <limits.h>

int get_func(void* ret)
{
  int i,index;
  int min=INT_MAX;
  for(i=0;i<FUNCTS_MAX_NUM;i++) {
   if(strlen(functions[i].name)==0)
      break;
   int diff=ret-functions[i].addr;
   if(diff<min && diff>=0) {
     min=diff;
     index=i;
   }
  }
  return index;
}
void traceback(FILE *fp)
{
  int index;
  void* ebp = ebp_addr();
  void *ret;
  while(1) {
    ret=(void*)(long)*(int *)(ebp+4);
    ebp=(void*)(long)*(int *)ebp;
    index=get_func(ret);
    if(strcmp(functions[index].name,"__libc_start_main")==0)
      return;
    printf("%d,%s\n",index,functions[index].name);
  }
}
