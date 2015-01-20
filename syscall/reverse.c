#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/reg.h>
#include <errno.h>


const int long_size = sizeof(long);
void reverse(char *str)
{   int i, j;
    char temp;
    for(i = 0, j = strlen(str) - 2;
        i <= j; ++i, --j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}
void getdata(pid_t child, long addr,
             char *str, int len)
{   char *laddr;
    int i, j;
    union u {
            long val;
            char chars[long_size];
    }data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while(i < j) {
        data.val = ptrace(PTRACE_PEEKDATA,
                          child, addr + i * 8,
                          NULL);
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if(j != 0) {
        data.val = ptrace(PTRACE_PEEKDATA,
                          child, addr + i * 8,
                          NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}
void putdata(pid_t child, long addr,
             char *str, int len)
{   char *laddr;
    int i, j;
    union u {
            long val;
            char chars[long_size];
    }data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while(i < j) {
        memcpy(data.chars, laddr, long_size);
        ptrace(PTRACE_POKEDATA, child,
               addr + i * 8, data.val);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if(j != 0) {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, child,
               addr + i * 8, data.val);
    }
}
int main()
{
   pid_t child;
   child = fork();
   if(child == 0) {
      ptrace(PTRACE_TRACEME, 0, NULL, NULL);
      execl("/bin/ls", "ls", NULL);
   }
   else {
      long orig_rax;
      long params[3];
      int status;
      char *str, *laddr;
      int toggle = 0;
      while(1) {
         wait(&status);
         if(WIFEXITED(status))
             break;
//	printf("before errno is %d\n",errno);	
         orig_rax = ptrace(PTRACE_PEEKUSER,
                           child, 8 * ORIG_RAX,
                           NULL);
//	printf("Child is %d\t",child);
//	printf("errno is %d\n",errno);	
//ptrace call above failed	 
//printf("rax got is %d\n",orig_rax);
         if(orig_rax == SYS_write) {
		printf("Caught sys_write call\n");
            if(toggle == 0) {
               toggle = 1;
               params[0] = ptrace(PTRACE_PEEKUSER,
                                  child, 8 * RDI,
                                  NULL);
		
	//	printf("before errno is %d\n",errno);	

               params[1] = ptrace(PTRACE_PEEKUSER,
                                  child, 8 * RSI,
                                  NULL);
	//	printf("after errno is %d\n",errno);	
               params[2] = ptrace(PTRACE_PEEKUSER,
                                  child, 8 * RDX,
                                  NULL);
               //str = (char *)calloc((params[2]+1)*sizeof(char));
               	str = (char *)calloc((params[2]+1),sizeof(char));
	       	getdata(child, params[1], str, params[2]);
	       	printf("string get is %s",str);
	       	//printf("params[1] is %d\t",params[1]);
	       	//printf("params[2] is %d\t",params[2]);
	       	reverse(str);
		printf("after reverse the string is %s",str);
               	putdata(child, params[1], str,
                       params[2]);
            }
            else {
               toggle = 0;
            }
         }
      ptrace(PTRACE_SYSCALL, child, NULL, NULL);
      }
   }
   return 0;
}


