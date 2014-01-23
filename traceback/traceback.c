/** @file traceback.c
 *  @brief The traceback stack crawler function.
 *
 *  This file contains the traceback function for the traceback library.
 *  The function is responsible for getting the current stack context,
 *  switching to the next stack frame, identifying which function the
 *  stack frame belongs to, and then parsing the arguments, according
 *  to their type and outputing the dta extracted to the given file
 *  pointer.
 *
 *  @author Sohil Habib (snhabib)
 *  @bug No known bugs.
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
#include <errno.h>

/*
 * The size used for the buffer.
 */
#define BUFFER_SIZE 1000

/* -- Funtion Definitions -- */
int get_func(void *ret);
void traceback(FILE *fp);
void get_string_array(char **str_array,void *addr);
void get_string(char *str,void *addr);
void get_argument_details(const argsym_t *arg,void *ebp);
void get_details(const functsym_t *func,void *ebp);

/*
 * The buffer which stores the formatted output.
 */
char buf[BUFFER_SIZE];

/*
 * A global char pointer which will be used to
 * keep a seek into our buffer, so we can append to
 * it as and when we need.
 */
char *temp=buf;

/** @brief get_func identifies which function in the fucntions
 *         array the return address belongs to.
 *
 *  The function computes the minimum difference between the return address
 *  and the start address of a function, as long as this difference is positive
 *  and within range of maximum function size. If the function is found its
 *  index is returned, if not then a -1 is returned.
 *
 *  @param ret The return value obtained from stack.
 *  @return The integer value of the index or -1.
 */
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

/** @brief str_join is a neat wrapper around the snprintf function
 *         which allows the fucntion to enable appending like in sprintf.
 *
 *  The function accepts variable args which are processed as per the format
 *  specified and then input into the vsnprintf function. The return value
 *  of vsnprintf is the number of bytes it printed and this is incrememted
 *  to my buffer pointer so that the next call will append it to the same
 *  buffer. I got this implementation from stack overflow when i was
 *  searching for a wrapper for snprintf.
 *
 *  @param format The format of the variable arguments.
 *  @param ... variable arguments to vsnprintf.
 *  @retuen Void.
 */
void str_join(char *format,... )
{
  va_list args;
  va_start (args, format);
  temp+=vsnprintf(temp,BUFFER_SIZE,format,args);
  va_end (args);
}

/** @brief get_details is the function parser which takes a function from
 *         the functions array and processes all the arguments that function
 *         takes
 *
 *  The function iterates over the arguments struct for that function and calls
 *  procedures to parse those arguments one by one till all the arguments have
 *  been processed. It also formats the buffer accordingly and prints void if
 *  the function has no arguments
 *
 *  @param func the function to process from the functions array
 *  @param ebp the ebp value of the current stack frame
 *  @return Void.
 */
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

/** @brief get_argument_details is used to extract data from the args struct for
 *         each argument.
 *
 *  This function takes in an argument struct and then based on the type of the
 *  argument, it treats it as per the requirements. If it is unknown it just
 *  goes forward and prints its type as unknown and prints its address.
 *  The string and string array tpes are handled with their own separate
 *  functions.
 *
 *  @param arg The argument struct of an argument for the corresponding
 *             function.
 *  @param the ebp of the current stack frame to calculate argument offset.
 *  @return Void.
 */
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

/** @brief get_string_array handles string arrays and formats
 *         the buffer to display all the strings it contains in the array
 *
 *  The function takes a char ** as input and checks the array for at most
 *  4 values .The array elements are considered valid/invalid as per the
 *  handout specifications and the strings extracted are passed on to 
 *  the get_string function to be processed.
 *
 *  @param str_array The string array to be processed.
 *  @param addr the address on stack of the string array
 *  @return Void.
 */
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

/** @brief get_string is used to validate and process a particular string
 *
 *  Each byte of the string is first checked to be a valid memory location with
 *  the help of the write syscall and when a '\0' is encountered, the string
 *  is processed based on its length. The string is also checked for
 *  printable/unprintable characters and the appropriate formatting is
 *  done as per the handout
 *
 *  @param str the string to be processed
 *  @param addr the memory address of the string
 *  @return Void.
 */
void get_string(char *str,void *addr)
{
  int i,len;
  char *temp = str;
  for(i=0;;i++,temp+=1) {
    write(0,temp,1);
    if(errno==EFAULT) {
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

/** @brief traceback is the main routine that does the stack crawling
 *
 *  The function accespts uses an assembly function to get the current ebp,
 *  and then uses it to change stack frames, identify the function for each
 *  frame, extract the arguments, process them, and finally store the
 *  details in a buf, which is then written to the file pointer
 *  provided to traceback.
 *
 *  @param fp the file to write the stack crawling information into.
 *  @return Void.
 */
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
