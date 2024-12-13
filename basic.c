////////////////////////////////////////////////////////////////////////////////
// TinyBasic Plus
////////////////////////////////////////////////////////////////////////////////
//
// Authors:
//    Gordon Brandly (Tiny Basic for 68000)
//    Mike Field <hamster@snap.net.nz> (Arduino Basic) (port to Arduino)
//    Scott Lawrence <yorgle@gmail.com> (TinyBasic Plus) (features, etc)
//
// Contributors:
//          Brian O'Dell <megamemnon@megamemnon.com> (INPUT)
//    (full list tbd)

//  For full history of Tiny Basic, please see the wikipedia entry here:
//    https://en.wikipedia.org/wiki/Tiny_BASIC

// LICENSING NOTES:
//    Mike Field based his C port of Tiny Basic on the 68000
//    Tiny BASIC which carried the following license:
/*
******************************************************************
*                                                                *
*               Tiny BASIC for the Motorola MC68000              *
*                                                                *
* Derived from Palo Alto Tiny BASIC as published in the May 1976 *
* issue of Dr. Dobb's Journal.  Adapted to the 68000 by:         *
*       Gordon Brandly                                           *
*       12147 - 51 Street                                        *
*       Edmonton AB  T5W 3G8                                     *
*       Canada                                                   *
*       (updated mailing address for 1996)                       *
*                                                                *
* This version is for MEX68KECB Educational Computer Board I/O.  *
*                                                                *
******************************************************************
*    Copyright (C) 1984 by Gordon Brandly. This program may be   *
*    freely distributed for personal use only. All commercial    *
*                      rights are reserved.                      *
******************************************************************
*/
//    ref: http://members.shaw.ca:80/gbrandly/68ktinyb.html
//
//    However, Mike did not include a license of his own for his
//    version of this.
//    ref: http://hamsterworks.co.nz/mediawiki/index.php/Arduino_Basic
//
//    From discussions with him, I felt that the MIT license is
//    the most applicable to his intent.
//
//    I am in the process of further determining what should be
//    done wrt licensing further.  This entire header will likely
//    change with the next version 0.16, which will hopefully nail
//    down the whole thing so we can get back to implementing
//    features instead of licenses.  Thank you for your time.

#if 0
#include <efi.h>
#include <efilib.h>
#include <efidef.h>
#endif


#include "ub_util.h"

#define NULL 0

typedef unsigned char uchar;
typedef const    char cchar;
typedef unsigned int  uint;
typedef unsigned short ushort;
typedef uchar    byte;

#define ZZ printmsg((uchar*)("ZZ:" XSTR(__LINE__)));

void debug();
void ub_backspace(void);

////////////////////////////////////////////////////////////////////////////////
// Feature option configuration...
// Non-terse error messages
#define ERR_MSGS
#ifdef  ERR_MSGS
#define XSTR(x) STR(x)
#define STR(x) #x
#define PRINT_ERR() printmsg((uchar*)("Line:" XSTR(__LINE__)))
#endif

// This enables LOAD, SAVE, FILES commands through the Arduino SD Library
// it adds 9k of usage as well.
#define ENABLE_FILEIO 1
//#undef ENABLE_FILEIO

// this turns on "autorun".  if there's FileIO, and a file "autorun.bas",
// then it will load it and run it when starting up
//#define ENABLE_AUTORUN 1
#undef ENABLE_AUTORUN
// and this is the file that gets run
#define kAutorunFilename  "autorun.bas"

// this is the alternate autorun.  Autorun the program in the eeprom.
// it will load whatever is in the EEProm and run it
#define ENABLE_EAUTORUN 0

// size of our program ram
#define kRamSize   64*1024 /* arbitrary - not dependant on libraries */


////////////////////

////////////////////
// various other desktop-tweaks and such.

#ifndef boolean
#define boolean int
#define true 1
#define false 0
#endif


////////////////////

#ifdef ENABLE_FILEIO
// functions defined elsehwere
void   cmd_Files( void );
void   cmd_User( void );
uchar *filenameWord(void);
//static boolean sd_is_initialized = false;
#endif

// some settings based things

static boolean runAfterLoad = false;
static boolean triggerRun = false;



////////////////////////////////////////////////////////////////////////////////
// ASCII Characters
#define CR_char '\r'
#define NL    	'\n'
#define LF      0x0a
#define TAB    	'\t'
#define BELL	'\b'
#define SPACE   ' '
#define SQUOTE  '\''
#define DQUOTE  '\"'
#define CTRLC	0x03
#define CTRLH	0x08
#define CTRLS	0x13
#define CTRLX	0x18

#define ECHO_CHARS 0


static uchar program[kRamSize];

static uchar *txtpos,*list_line, *tmptxtpos;
static uchar expression_error;
static uchar *tempsp;

