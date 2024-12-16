#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "ub_util.h"

char *itohex(char *buf,long hex)
{
	char *ret = buf;
	int i,c;
	for(i=0;i<32;i++) {
		*buf++ = ' ';
	}
	
	while(hex!=0) {
		c=(hex & 0x0f);
		if(c>=10) c=c+6;
		*--buf = c+'0';
		hex >>=4;
		hex &= 0x7fffffffffffffff;
	}
	return ret;
}

void   cmd_User( void )
{
	char buf[100];
	int *p = malloc(0x1000);
	ub_puts("malloc()= 0x");
	ub_puts(itohex( buf,(long)p ));
	ub_puts("\n");
	
	sprintf(buf,"hex = %lx\n",(long)p );
	ub_puts(buf);
	
	printf("HEX = %lx\n",(long)p );
	
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

