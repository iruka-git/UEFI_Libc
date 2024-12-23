#include <reent.h>
#include <stdio.h>
#include <limits.h>

int __global_locale(void)	 {return 0;}
int __locale_mb_cur_max(void){return 0;}
int _strtod_r(void)		{return 0;}
int _strtol_r(void)		{return 0;}
int _strtoul_r(void)	{return 0;}
int _dtoa_r(void)		{return 0;}

int _execve(void)		{return 0;}
int _fcntl(void)		{return 0;}
int _fork(void)			{return 0;}
int _getentropy(void)	{return 0;}
int _jp2uc_l(void)		{return 0;}
int _localeconv_r(void)	{return 0;}
int _mbrtowc_r(void)	{return 0;}
int _mkdir(void)		{return 0;}
int _stat64(void)		{return 0;}
int _uc2jp_l(void)		{return 0;}
int _wait(void)			{return 0;}
int _wcrtomb_r(void)	{return 0;}
int _wcsrtombs_r(void)	{return 0;}
int abort(void)			{return 0;}
int btowc(void)			{return 0;}
int getenv(void)		{return 0;}
int nanf(void)			{return 0;}
int environ(void)		{return 0;}

int mmap(void)		    {return 0;}
int munmap(void)		{return 0;}

/*
int __sfp(void)			{return 0;}
int __sfp_lock_acquire(void)	{return 0;}
int __sfp_lock_release(void)	{return 0;}
int __sinit(void)		{return 0;}
int __svfwscanf_r(void)	{return 0;}
int _vfwscanf_r(void)	{return 0;}
*/

/*
int _sbrk(void)			{return 0;}
int _write(void)		{return 0;}
int realloc(void)		{return 0;}
int malloc(void)		{return 0;}
int _malloc_r(void)		{return 0;}
int _realloc_r(void)	{return 0;}
int _free_r(void)		{return 0;}
int _lseek(void)		{return 0;}
int _open(void)			{return 0;}
int _read(void)			{return 0;}
int _close(void)		{return 0;}
int _isatty(void)		{return 0;}
int _fstat(void)		{return 0;}
int _stat(void)			{return 0;}
int _link(void)			{return 0;}
int _unlink(void)		{return 0;}
int _gettimeofday(void)	{return 0;}
int _times(void)		{return 0;}
int _getpid(void)		{return 0;}
int _kill(void)			{return 0;}
*/
