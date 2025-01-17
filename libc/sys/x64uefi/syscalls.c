/* Support files for GNU libc.  Files in the system namespace go here.
   Files in the C namespace (ie those that do not start with an
   underscore) go in .c.  */

#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <reent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swi.h"

#include "../../../ub_util.h"

/* Forward prototypes.  */
int	_system			(const char *);
int	_rename			(const char *, const char *);
int	_isatty			(int);
clock_t _times		(struct tms *);
int	_gettimeofday	(struct timeval *, void *);
void	_raise		(void);
int	_unlink			(const char *);
int	_link			(const char *, const char *);
int	_stat			(const char *, struct stat *);
int	_fstat			(int, struct stat *);
void *	_sbrk		(ptrdiff_t);
pid_t	_getpid		(void);
int	_kill			(int, int);
void	_exit		(int);
int	_close			(int);
int	_swiclose		(int);
int	_open			(const char *, int, ...);
int	_swiopen		(const char *, int);
int	_write			(int, const void *, size_t);
int	_swiwrite		(int, const void *, size_t);
_off_t	_lseek		(int, _off_t, int);
_off_t	_swilseek	(int, _off_t, int);
int	_read			(int, void *, size_t);
int	_swiread		(int, void *, size_t);
static int	wrap		(int);
static int	error		(int);
static int	get_errno	(void);
static int	remap_handle(int);
static int	findslot	(int);
static int	_kill_shared(int, int, int) __attribute__((__noreturn__));

typedef unsigned int uint;



/* Register name faking - works in collusion with the linker.  */
// register char * stack_ptr asm ("sp");


/* following is copied from libc/stdio/local.h to check std streams */
extern void   __sinit (struct _reent *);
#define CHECK_INIT(ptr) \
  do						\
    {						\
      if (!_REENT_IS_NULL(ptr) &&		\
	  !_REENT_CLEANUP(ptr))			\
	__sinit (ptr);				\
    }						\
  while (0)

/* Adjust our internal handles to stay away from std* handles.  */
// #define FILE_HANDLE_OFFSET (0x20)
/*
  if (fh == STDIN_FILENO)
    return monitor_stdin;
  if (fh == STDOUT_FILENO)
    return monitor_stdout;
  if (fh == STDERR_FILENO)

static int monitor_stdin;
static int monitor_stdout;
static int monitor_stderr;
*/

/* Struct used to keep track of the file position, just so we
   can implement fseek(fh,x,SEEK_CUR).  */
typedef struct
{
  int handle;
  off_t pos;
}
poslog;

/*****************************************************************
 *  おそらくAngelという ARMの開発用基板でサポートされていたSystemCall
 *  単なるダミー
 *****************************************************************
 */
do_AngelSWI (int reason, void * arg)
{
	int value = 0;

	// ・・・

	return value;
}




#define MAX_OPEN_FILES 20
static poslog openfiles [MAX_OPEN_FILES];

static int
findslot (int fh)
{
  int i;
  for (i = 0; i < MAX_OPEN_FILES; i ++)
    if (openfiles[i].handle == fh)
      break;
  return i;
}

/* Function to convert std(in|out|err) handles to internal versions.  */
static int
remap_handle (int fh)
{
  CHECK_INIT(_REENT);
  return fh;
}


static int
get_errno (void)
{
#if 1 //def ARM_RDI_MONITOR
	return do_AngelSWI (AngelSWI_Reason_Errno, NULL);
#else
  register int r0 asm("r0");
  asm ("swi %a1" : "=r"(r0) : "i" (SWI_GetErrno));
  return r0;
#endif
}

/* Set errno and return result. */
static int
error (int result)
{
  errno = get_errno ();
  return result;
}

static int
wrap (int result)
{
  if (result == -1)
    return error (-1);
  return result;
}

/* file, is a valid user file handle.
   ptr, is a null terminated string.
   len, is the length in bytes to read. 
   Returns the number of bytes *not* written. */
int
_swiread (int file, void * ptr, size_t len)
{
  int fh = remap_handle (file);
  int block[3];

  block[0] = fh;
  block[1] = (int) ptr;
  block[2] = (int) len;

  return do_AngelSWI (AngelSWI_Reason_Read, block);
}

/* file, is a valid user file handle.
   Translates the return of _swiread into
   bytes read. */
int // __attribute__((weak))
_read (int file, void * ptr, size_t len)
{
  int slot = findslot (remap_handle (file));
  int x = _swiread (file, ptr, len);

  if (x < 0)
    return error (-1);

  if (slot != MAX_OPEN_FILES)
    openfiles [slot].pos += len - x;

  /* x == len is not an error, at least if we want feof() to work.  */
  return len - x;
}

