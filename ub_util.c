#include <efi.h>
#include <efilib.h>
#include <efidef.h>

#include "ub_util.h"

EFI_SYSTEM_TABLE *global_eST = (void *) 0 ;


void ub_outchar(uchar c);
void cmd_User( void );

int  ub_puts(char *s)
{
	while(*s) {
		ub_outchar(*s++);
	}
	return 0;
}

int  ub_putsW(ushort *s)
{
	while(*s) {
		ub_outchar(*s++);
	}
	return 0;
}

void ub_putsYn(int f)
{
	if(f) {
		ub_puts("True\n");
	}else{
		ub_puts("False\n");
	}
}


/***************************************************************************/
/* Function declaration for GCC */
void basic_main();

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *sys_table) 
{
	global_eST = sys_table;
	InitializeLib(image, sys_table);  // I need It.

	cmd_User();
	
	basic_main();

	return EFI_SUCCESS;
}

#define MAX_FILE_BUF 1024

void   cmd_Files( void )
{
	EFI_GUID sfsp_guid = {0x0964e5b22, 0x6459,0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp;
	EFI_FILE_PROTOCOL *root;
	unsigned long long status;
	unsigned long buf_size = MAX_FILE_BUF;
	char file_buf[MAX_FILE_BUF];
//	unsigned short str[1024];
	
	status = global_eST->BootServices->LocateProtocol(&sfsp_guid, NULL, (void **)&sfsp);
	if (status) {
		ub_puts("error: SystemTable->BootServices->LocateProtocol\r\n");
	}else{
		ub_puts("OK: SystemTable->BootServices->LocateProtocol\r\n");
	}

	status = sfsp->OpenVolume(sfsp, &root);
	if (status) {
		ub_puts("error: sfsp->OpenVolume\r\n");
		return ;
	}
	
	ub_puts("before buf_size: ");
//	ub_puts(int_to_unicode(buf_size, 5, str), SystemTable);
	ub_puts("\r\n");
	
	ub_puts("sizeof EFI_FILE_INFO: ");
//	ub_puts(int_to_unicode(sizeof(struct EFI_FILE_INFO), 5, str), SystemTable);
	ub_puts("\r\n");
	
//	struct 
		EFI_FILE_INFO *efi;
	while (1) {
		status = root->Read(root, &buf_size, (void *)file_buf);
		ub_puts("status: root->Read: ");
//		ub_puts(int_to_unicode_hex(status, 16, str), SystemTable);
		ub_puts("\r\n");
		if (!buf_size)
			break;
		
		ub_puts("after buf_size: ");
//		ub_puts(int_to_unicode(buf_size, 5, str));
		ub_puts("\r\n");
		
		efi = (EFI_FILE_INFO *)file_buf;
		ub_puts("FileName: ");
		ub_putsW(efi->FileName);
		ub_puts("\r\n");
		
		buf_size = MAX_FILE_BUF;
	}
	
	status = root->Close(root);
	if (status) {
		ub_puts("root->Close\r\n");
		return ;
	}
	
}

void ub_backspace(void)
{
	uefi_call_wrapper(global_eST->ConOut->OutputString, 2, global_eST->ConOut, L"\b \b");
}

int ub_inchar()
{
	EFI_INPUT_KEY key;
	UINTN Index;
	int got = 0;
	global_eST->BootServices->WaitForEvent(1, &(global_eST->ConIn->WaitForKey), &Index);
	global_eST->ConIn->ReadKeyStroke(global_eST->ConIn, &key);
	got = key.UnicodeChar;
	return got;
}

int ub_c2u(ushort *pwcs, const char *s, int n)
{
	int count = 0;
	if (n != 0) {
		do {
			if ((*pwcs++ = (CHAR16) *s++) == 0)
				break;
			count++;
		} while (--n != 0);
	}
	return count;
}

int ub_putchar(int character)
{
	CHAR16 ch[2]; /* This variable needs to be an array of 2 or else an extra space prints */
	ub_c2u(ch,(char *)&character,1);

	uefi_call_wrapper(global_eST->ConOut->OutputString, 2, global_eST->ConOut, ch);

	return character;
}

void ub_outchar(uchar c)
{
	if( c == '\n' ) {
		ub_putchar( '\r' );
	}
	ub_putchar(c);

}

/***********************************************************/
uchar ub_breakcheck(void)
{
#ifdef __CONIO__
	if(kbhit())
		return getch() == CTRLC;
	else
#endif
		return 0;
}


