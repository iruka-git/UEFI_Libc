/* dummy file for external tools to use.  Real file is created by
   newlib configuration. */


#ifndef _LIBC_NEWLIB_H_
#define _LIBC_NEWLIB_H_ 1

//#define _GCC_LIMITS_H_

typedef unsigned int wint_t;

#define _LIBC

#define _LDBL_EQ_DBL

void logprint(char *mesg,char *file,int line);

#define ZZ  logprint("ZZ:",__FILE__,__LINE__);



#endif //_LIBC_NEWLIB_H_
