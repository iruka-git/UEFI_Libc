#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <sys/unistd.h>

#include "ub_util.h"

void logprint(char *mesg,char *file,int line)
{
	char buf[1024];
	if(mesg==NULL) {
		mesg="";
	}
	sprintf(buf,"%s:%d: %s\n",file,line,mesg);
	ub_puts(buf);
}

void   cmd_User( void )
{
	char buf[100];
	int *p = malloc(0x1000);
	sprintf(buf,"malloc()= 0x%lx\n",(long)p );
	ub_puts(buf);
	
	_write(1,buf,6);
	ub_puts("\n---\n");

	printf("HEX = %lx\n",(long)p );
/*	putc('a',stdout);
	putc('b',stdout);
	putc('c',stdout);
	fflush(stdout);
*/	
	ub_puts("\n===\n");

}

#if 0
void   cmd_User( void )
{
	char buf[1024];
	strcpy(buf,"Hello,world\n");
	
	ub_puts("user: ");
	ub_puts(buf);
	
	ub_puts("cmp: aaa");
	ub_putsYn( strcmp("aaa","aaa") );
	ub_puts("cmp: aaaa");
	ub_putsYn( strcmp("aaa","aaaa") );
}
#endif