/***********************************************************/
// Keyword table and constants
static cchar *keywords[]= {
	"LIST",
	"LOAD",
	"NEW",
	"RUN",
	"SAVE",
	"NEXT",
	"LET",
	"IF",
	"GOTO",
	"GOSUB",
	"RETURN",
	"REM",
	"FOR",
	"INPUT",
	"PRINT",
	"POKE",
	"STOP",
	"BYE",
	"FILES",
	"MEM",
	"?",
	"\"",
	"DELAY",
	"END",
	"RSEED",
	"CHAIN",
	"USER",
	NULL
};

// by moving the command list to an enum, we can easily remove sections
// above and below simultaneously to selectively obliterate functionality.
enum {
	KW_LIST = 0,
	KW_LOAD, KW_NEW, KW_RUN, KW_SAVE,
	KW_NEXT, KW_LET, KW_IF,
	KW_GOTO, KW_GOSUB, KW_RETURN,
	KW_REM,
	KW_FOR,
	KW_INPUT, KW_PRINT,
	KW_POKE,
	KW_STOP, KW_BYE,
	KW_FILES,
	KW_MEM,
	KW_QMARK, KW_QUOTE,
	KW_DELAY,
	KW_END,
	KW_RSEED,
	KW_CHAIN,
	KW_USER,
	KW_DEFAULT /* always the final one*/
};

struct stack_for_frame {
	char frame_type;
	char for_var;
		int terminal;
		int step;
	uchar *current_line;
	uchar *txtpos;
};

struct stack_gosub_frame {
	char   frame_type;
	uchar *current_line;
	uchar *txtpos;
};

/*PUTC and GETC added to func tab */
static cchar *func_tab[]= {
	"PEEK",
	"ABS",
	"AREAD",
	"DREAD",
	"RND",
	"PUTC",
	"GETC",
	NULL
};
#define FUNC_PEEK    0
#define FUNC_ABS     1
#define FUNC_AREAD   2
#define FUNC_DREAD   3
#define FUNC_RND     4
#define FUNC_PUTC    5
#define FUNC_GETC    6
#define FUNC_UNKNOWN 7

static cchar *to_tab[]= {
	"TO",
	NULL
};

static cchar *step_tab[]= {
	"STEP",
	NULL
};

static cchar *relop_tab[]= {
	">=",
	"<>",
	">",
	"=",
	"<=",
	"<",
	"!=",
	NULL
};

#define RELOP_GE		0
#define RELOP_NE		1
#define RELOP_GT		2
#define RELOP_EQ		3
#define RELOP_LE		4
#define RELOP_LT		5
#define RELOP_NE_BANG		6
#define RELOP_UNKNOWN	7

#define HIGHLOW_HIGH    1
#define HIGHLOW_UNKNOWN 4


//#define INT_SIZE_16bit

#define STACK_SIZE (sizeof(struct stack_for_frame)*64)


typedef long long int64;

#ifdef INT_SIZE_16bit
#define MAX_UINT    0xFFFF
typedef          short int S_INT;
typedef unsigned short int U_INT;

#define LINENUM_END 0xFFFF
typedef short unsigned LINENUM;


#else
#define MAX_UINT 0xFFFFFFFFFFFFFFFFUL
typedef          long long S_INT;
typedef unsigned long long U_INT;

#define LINENUM_END 0xFFFFFFFF
typedef unsigned int LINENUM;

#endif


#define VAR_SIZE sizeof(S_INT) // Size of variables in bytes

static uchar *stack_limit;
static uchar *program_start;
static uchar *program_end;
static uchar *variables_begin;
static uchar *current_line;
static uchar *sp;

#define STACK_GOSUB_FLAG 'G'
#define STACK_FOR_FLAG 'F'

static uchar table_index;
static LINENUM linenum;

static const uchar okmsg[]            = "OK";
static const uchar whatmsg[]          = "What? ";
static const uchar howmsg[]           =	"How?";
static const uchar sorrymsg[]         = "Sorry!";
static const uchar memorymsg[]        = " bytes free.";
static const uchar breakmsg[]         = "break!";
static const uchar unimplimentedmsg[] = "Unimplemented";
static void  line_terminator(void);
static S_INT expression(void);

uchar ub_breakcheck(void);
int   ub_inchar(void);
void  ub_outchar(uchar c);

/***************************************************************************/
static void ignore_blanks(void)
{
	while(*txtpos == SPACE || *txtpos == TAB)
		txtpos++;
}

static int str_chk(cchar *s,cchar *t)
{
	while(*t) {
		if(*s!=*t) return 1;
		s++;
		t++;
	}
	return 0;
}

static int str_len(cchar *t)
{
	int len=0;
	while(*t) {
		t++;len++;
	}
	return len;
}

