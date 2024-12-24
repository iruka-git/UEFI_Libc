#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>

#include <sys/unistd.h>

#include "ub_util.h"

// ===============================================
//      malloc() を伴わないprintf()
// ===============================================
//
void    ub_printf(const char *format , ...)
{
	char buf[1024];
	va_list ap;

	va_start(ap,format);
	vsprintf(buf,format,ap);

	ub_puts(buf);
	
	va_end(ap);
}



// ===============================================
//      ZZ という通過printマクロ専用のprint
// ===============================================
//
void logprint(char *mesg,char *file,int line)
{
	char buf[1024];
	if(mesg==NULL) {
		mesg="";
	}
	sprintf(buf,"%s:%d: %s\n",file,line,mesg);
	ub_puts(buf);
}



// ===============================================
//      malloc() / free() 関数のテスト
// ===============================================
//
void cmd_MallocTest(void)
{
	int n;
	int size;
	char *buf[1024];
	char *p;
	for(n=1;n<32;n++) {
		size = 1<<(8+n);
//		size = 0x10000;
		p = malloc(size);
//		p = calloc(1,size);
		buf[n] = p;
		printf("* malloc(%8d)=%lx\n",size,(long)p);
		if(p==NULL) {
			break;
		}
		memset(p,0,size);
	}
return;
	for(n=n-1;n>=1;n--) {
		p = buf[n];
		printf("* %8d: free(%lx)\n",n,(long)p);
		if(p) free(p);
	}
}

void   cmd_User( void )
{
	cmd_MallocTest();
}