/* file, is a user file descriptor. */
off_t
_swilseek (int file, off_t ptr, int dir)
{
  _off_t res;
  int fh = remap_handle (file);
  int slot = findslot (fh);

  if (dir == SEEK_CUR)
    {
      off_t pos;
      if (slot == MAX_OPEN_FILES)
	return -1;
      pos = openfiles[slot].pos;

      /* Avoid SWI SEEK command when just querying file position. */
      if (ptr == 0)
	return pos;

      ptr += pos;
      dir = SEEK_SET;
    }

  int block[2];
  if (dir == SEEK_END)
    {
      block[0] = fh;
      ptr += do_AngelSWI (AngelSWI_Reason_FLen, block);
    }

  /* This code only does absolute seeks.  */
  block[0] = remap_handle (file);
  block[1] = ptr;
  res = do_AngelSWI (AngelSWI_Reason_Seek, block);

  if (slot != MAX_OPEN_FILES && res == 0)
    openfiles[slot].pos = ptr;

  /* This is expected to return the position in the file.  */
  return res == 0 ? ptr : -1;
}

off_t
_lseek (int file, off_t ptr, int dir)
{
  return wrap (_swilseek (file, ptr, dir));
}

/* file, is a valid internal file handle.
   Returns the number of bytes *not* written. */
int
_swiwrite (int file, const void * ptr, size_t len)
{
  int fh = remap_handle (file);
  int block[3];

  block[0] = fh;
  block[1] = (int) ptr;
  block[2] = (int) len;

// return do_AngelSWI (AngelSWI_Reason_Write, block);

// STDOUTを仮定.
 
	if(fh != STDOUT_FILENO) {
	 if(fh != STDERR_FILENO) {
		 return 0;
	 }
	}
// STDOUT以外は、未実装.
	
	char *p = ptr;
	int i;
	for(i=0;i<len;i++) {
		ub_outchar(*p++);
	}
	return len;
}

/* file, is a user file descriptor. */
int //__attribute__((weak))
_write (int file, const void * ptr, size_t len)
{
  int slot = findslot (remap_handle (file));
  int x = _swiwrite (file, ptr, len);

  if (x == -1 || x == len)
    return error (-1);

  if (slot != MAX_OPEN_FILES)
    openfiles[slot].pos += len - x;

  return len - x;
}

extern int strlen (const char *);

int
_swiopen (const char * path, int flags)
{
  int aflags = 0, fh;
  int block[3];

  int i = findslot (-1);

  if (i == MAX_OPEN_FILES)
    return -1;

  /* The flags are Unix-style, so we need to convert them.  */
#ifdef O_BINARY
  if (flags & O_BINARY)
    aflags |= 1;
#endif

  if (flags & O_RDWR)
    aflags |= 2;

  if (flags & O_CREAT)
    aflags |= 4;

  if (flags & O_TRUNC)
    aflags |= 4;

  if (flags & O_APPEND)
    {
      aflags &= ~4; /* Can't ask for w AND a; means just 'a'.  */
      aflags |= 8;
    }

#if 1 //def ARM_RDI_MONITOR
  block[0] = (int) path;
  block[2] = strlen (path);
  block[1] = aflags;

  fh = do_AngelSWI (AngelSWI_Reason_Open, block);

#else
  register int r0 asm("r0") = (int) path;
  register int r1 asm("r1") = (int) aflags;;
  asm ("swi %a3"
       : "=r"(r0)
       : "0"(r0), "r"(r1), "i" (SWI_Open));
  fh = r0;
#endif

  if (fh >= 0)
    {
      openfiles[i].handle = fh;
      openfiles[i].pos = 0;
    }

  return fh >= 0 ? fh : error (fh);
}

int
_open (const char * path, int flags, ...)
{
  return wrap (_swiopen (path, flags));
}

int
_swiclose (int file)
{
  int myhan = remap_handle (file);
  int slot = findslot (myhan);

  if (slot != MAX_OPEN_FILES)
    openfiles[slot].handle = -1;

#if 1 //def ARM_RDI_MONITOR
  return do_AngelSWI (AngelSWI_Reason_Close, & myhan);
#else
  register int r0 asm("r0") = myhan;
  asm ("swi %a2" : "=r"(r0): "0"(r0), "i" (SWI_Close));
  return r0;
#endif
}

int
_close (int file)
{
  return wrap (_swiclose (file));
}