/***************************************************************************/
//  キーワードマッチング.
//
static void scantable(cchar **table)
{
	table_index = 0;
	while(*table) {
		if( str_chk((const char*)txtpos,*table)== 0 ) {
			txtpos += str_len(*table);  // Advance the pointer to following the keyword
			ignore_blanks();
			return;
		}
		// Next Table search start.
		table++;
		table_index++;
//		ignore_blanks();
	}
}

/***************************************************************************/
static void pushb(uchar b)
{
	sp--;
	*sp = b;
}

/***************************************************************************/
static uchar popb()
{
	uchar b;
	b = *sp;
	sp++;
	return b;
}

/***************************************************************************/
void printnum(S_INT num)
{
	int digits = 0;

	if(num < 0) {
		num = -num;
		ub_outchar('-');
	}
	do {
		pushb(num%10+'0');
		num = num/10;
		digits++;
	} while (num > 0);

	while(digits > 0) {
		ub_outchar(popb());
		digits--;
	}
}

void printUnum(unsigned int num)
{
	int digits = 0;

	do {
		pushb(num%10+'0');
		num = num/10;
		digits++;
	} while (num > 0);

	while(digits > 0) {
		ub_outchar(popb());
		digits--;
	}
}

/***************************************************************************/
static S_INT testnum(void)
{
	U_INT num = 0;
	ignore_blanks();
	while(*txtpos>= '0' && *txtpos <= '9' ) {
		// Trap overflows
		if(num >= MAX_UINT /10) {
			num = MAX_UINT;
			break;
		}

		num = num *10 + *txtpos - '0';
		txtpos++;
	}
	return	num;
}

/***************************************************************************/
static uchar print_quoted_string(void)
{
	int i=0;
	uchar delim = *txtpos;
	if(delim != '"' && delim != '\'')
		return 0;
	txtpos++;

	// Check we have a closing delimiter
	while(txtpos[i] != delim) {
		if(txtpos[i] == NL)
			return 0;
		i++;
	}

	// Print the characters
	while(*txtpos != delim) {
		ub_outchar(*txtpos);
		txtpos++;
	}
	txtpos++; // Skip over the last delimiter

	return 1;
}


/***************************************************************************/
void printmsgNoNL(const uchar *msg)
{
	while( ( *msg ) != 0 ) {
		ub_outchar( *msg++ );
	};
}

/***************************************************************************/
void printmsg(const uchar *msg)
{
	printmsgNoNL(msg);
	line_terminator();
}

void debug()
{
	printmsg((uchar*)"debug:");
	printmsg((uchar*)txtpos);
}

/****************************************************************************
 *	メモリー内容をダンプ.
 ****************************************************************************
 */
#if 0
void memdump(cchar *msg,void *ptr,int len)
{
	int i;
	uchar *p = (uchar *)ptr;
	int adr = 0;
	printf("%s:\n",msg);
	for(i=0; i<len; i++) {
		if( (i & 15) == 0 ) {
			printf("%llx",(int64) ptr);
		}
		printf(" %02x",*p);
		p++;adr++;;
		if( (i & 15) == 15 ) {
			printf("\n");
		}
	}

	if(len & 0x0f) {
		printf("\n");
	}
}
#endif

/***************************************************************************/
static void getln(char prompt)
{
	ub_outchar(prompt);
	txtpos = program_end+sizeof(LINENUM);

	while(1) {
		char c = ub_inchar();
		switch(c) {
		case NL:
		//break;
		case CR_char:
			line_terminator();
			// Terminate all strings with a NL
			txtpos[0] = NL;
			return;
		case CTRLH:
			if(txtpos == program_end)
				break;
			txtpos--;

			ub_backspace();

			break;
		default:
			// We need to leave at least one space to allow us to shuffle the line into order
			if(txtpos == variables_begin-2)
				ub_outchar(BELL);
			else {
				txtpos[0] = c;
				txtpos++;
				ub_outchar(c);
			}
		}
	}
}

/***************************************************************************/
static uchar *findline(void)
{
	uchar *line = program_start;
	while(1) {
		if(line == program_end)
			return line;

		if(((LINENUM *)line)[0] >= linenum)
			return line;

		// Add the line length onto the current address, to get to the next line;
		line += line[sizeof(LINENUM)];
	}
}

/***************************************************************************/
static void toUppercaseBuffer(void)
{
	uchar *c = program_end+sizeof(LINENUM);
	uchar quote = 0;

	while(*c != NL) {
		// Are we in a quoted string?
		if(*c == quote)
			quote = 0;
		else if(*c == '"' || *c == '\'')
			quote = *c;
		else if(quote == 0 && *c >= 'a' && *c <= 'z')
			*c = *c + 'A' - 'a';
		c++;
	}
}

