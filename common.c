#include "efi.h"
#include "common.h"

#define MAX_STR_BUF	100

void putc(unsigned short c)
{
	unsigned short str[2] = L" ";
	str[0] = c;
	ST->ConOut->OutputString(ST->ConOut, str);
}

void puts(unsigned short *s) {
	ST->ConOut->OutputString(ST->ConOut, s);	
}

void puth(unsigned long long val, unsigned char num_digits)
{
	int i;
	unsigned short unicode_val;
	unsigned short str[MAX_STR_BUF];

	for (i = num_digits - 1; i >= 0; i--) {
		unicode_val = (unsigned short)(val & 0x0f);
		if (unicode_val < 0xa)
			str[i] = L'0' + unicode_val;
		else
			str[i] = L'A' + (unicode_val - 0xa);
		val >>= 4;
	}
	str[num_digits] = L'\0';

	puts(str);
}

unsigned short getc(void)
{
	struct EFI_INPUT_KEY key;
	unsigned long long waitidx;

	ST->BootServices->WaitForEvent(1, &(ST->ConIn->WaitForKey),
				       &waitidx);
	while (ST->ConIn->ReadKeyStroke(ST->ConIn, &key));

	return (key.UnicodeChar) ? key.UnicodeChar
		: (key.ScanCode + SC_OFS);
}

unsigned int gets(unsigned short *buf, unsigned int buf_size)
{
	unsigned int i;

	for (i = 0; i < buf_size - 1;) {
		buf[i] = getc();
		putc(buf[i]);
		//Enter
		if (buf[i] == L'\r') {
			putc(L'\n');
			break;
		}
		i++;
	}
	buf[i] = L'\0';

	return i;
}

unsigned int pass_gets(unsigned short *buf, unsigned int buf_size) {
	unsigned int i;

	for (i = 0; i < buf_size - 1;) {
		buf[i] = getc();
		putc(L'*');
		if (buf[i] == L'\r') {
			putc(L'\n');
			break;
		}
		i++;
	}
	buf[i] = L'\0';

	return i;
}

int strcmp(const unsigned short *s1, const unsigned short *s2)
{
	char is_equal = 1;

	for (; (*s1 != L'\0') && (*s2 != L'\0'); s1++, s2++) {
		if (*s1 != *s2) {
			is_equal = 0;
			break;
		}
	}

	if (is_equal) {
		if (*s1 != L'\0') {
			return 1;
		} else if (*s2 != L'\0') {
			return -1;
		} else {
			return 0;
		}
	} else {
		return (int)(*s1 - *s2);
	}
}

void strncpy(unsigned short *dst, unsigned short *src, unsigned long long n)
{
	unsigned short *end = L"\0";
	while (n--)
		*dst++ = *src++;
	*dst = *end;
	return;
}

unsigned char check_warn_error(unsigned long long status, unsigned short *message)
{
	if (status) {
		puts(message);
		puts(L":");
		puth(status, 16);
		puts(L"\r\n");
	}

	return !status;
}

void assert(unsigned long long status, unsigned short *message) {
	check_warn_error(status, message);
}

int strncmp(unsigned short *s1, unsigned short *s2, int n) {
	int len = sizeof(s1) - n;
	for (int i = 0;i < len;i++) 
		s2--;
	puts(L"\n");
	puts(s1);
	puts(L":");
	puts(s2);
	puts(L"\n");
	if (strcmp(s1,s2) == 0) return 0;
	else return 1;
}