static int
_kill_shared (int pid, int sig, int reason)
{
  (void) pid; (void) sig;
#if 1 //def ARM_RDI_MONITOR
  /* Note: The pid argument is thrown away.  */
  int block[2];
  block[1] = sig;
  block[0] = reason;
  int insn;

#if SEMIHOST_V2
  if (_has_ext_exit_extended ())
    {
      insn = AngelSWI_Reason_ReportExceptionExtended;
    }
  else
#endif
    {
      insn = AngelSWI_Reason_ReportException;
    }

#if SEMIHOST_V2
if (_has_ext_exit_extended ())
  do_AngelSWI (insn, block);
else
#endif
  do_AngelSWI (insn, (void*)block[0]);

#else
  asm ("swi %a0" :: "i" (SWI_Exit));
#endif

  __builtin_unreachable();
}

int
_kill (int pid, int sig)
{
  if (sig == SIGABRT)
    _kill_shared (pid, sig, ADP_Stopped_RunTimeError);
  else
    _kill_shared (pid, sig, ADP_Stopped_ApplicationExit);
}

void
_exit (int status)
{
  /* The same SWI is used for both _exit and _kill.
     For _exit, call the SWI with "reason" set to ADP_Stopped_ApplicationExit
     to mark a standard exit.
     Note: The RDI implementation of _kill_shared throws away all its
     arguments and all implementations ignore the first argument.  */
  _kill_shared (-1, status, ADP_Stopped_ApplicationExit);
}

pid_t
_getpid (void)
{
  return (pid_t)1;
}

#if 0
/*****************************************************************
 *   _sbrk() の実装例（１）：1MByte の固定プールから切り出す.
 *****************************************************************
 */
char mempool[0x10000];

void *
_sbrk (ptrdiff_t incr)
{
	static char * heap_end;
	char *        prev_heap_end;
	if (heap_end == NULL) {
		heap_end = mempool;
	}
	prev_heap_end = heap_end;
	heap_end += incr;

	ub_printf("  _sbrk(%8x)= %lx -  %lx\n",(int)incr,(long)prev_heap_end,(long)heap_end);
		
	return (void *) prev_heap_end;
}
#endif

#if 0
/*****************************************************************
 *   _sbrk() の実装例（２）：AllocatePool()を呼んでみる. NGでした
 *   heapが分割され、かつ若い方向のメモリーが返ってくるのでエラー
 *****************************************************************
 */
#define POOL_SIZE 0x100000
#define POOL_MARGIN 0x100

static char *heap_end=NULL;
static char *prev_heap_end=NULL;

void *_sbrk (ptrdiff_t incr)
{
	uint size = POOL_SIZE;
	if( size < (incr+POOL_MARGIN) ) {
		size = (incr+POOL_MARGIN);
		char *large_heap = ub_malloc(size);
		return (void *) large_heap;
	}
	uint rest = (uint)prev_heap_end - (uint)heap_end;
	if( incr >= rest ) {
		// 新規にメモリープールを取得.
		heap_end = ub_malloc(size);
	}
	prev_heap_end = heap_end;
	heap_end     += incr;

	return (void *) prev_heap_end;
}
#endif

#if 0

/*****************************************************************
 *   _sbrk() の実装例（３）：syscall.c にあった オリジナル版 
 *     MMU無しなら行ける。UEFIでは使えない
 *****************************************************************
 */
/* Heap limit returned from SYS_HEAPINFO Angel semihost call.  */
uint __heap_limit = 0xcafedead;

void * //__attribute__((weak))
_sbrk (ptrdiff_t incr)
{
  static char * heap_end;
  char *        prev_heap_end;

#if 0
  extern char   end asm ("end"); /* Defined by the linker.  */

  if (heap_end == NULL)
    heap_end = & end;

  prev_heap_end = heap_end;

  if ((heap_end + incr > stack_ptr)
      /* Honour heap limit if it's valid.  */
      || (__heap_limit != 0xcafedead && heap_end + incr > (char *)__heap_limit))
    {
      /* Some of the libstdc++-v3 tests rely upon detecting
	 out of memory errors, so do not abort here.  */
#if 0
      extern void abort (void);

      _write (1, "_sbrk: Heap and stack collision\n", 32);

      abort ();
#else
      errno = ENOMEM;
      return (void *) -1;
#endif
    }

#endif
  heap_end += incr;

  return (void *) prev_heap_end;
}
#endif


#if 1

/*****************************************************************
 *   _sbrk() の実装例（４）： 【 採用 】

  UEFIの AllocaltePages()を呼び出してsbrk()を実装してもらいました。

 * syscalls.c 側には実装せず、呼び出しのみです。
 * （ubは μ Basicの略）
 *****************************************************************
 */
void* ub_sbrk(unsigned long increment);

void *_sbrk (ptrdiff_t incr)
{
	return ub_sbrk(incr);
}