/***************************************************************************/
void printline()
{
	LINENUM line_num;

	line_num = *((LINENUM *)(list_line));
	list_line += sizeof(LINENUM) + sizeof(char);

	// Output the line */
	printnum(line_num);
	ub_outchar(' ');
	while(*list_line != NL) {
		ub_outchar(*list_line);
		list_line++;
	}
	list_line++;
//#ifdef ALIGN_MEMORY
//	// Start looking for next line on even page
//	if (ALIGN_UP(list_line) != list_line)
//		list_line++;
//#endif
	line_terminator();
}

/***************************************************************************/
static S_INT expr4(void)
{
	// fix provided by Jurg Wullschleger wullschleger@gmail.com
	// fixes whitespace and unary operations
	ignore_blanks();

	if( *txtpos == '-' ) {
		txtpos++;
		return -expr4();
	}
	// end fix

	if(*txtpos == '0') {
		txtpos++;
		return 0;
	}

	if(*txtpos >= '1' && *txtpos <= '9') {
		S_INT a = 0;
		do 	{
			a = a*10 + *txtpos - '0';
			txtpos++;
		} while(*txtpos >= '0' && *txtpos <= '9');
		return a;
	}

	// Is it a function or variable reference?
	if(txtpos[0] >= 'A' && txtpos[0] <= 'Z') {
		S_INT a;
		// Is it a variable reference (single alpha)
		if(txtpos[1] < 'A' || txtpos[1] > 'Z') {
			a = ((S_INT *)variables_begin)[*txtpos - 'A'];
			txtpos++;
			return a;
		}

		// Is it a function with a single parameter
		scantable(func_tab);
		if(table_index == FUNC_UNKNOWN)
			goto expr4_error;

		uchar f = table_index;

		if(*txtpos != '(')
			goto expr4_error;

		txtpos++;
		a = expression();
		if(*txtpos != ')')
			goto expr4_error;
		txtpos++;
		switch(f) {
		case FUNC_PEEK:
			/*printf("kRamSize: %d\n",kRamSize);
			printf("sizeof(int): %d\n",sizeof(unsigned int));
			printf("Addr to peek: %d\n\n",(unsigned int)a);
			printf("Printing data at -1200/64336: %d\n",program[64336]);
			*/
			return program[(U_INT)a];
		case FUNC_ABS:
			if(a < 0)
				return -a;
			return a;
		case FUNC_GETC:
			/* Hack to make getchar work without waiting for enter key */
#if 0 //def linux
			system("/bin/stty raw -echo");
#endif
			a = ub_inchar();
#if 0 //def linux
			system("/bin/stty cooked echo");
#endif
			return a;
		case FUNC_PUTC:
			ub_outchar(a);
			return a;


		case FUNC_RND:
#if 1
			//TODO
			return 3;
#else
			return( rand() % a );
#endif
		}
	}

	if(*txtpos == '(') {
		S_INT a;
		txtpos++;
		a = expression();
		if(*txtpos != ')')
			goto expr4_error;

		txtpos++;
		return a;
	}

expr4_error:
	expression_error = 1;
	return 0;

}

/***************************************************************************/
static S_INT expr3(void)
{
	S_INT a,b;

	a = expr4();

	ignore_blanks(); // fix for eg:  100 a = a + 1

	while(1) {
		if(*txtpos == '*') {
			txtpos++;
			b = expr4();
			a *= b;
		} else if(*txtpos == '/') {
			txtpos++;
			b = expr4();
			if(b != 0)
				a /= b;
			else
				expression_error = 1;
		} else
			return a;
	}
}

/***************************************************************************/
static S_INT expr2(void)
{
	S_INT a,b;

	if(*txtpos == '-' || *txtpos == '+')
		a = 0;
	else
		a = expr3();

	while(1) {
		if(*txtpos == '-') {
			txtpos++;
			b = expr3();
			a -= b;
		} else if(*txtpos == '+') {
			txtpos++;
			b = expr3();
			a += b;
		} else
			return a;
	}
}
/***************************************************************************/
static S_INT expression(void)
{
	S_INT a,b;

	a = expr2();

	// Check if we have an error
	if(expression_error)	return a;

	scantable(relop_tab);
	if(table_index == RELOP_UNKNOWN)
		return a;

	switch(table_index) {
	case RELOP_GE:
		b = expr2();
		if(a >= b) return 1;
		break;
	case RELOP_NE:
	case RELOP_NE_BANG:
		b = expr2();
		if(a != b) return 1;
		break;
	case RELOP_GT:
		b = expr2();
		if(a > b) return 1;
		break;
	case RELOP_EQ:
		b = expr2();
		if(a == b) return 1;
		break;
	case RELOP_LE:
		b = expr2();
		if(a <= b) return 1;
		break;
	case RELOP_LT:
		b = expr2();
		if(a < b) return 1;
		break;
	}
	return 0;
}

