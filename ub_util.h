#ifndef _ub_util_H_
#define _ub_util_H_

typedef unsigned char uchar;
typedef const    char cchar;
typedef unsigned int  uint;
typedef unsigned short ushort;


int     ub_puts(char *s);
int     ub_putsW(ushort *s);
void	cmd_Files( void );
void	ub_backspace(void);
int		ub_inchar();
int		ub_c2u(ushort *pwcs, const char *s, int n);
int     ub_putchar(int character);
void	ub_outchar(uchar c);
uchar	ub_breakcheck(void);

void	*ub_malloc(uint size);

void    ub_printf(const char *fmt , ...);

#endif 