#endif

extern void memset (struct stat *, int, unsigned int);

int //__attribute__((weak))
_fstat (int file, struct stat * st)
{
  memset (st, 0, sizeof (* st));
  st->st_mode = S_IFCHR;
  st->st_blksize = 1024;
  return 0;
  file = file;
}

int //__attribute__((weak))
_stat (const char *fname, struct stat *st)
{
  int file;

  /* The best we can do is try to open the file readonly.  If it exists,
     then we can guess a few things about it.  */
  if ((file = _open (fname, O_RDONLY)) < 0)
    return -1;

  memset (st, 0, sizeof (* st));
  st->st_mode = S_IFREG | S_IREAD;
  st->st_blksize = 1024;
  _swiclose (file); /* Not interested in the error.  */
  return 0;
}

int //__attribute__((weak))
_link (const char *__path1 __attribute__ ((unused)), const char *__path2 __attribute__ ((unused)))
{
  errno = ENOSYS;
  return -1;
}

int
_unlink (const char *path)
{
#if 0 //def ARM_RDI_MONITOR
  int block[2];
  block[0] = (int)path;
  block[1] = strlen(path);
  return wrap (do_AngelSWI (AngelSWI_Reason_Remove, block)) ? -1 : 0;
#else
  errno = ENOSYS;
  return -1;
#endif
}

void
_raise (void)
{
  return;
}

int
_gettimeofday (struct timeval * tp, void * tzvp)
{
  struct timezone *tzp = tzvp;
  if (tp)
    {
    /* Ask the host for the seconds since the Unix epoch.  */
#if 1 //def ARM_RDI_MONITOR
      tp->tv_sec = do_AngelSWI (AngelSWI_Reason_Time,NULL);
#else
      {
	register int r0 asm("r0");
	asm ("swi %a1" : "=r" (r0): "i" (SWI_Time));
	tp->tv_sec = r0;
      }
#endif
      tp->tv_usec = 0;
    }

  /* Return fixed data for the timezone.  */
  if (tzp)
    {
      tzp->tz_minuteswest = 0;
      tzp->tz_dsttime = 0;
    }

  return 0;
}

/* Return a clock that ticks at 100Hz.  */
clock_t
_times (struct tms * tp)
{
  clock_t timeval;

#if 1 //def ARM_RDI_MONITOR
  timeval = do_AngelSWI (AngelSWI_Reason_Clock, NULL);
#else
  register int r0 asm("r0");
  asm ("swi %a1" : "=r" (r0): "i" (SWI_Clock));
  timeval = (clock_t) r0;
#endif

  if (tp)
    {
      tp->tms_utime  = timeval;	/* user time */
      tp->tms_stime  = 0;	/* system time */
      tp->tms_cutime = 0;	/* user time, children */
      tp->tms_cstime = 0;	/* system time, children */
    }

  return timeval;
};


int
_isatty (int fd)
{
#if 0 //def ARM_RDI_MONITOR
  int fh = remap_handle (fd);
  return wrap (do_AngelSWI (AngelSWI_Reason_IsTTY, &fh));
#else
  return (fd <= 2) ? 1 : 0;  /* one of stdin, stdout, stderr */
#endif
}

int
_system (const char *s)
{
#if 1 //def ARM_RDI_MONITOR
  int block[2];
  int e;

  /* Hmmm.  The ARM debug interface specification doesn't say whether
     SYS_SYSTEM does the right thing with a null argument, or assign any
     meaning to its return value.  Try to do something reasonable....  */
  if (!s)
    return 1;  /* maybe there is a shell available? we can hope. :-P */
  block[0] = (int)s;
  block[1] = strlen (s);
  e = wrap (do_AngelSWI (AngelSWI_Reason_System, block));
  if ((e >= 0) && (e < 256))
    {
      /* We have to convert e, an exit status to the encoded status of
         the command.  To avoid hard coding the exit status, we simply
	 loop until we find the right position.  */
      int exit_code;

      for (exit_code = e; e && WEXITSTATUS (e) != exit_code; e <<= 1)
	continue;
    }
  return e;
#else
  if (s == NULL)
    return 0;
  errno = ENOSYS;
  return -1;
#endif
}

int
_rename (const char * oldpath, const char * newpath)
{
#if 1 //def ARM_RDI_MONITOR
  int block[4];
  block[0] = (int) oldpath;
  block[1] = strlen(oldpath);
  block[2] = (int) newpath;
  block[3] = strlen(newpath);
  return wrap (do_AngelSWI (AngelSWI_Reason_Rename, block)) ? -1 : 0;
#else
  errno = ENOSYS;
  return -1;
#endif
}