void basic_main()
{
	uchar *start;
	uchar *newEnd;
	uchar linelen;
//	boolean isDigital;
//	boolean alsoWait = false;
//	int val;

	program_start = program;
	program_end = program_start;
	sp = program+sizeof(program);  // Needed for printnum
//#ifdef ALIGN_MEMORY
//	// Ensure these memory blocks start on even pages
//	stack_limit = ALIGN_DOWN(program+sizeof(program)-STACK_SIZE);
//	variables_begin = ALIGN_DOWN(stack_limit - 27*VAR_SIZE);
//#else
	stack_limit = program+sizeof(program)-STACK_SIZE;
	variables_begin = stack_limit - 27*VAR_SIZE;
//#endif

	// memory free
	printnum(variables_begin-program_end);
	printmsg(memorymsg);

warmstart:
	// this signifies that it is running in 'direct' mode.
	current_line = 0;
	sp = program+sizeof(program);
	printmsg(okmsg);

prompt:
	if( triggerRun ) {
		triggerRun = false;
		current_line = program_start;
		goto execline;
	}

	getln( '>' );
	toUppercaseBuffer();

	txtpos = program_end+sizeof(LINENUM);

	// Find the end of the freshly entered line
	while(*txtpos != NL) {
		txtpos++;
	}
		
	// Move it to the end of program_memory
	{
		uchar *dest;
		dest = variables_begin-1;
		while(1) {
			*dest = *txtpos;
			if(txtpos == program_end+sizeof(LINENUM))
				break;
			dest--;
			txtpos--;
		}
		txtpos = dest;
	}

	// Now see if we have a line number
	linenum = testnum();
	ignore_blanks();
	if(linenum == 0) {
		goto direct;
	}

	if(linenum == LINENUM_END) {
		PRINT_ERR();
		goto qhow;
	}

	// Find the length of what is left, including the (yet-to-be-populated) line header
	linelen = 0;
	// 改行までSkip.
	while(txtpos[linelen] != NL) {
		linelen++;
	}
	linelen++; // Include the NL in the line length
	linelen += sizeof(LINENUM)+sizeof(char); // Add space for the line number and line length

	// Now we have the number, add the line header.
//	txtpos -= 3; ???
	txtpos -= (sizeof(LINENUM)+sizeof(char)); 

//#ifdef ALIGN_MEMORY
//	// Line starts should always be on 16-bit pages
//	if (ALIGN_DOWN(txtpos) != txtpos) {
//		txtpos--;
//		linelen++;
//		// As the start of the line has moved, the data should move as well
//		uchar *tomove;
//		tomove = txtpos + 3;
//		while (tomove < txtpos + linelen - 1) {
//			*tomove = *(tomove + 1);
//			tomove++;
//		}
//	}
//#endif

//	*((unsigned short *)txtpos) = linenum;
	*( (LINENUM *) txtpos)  = linenum;
	txtpos[sizeof(LINENUM)] = linelen;


	// Merge it into the rest of the program
	start = findline();

	// If a line with that number exists, then remove it
	if(start != program_end && *((LINENUM *)start) == linenum) {
		uchar *dest, *from;
		unsigned tomove;

		from = start + start[sizeof(LINENUM)];
		dest = start;

		tomove = program_end - from;
		while( tomove > 0) {
			*dest = *from;
			from++;
			dest++;
			tomove--;
		}
		program_end = dest;
	}

	if(txtpos[sizeof(LINENUM)+sizeof(char)] == NL) // If the line has no txt, it was just a delete
		goto prompt;



	// Make room for the new line, either all in one hit or lots of little shuffles
	while(linelen > 0) {
		unsigned int tomove;
		uchar *from,*dest;
		unsigned int space_to_make;

		space_to_make = txtpos - program_end;

		if(space_to_make > linelen)
			space_to_make = linelen;
		newEnd = program_end+space_to_make;
		tomove = program_end - start;


		// Source and destination - as these areas may overlap we need to move bottom up
		from = program_end;
		dest = newEnd;
		while(tomove > 0) {
			from--;
			dest--;
			*dest = *from;
			tomove--;
		}

		// Copy over the bytes into the new space
		for(tomove = 0; tomove < space_to_make; tomove++) {
			*start = *txtpos;
			txtpos++;
			start++;
			linelen--;
		}
		program_end = newEnd;
	}
	goto prompt;

unimplemented:
	printmsg(unimplimentedmsg);
	goto prompt;

qhow:
	printmsg(howmsg);
	goto prompt;

qwhat:
	printmsgNoNL(whatmsg);
	if(current_line != NULL) {
		uchar tmp = *txtpos;
		if(*txtpos != NL)
			*txtpos = '^';
		list_line = current_line;
		printline();
		*txtpos = tmp;
	}
	line_terminator();
	goto prompt;

qsorry:
	printmsg(sorrymsg);
	goto warmstart;

run_next_statement:
	while(*txtpos == ':')
		txtpos++;
	ignore_blanks();
	if(*txtpos == NL)
		goto execnextline;
	goto interperateAtTxtpos;

direct:
	txtpos = program_end+sizeof(LINENUM);
	if(*txtpos == NL) {
		goto prompt;
	}
		
interperateAtTxtpos:
	if(ub_breakcheck()) {
		printmsg(breakmsg);
		goto warmstart;
	}

	scantable(keywords);
	
	switch(table_index) {
	case KW_DELAY: {
		goto unimplemented;
	}

	case KW_FILES:
		goto files;
	case KW_LIST:
		goto list;
	case KW_CHAIN:
		goto chain;
	case KW_USER:
		goto user;
	case KW_LOAD:
		goto load;
	case KW_MEM:
		goto mem;
	case KW_NEW:
		if(txtpos[0] != NL) {
			PRINT_ERR();
			goto qwhat;
		}
		program_end = program_start;
		goto prompt;
	case KW_RUN:
		current_line = program_start;
		goto execline;
	case KW_SAVE:
		goto save;
	case KW_NEXT:
		goto next;
	case KW_LET:
		goto assignment;
	case KW_IF:
		;
		S_INT val;
		expression_error = 0;
		val = expression();
		if(expression_error || *txtpos == NL) {
			PRINT_ERR();
			goto qhow;
		}
		if(val != 0) {
			goto interperateAtTxtpos;
		}
		goto execnextline;

	case KW_GOTO:
		expression_error = 0;
		linenum = expression();
		if(expression_error || *txtpos != NL) {
			PRINT_ERR();
			goto qhow;
		}
		current_line = findline();
		goto execline;

	case KW_GOSUB:
		goto gosub;
	case KW_RETURN:
		goto gosub_return;
	case KW_REM:
	case KW_QUOTE:
		goto execnextline;	// Ignore line completely
	case KW_FOR:
		goto forloop;
	case KW_INPUT:
		goto input;
	case KW_PRINT:
	case KW_QMARK:
		goto print;
	case KW_POKE:
		goto poke;
	case KW_END:
	case KW_STOP:
		// This is the easy way to end - set the current line to the end of program attempt to run it
		if(txtpos[0] != NL) {
			PRINT_ERR();
			goto qwhat;
		}
		current_line = program_end;
		goto execline;
	case KW_BYE:
		// Leave the basic interperater
		return;

	case KW_RSEED:
		goto rseed;

	case KW_DEFAULT:
		goto assignment;
	default:
		break;
	}

execnextline:
	if(current_line == NULL)		// Processing direct commands?
		goto prompt;
	current_line +=	 current_line[sizeof(LINENUM)];

execline:
	if(current_line == program_end) // Out of lines to run
		goto warmstart;
	txtpos = current_line+sizeof(LINENUM)+sizeof(char);
	goto interperateAtTxtpos;

input: {
		uchar var;
		S_INT value;
		ignore_blanks();
		if(*txtpos < 'A' || *txtpos > 'Z') {
			PRINT_ERR();
			goto qwhat;
		}
		var = *txtpos;
		txtpos++;
		ignore_blanks();
		if(*txtpos != NL && *txtpos != ':') {
			PRINT_ERR();
			goto qwhat;
		}
inputagain:
		tmptxtpos = txtpos;
		getln( '?' );
		toUppercaseBuffer();
		txtpos = program_end+sizeof(LINENUM);
		ignore_blanks();
		expression_error = 0;
		value = expression();
		if(expression_error)
			goto inputagain;
		((S_INT *)variables_begin)[var-'A'] = value;
		txtpos = tmptxtpos;

		goto run_next_statement;
	}

forloop: {
		uchar var;
		S_INT initial, step, terminal;
		ignore_blanks();
		if(*txtpos < 'A' || *txtpos > 'Z') {
			PRINT_ERR();
			goto qwhat;
		}
		var = *txtpos;
		txtpos++;
		ignore_blanks();
		if(*txtpos != '=') {
			PRINT_ERR();
			goto qwhat;
		}
		txtpos++;
		ignore_blanks();

		expression_error = 0;
		initial = expression();
		if(expression_error) {
			PRINT_ERR();
			goto qwhat;
		}

		scantable(to_tab);
		if(table_index != 0) {
			PRINT_ERR();
			goto qwhat;
		}

		terminal = expression();
		if(expression_error) {
			PRINT_ERR();
			goto qwhat;
		}

		scantable(step_tab);
		if(table_index == 0) {
			step = expression();
			if(expression_error) {
				PRINT_ERR();
				goto qwhat;
			}
		} else
			step = 1;
		ignore_blanks();
		if(*txtpos != NL && *txtpos != ':') {
			PRINT_ERR();
			goto qwhat;
		}


		if(!expression_error && *txtpos == NL) {
			struct stack_for_frame *f;
			if(sp + sizeof(struct stack_for_frame) < stack_limit)
				goto qsorry;

			sp -= sizeof(struct stack_for_frame);
			f = (struct stack_for_frame *)sp;
			((S_INT *)variables_begin)[var-'A'] = initial;
			f->frame_type = STACK_FOR_FLAG;
			f->for_var = var;
			f->terminal = terminal;
			f->step     = step;
			f->txtpos   = txtpos;
			f->current_line = current_line;
			goto run_next_statement;
		}
	}
	PRINT_ERR();
	goto qhow;

gosub:
	expression_error = 0;
	linenum = expression();
	if(!expression_error && *txtpos == NL) {
		struct stack_gosub_frame *f;
		if(sp + sizeof(struct stack_gosub_frame) < stack_limit)
			goto qsorry;

		sp -= sizeof(struct stack_gosub_frame);
		f = (struct stack_gosub_frame *)sp;
		f->frame_type = STACK_GOSUB_FLAG;
		f->txtpos = txtpos;
		f->current_line = current_line;
		current_line = findline();
		goto execline;
	}
	PRINT_ERR();
	goto qhow;

next:
	// Fnd the variable name
	ignore_blanks();
	if(*txtpos < 'A' || *txtpos > 'Z') {
		PRINT_ERR();
		goto qhow;
	}
	txtpos++;
	ignore_blanks();
	if(*txtpos != ':' && *txtpos != NL) {
		PRINT_ERR();
		goto qwhat;
	}

gosub_return:
	// Now walk up the stack frames and find the frame we want, if present
	tempsp = sp;
	while(tempsp < program+sizeof(program)-1) {
		switch(tempsp[0]) {
		case STACK_GOSUB_FLAG:
			if(table_index == KW_RETURN) {
				struct stack_gosub_frame *f = (struct stack_gosub_frame *)tempsp;
				current_line	= f->current_line;
				txtpos			= f->txtpos;
				sp += sizeof(struct stack_gosub_frame);
				goto run_next_statement;
			}
			// This is not the loop you are looking for... so Walk back up the stack
			tempsp += sizeof(struct stack_gosub_frame);
			break;
		case STACK_FOR_FLAG:
			// Flag, Var, Final, Step
			if(table_index == KW_NEXT) {
				struct stack_for_frame *f = (struct stack_for_frame *)tempsp;
				// Is the the variable we are looking for?
				if(txtpos[-1] == f->for_var) {
					S_INT *varaddr = ((S_INT *)variables_begin) + txtpos[-1] - 'A';
					*varaddr = *varaddr + f->step;
					// Use a different test depending on the sign of the step inCR_charement
					if((f->step > 0 && *varaddr <= f->terminal) || (f->step < 0 && *varaddr >= f->terminal)) {
						// We have to loop so don't pop the stack
						txtpos = f->txtpos;
						current_line = f->current_line;
						goto run_next_statement;
					}
					// We've run to the end of the loop. drop out of the loop, popping the stack
					sp = tempsp + sizeof(struct stack_for_frame);
					goto run_next_statement;
				}
			}
			// This is not the loop you are looking for... so Walk back up the stack
			tempsp += sizeof(struct stack_for_frame);
			break;
		default:
			//printf("Stack is stuffed!\n");
			goto warmstart;
		}
	}
	// Didn't find the variable we've been looking for
	PRINT_ERR();
	goto qhow;

assignment: {
		S_INT value;
		S_INT *var;

		if(*txtpos < 'A' || *txtpos > 'Z') {
			PRINT_ERR();
			goto qhow;
		}
		var = (S_INT *)variables_begin + *txtpos - 'A';
		txtpos++;

		ignore_blanks();

		if (*txtpos != '=') {
			PRINT_ERR();
			goto qwhat;
		}
		txtpos++;
		ignore_blanks();
		expression_error = 0;
		value = expression();
		if(expression_error) {
			PRINT_ERR();
			goto qwhat;
		}
		// Check that we are at the end of the statement
		if(*txtpos != NL && *txtpos != ':') {
			PRINT_ERR();
			goto qwhat;
		}
		*var = value;
	}
	goto run_next_statement;

poke: {
		U_INT value;
		/* both peek and poke use unsigned S_INTs (should be uint_32t) now */
		U_INT address;

		// Work out where to put it
		expression_error = 0;
		value = expression();
		if(expression_error) {
			PRINT_ERR();
			goto qwhat;
		}
		//address = (uchar *)value;
		address = value;

		// check for a comma
		ignore_blanks();
		if (*txtpos != ',') {
			PRINT_ERR();
			goto qwhat;
		}
		txtpos++;
		ignore_blanks();

		// Now get the value to assign
		expression_error = 0;
		value = expression();
		if(expression_error) {
			PRINT_ERR();
			goto qwhat;
		}
		//printf("Poke %i value %i\n",address, (uchar)value);

		/*Actually do the poke */
		*(program + address) = value;
		// Check that we are at the end of the statement
		if(*txtpos != NL && *txtpos != ':') {
			PRINT_ERR();
			goto qwhat;
		}
	}
	goto run_next_statement;

list:
	linenum = testnum(); // Retuns 0 if no line found.

	// Should be EOL
	if(txtpos[0] != NL) {
		PRINT_ERR();
		goto qwhat;
	}

	// Find the line
	list_line = findline();
	while(list_line != program_end)
		printline();
	goto warmstart;

print:
	// If we have an empty list then just put out a NL
	if(*txtpos == ':' ) {
		line_terminator();
		txtpos++;
		goto run_next_statement;
	}
	if(*txtpos == NL) {
		goto execnextline;
	}

	while(1) {
		ignore_blanks();
		if(print_quoted_string()) {
			;
		} else if(*txtpos == '"' || *txtpos == '\'') {
			PRINT_ERR();
			goto qwhat;
		} else {
			S_INT e;
			expression_error = 0;
			e = expression();
			if(expression_error) {
				PRINT_ERR();
				goto qwhat;
			}
			printnum(e);
		}

		// At this point we have three options, a comma or a new line
		if(*txtpos == ',')
			txtpos++;	// Skip the comma and move onto the next
		else if(txtpos[0] == ';' && (txtpos[1] == NL || txtpos[1] == ':')) {
			txtpos++; // This has to be the end of the print - no newline
			break;
		} else if(*txtpos == NL || *txtpos == ':') {
			line_terminator();	// The end of the print statement
			break;
		} else {
			PRINT_ERR();
			goto qwhat;
		}
	}
	goto run_next_statement;

mem:
#if 1
	// memory free
	printnum(variables_begin-program_end);
	printmsg(memorymsg);
#else
	memdump("program_start",program_start,256);
#endif
	goto run_next_statement;

	/*************************************************/
files:
	// display a listing of files on the device.
	// version 1: no support for subdirectories

#ifdef ENABLE_FILEIO
	cmd_Files();
	goto warmstart;
#else
	goto unimplemented;
#endif // ENABLE_FILEIO

	
user:
	cmd_User();
	goto warmstart;
	

chain:
	runAfterLoad = true;

load:
	// clear the program
	program_end = program_start;

	// load from a file into memory
	goto unimplemented;



save:
	// save from memory out to a file
#ifdef ENABLE_FILEIO
	{
		uchar *filename;

		// Work out the filename
		expression_error = 0;
		filename = filenameWord();
		if(expression_error) {
			PRINT_ERR();
			goto qwhat;
		}

		(void) filename;
		
		// desktop
		goto warmstart;
	}
#else // ENABLE_FILEIO
	goto unimplemented;
#endif // ENABLE_FILEIO

rseed: {
		S_INT value;

		//Get the pin number
		expression_error = 0;
		value = expression();
		if(expression_error) {
			PRINT_ERR();
			goto qwhat;
		}

#if 1
		//TODO Add actual RNG
#else // ARDUINO
		srand( value );
#endif
	    (void) value;
	
		goto run_next_statement;
	}
}

