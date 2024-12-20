#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>

#include <sys/unistd.h>

#include "ub_util.h"

void    ub_printf(const char *format , ...)
{
	char buf[1024];
	va_list ap;

	va_start(ap,format);
	vsprintf(buf,format,ap);

	ub_puts(buf);
	
	va_end(ap);
}



void logprint(char *mesg,char *file,int line)
{
	char buf[1024];
	if(mesg==NULL) {
		mesg="";
	}
	sprintf(buf,"%s:%d: %s\n",file,line,mesg);
	ub_puts(buf);
}



void cmd_MallocTest(void)
{
	int n;
	int size;
	char *buf[32];
	char *p;
	for(n=1;n<24;n++) {
		size = 1<<(8+n);
//		size = 0x1000;
		p = malloc(size);
		buf[n] = p;
#if 0
		char buf2[1024];
		sprintf(buf2,"* malloc(%8d)=%lx\n",size,(long)p);
		ub_puts(buf2);
#else
		printf("* malloc(%8d)=%lx\n",size,(long)p);
#endif
		if(p==NULL) {
			break;
		}
	}

	for(n=n-1;n>=1;n--) {
		p = buf[n];
		printf("* %8d: free(%lx)\n",n,(long)p);
		if(p) free(p);
//		printf("ok.\n");
	}
}

void   cmd_User( void )
{
	cmd_MallocTest();
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

