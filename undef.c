#include <reent.h>
#include <stdio.h>
#include <limits.h>



//extern 
struct _reent *_impure_ptr __ATTRIBUTE_IMPURE_PTR__;

void _malloc_r(void)
{}

void _strerror_r(void)
{}

int siprintf(char *a, const char *b, ...)
{
	(void)a;
	(void)b;
	int c;
	c = INT_MAX - 10;
	(void)c;
	
	return 0;
}

	