// returns 1 if the character is valid in a filename
static int isValidFnChar( char c )
{
	if( c >= '0' && c <= '9' ) return 1; // number
	if( c >= 'A' && c <= 'Z' ) return 1; // LETTER
	if( c >= 'a' && c <= 'z' ) return 1; // letter (for completeness)
	if( c == '_' ) return 1;
	if( c == '+' ) return 1;
	if( c == '.' ) return 1;
	if( c == '~' ) return 1;  // Window~1.txt

	return 0;
}

uchar * filenameWord(void)
{
	// SDL - I wasn't sure if this functionality existed above, so I figured i'd put it here
	uchar * ret = txtpos;
	expression_error = 0;

	// make sure there are no quotes or spaces, search for valid characters
	//while(*txtpos == SPACE || *txtpos == TAB || *txtpos == SQUOTE || *txtpos == DQUOTE ) txtpos++;
	while( !isValidFnChar( *txtpos )) txtpos++;
	ret = txtpos;

	if( *ret == '\0' ) {
		expression_error = 1;
		return ret;
	}

	// now, find the next nonfnchar
	txtpos++;
	while( isValidFnChar( *txtpos )) txtpos++;
	if( txtpos != ret ) *txtpos = '\0';

	// set the error code if we've got no string
	if( *ret == '\0' ) {
		expression_error = 1;
	}

	return ret;
}

/***************************************************************************/
static void line_terminator(void)
{
	ub_outchar(NL);
	ub_outchar(CR_char);
}

