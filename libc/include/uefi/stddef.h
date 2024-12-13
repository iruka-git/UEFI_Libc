/*
 * 
 */

#ifndef _LIBC_stddef_H_
#define _LIBC_stddef_H_ 1

#define NULL 0

#define CHAR_BIT 8

#define _NANO_MALLOC 1

typedef unsigned short wchar_t;

#ifndef __size_t
//typedef unsigned long long __size_t;
//typedef	__size_t	size_t;

/*
typedef unsigned long long size_t;
typedef unsigned long long ptrdiff_t;
*/
typedef unsigned long size_t;
typedef unsigned long ptrdiff_t;

#endif 

#endif //_LIBC_stddef_H_
