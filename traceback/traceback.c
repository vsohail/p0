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
#include <ctype.h>

int get_func(void *ret);
void traceback(FILE *fp);
void get_string(char *str,char *buf,void *addr);
void get_argument_details(char *buf,const argsym_t *arg,void *ebp);
void get_details(char *buf,const functsym_t *func,void *ebp);

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

void get_details(char* buf,const functsym_t* func,void *ebp)
{
  int i;
  sprintf(buf,"Function %s(",func->name);
  for(i=0;i<ARGS_MAX_NUM;++i) {
    if(strlen(func->args[i].name)==0)
      break;
    if(i>0)
      sprintf(buf,"%s, ",buf);
    get_argument_details(buf,&func->args[i],ebp);
  }
  if(i==0) {
    sprintf(buf,"%svoid",buf);
  }
  sprintf(buf,"%s), in\n",buf);
}

void get_argument_details(char* buf,const argsym_t *arg,void* ebp)
{
  void *addr = arg->offset+ebp;
  switch (arg->type) {
    case TYPE_VOIDSTAR:
      sprintf(buf,"%svoid *%s=0v%x",buf,arg->name,(unsigned int)(*(int *)addr));
      break;
    case TYPE_INT:
      sprintf(buf,"%sint %s=%d",buf,arg->name,*((int *)addr));
      break;
    case TYPE_FLOAT:
      sprintf(buf,"%sfloat %s=%f",buf,arg->name,*((float *)addr));
      break;
    case TYPE_DOUBLE:
      sprintf(buf,"%sdouble %s=%f",buf,arg->name,*((double *)addr));
      break;
    case TYPE_CHAR:
      sprintf(buf,"%schar %s=",buf,arg->name);
      if(isprint(*((char *)addr)))
        sprintf(buf,"%s'%c'",buf,*((char *)addr));
      else
        sprintf(buf,"%s'\\%o'",buf,*((char *)addr));
      break;
    case TYPE_STRING:
      sprintf(buf,"%schar *%s=",buf,arg->name);
      get_string((char *)((long)(*((int *)addr))),buf,addr);
      break;
    case TYPE_STRING_ARRAY:
      sprintf(buf,"%schar **%s=",buf,arg->name);
      break;
    default:
      sprintf(buf,"%sUNKNOWN %s=%p",buf,arg->name,addr);
  }
}

void get_string(char *str,char *buf,void *addr)
{
  int i;
  int len=strlen(str);
  char *temp = str;
  for(i=0;i<len && i<25;i++,temp+=1) {
    if(!isprint(*temp)) {
      sprintf(buf,"%s%p",buf,addr);
      return;
    }
  }
  temp=str;
  sprintf(buf,"%s\"",buf);
  if(len>25) {
    for(i=0;i<25;i++,temp+=1)
      sprintf(buf,"%s%c",buf,*temp);
    sprintf(buf,"%s...\"",buf);
    return;
  }
  sprintf(buf,"%s%s\"",buf,str);
}

void traceback(FILE *fp)
{
  int index;
  void* ebp = ebp_addr();
  void *ret;
  char buf[1000];
  while(1) {
    ret=(void*)(long)*(int *)(ebp+4);
    ebp=(void*)(long)*(int *)ebp;
    index=get_func(ret);
    if(strcmp(functions[index].name,"__libc_start_main")==0)
      return;
    get_details(buf,&functions[index],ebp);
    fprintf(fp,"%s",buf);
  }
}
