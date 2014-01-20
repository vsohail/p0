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
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#define BUFFER_SIZE 1000

int get_func(void *ret);
void traceback(FILE *fp);
void get_string_array(char **str_array,void *addr);
void get_string(char *str,void *addr);
void get_argument_details(const argsym_t *arg,void *ebp);
void get_details(const functsym_t *func,void *ebp);
char buf[BUFFER_SIZE];
char *temp=buf;
int get_func(void* ret)
{
  int i,index=-1;
  int min=INT_MAX;
  for(i=0;i<FUNCTS_MAX_NUM;i++) {
    if(strlen(functions[i].name)==0)
      break;
    int diff=ret-functions[i].addr;
    if(diff<min && diff>=0 && diff<=MAX_FUNCTION_SIZE_BYTES) {
      min=diff;
      index=i;
    }
  }
  return index;
}

void str_join(char *format,... )
{
  va_list args;
  va_start (args, format);
  temp+=vsnprintf(temp,BUFFER_SIZE,format,args);
  va_end (args);
}

void get_details(const functsym_t* func,void *ebp)
{
  int i;
  str_join("Function %s(",func->name);
  for(i=0;i<ARGS_MAX_NUM;++i) {
    if(strlen(func->args[i].name)==0)
      break;
    if(i>0)
      str_join(", ");
    get_argument_details(&func->args[i],ebp);
  }
  if(i==0) {
    str_join("void");
  }
  str_join("), in\n");
}

void get_argument_details(const argsym_t *arg,void* ebp)
{
  void *addr = arg->offset+ebp;
  switch (arg->type) {
    case TYPE_VOIDSTAR:
      str_join("void *%s=0v%x",arg->name,(unsigned int)(*(int *)addr));
      break;
    case TYPE_INT:
      str_join("int %s=%d",arg->name,*((int *)addr));
      break;
    case TYPE_FLOAT:
      str_join("float %s=%f",arg->name,*((float *)addr));
      break;
    case TYPE_DOUBLE:
      str_join("double %s=%f",arg->name,*((double *)addr));
      break;
    case TYPE_CHAR:
      str_join("char %s=",arg->name);
      if(isprint(*((char *)addr)))
        str_join("'%c'",*((char *)addr));
      else
        str_join("'\\%o'",*((char *)addr));
      break;
    case TYPE_STRING:
      str_join("char *%s=",arg->name);
      get_string((char *)((long)(*((int *)addr))),addr);
      break;
    case TYPE_STRING_ARRAY:
      str_join("char **%s=",arg->name);
      get_string_array((char **)((long)(*((int *)addr))),addr);
      break;
    default:
      str_join("UNKNOWN %s=%p",arg->name,addr);
  }
}

void get_string_array(char **str_array,void *addr)
{
  int i;
  str_join("{");
  for(i=0;i<3;i++) {
    if(*str_array==NULL) {
      break;
    }
    get_string(*str_array,addr);
    str_array++;
    if(i!=2 && *str_array!=NULL) {
      str_join(", ");
    }
  }
  if(i==3 && *str_array!=NULL) {
    str_join(", ...");
  }
  str_join("}");
}
void get_string(char *str,void *addr)
{
  int i,len;
  char *temp = str;
  for(i=0;;i++,temp+=1) {
    if(write(0,temp,1)<0) {
      str_join("%p",addr);
      return;
    }
    write(0,"\r",1);
    if(*temp=='\0'){
      break;
    }
    if(!isprint(*temp)) {
      str_join("%p",addr);
      return;
    }
  }
  len=strlen(str);
  temp=str;
  str_join("\"");
  if(len>25) {
    for(i=0;i<25;i++,temp+=1)
      str_join("%c",*temp);
    str_join("...\"");
    return;
  }
  else if(len>0){
    for(i=0;i<len;i++,temp+=1)
      str_join("%c",*temp);
    str_join("\"");
    return;
  }
  else {
    str_join("\"");
  }
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
    if(index==-1) {
      str_join("Function %p(...), in\n",ret);
      continue;
    }
    if(strcmp(functions[index].name,"__libc_start_main")==0)
      break;
    get_details(&functions[index],ebp);
  }
  fprintf(fp,"%s",buf);
}
