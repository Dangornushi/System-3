#include "efi.h"
#include "common.h"
#include "file.h"
#include "graphics.h"
#include "shell.h"
#include "gui.h"

#define WIDTH_PER_CH	8
#define HEIGHT_PER_CH	20
#define MAX_COMMAND_LEN	100
#define K_SPACE 52


unsigned short *command(unsigned short *s1, unsigned short *s2, int n);
int to_int(unsigned short *str);

struct CONSOLE *print(unsigned short moji[][12][8], unsigned int arfa, struct CONSOLE *c, struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color ) {
	for(int i=0;i<12;i++){
		for(int j=0;j<8;j++){
			if(moji[arfa][i][j]!=0){draw_pixel(j+c->sp,i+c->ent,color);}
		}
	}
	c->sp+=9;
	return c;
}

struct CONSOLE *putchar(unsigned short moji[][12][8], unsigned short cha, struct CONSOLE *c, struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color ) {	
	if (cha==L'\r') {}
	else if (cha==L'\n') {c->sp=0;c->ent+=13;}
	else if (cha==8) { c->sp-=9;c = print(moji,K_SPACE,c,black);c->sp-=9;}
	else if (cha==L' ') { c = print(moji,K_SPACE,c,black);}
	else if (cha==K_SPACE) { c = print(moji,K_SPACE,c,black); }
	else if (cha > 60 && cha < 96) {c = print(moji,cha-65,c,color); }//大文字
	else if (cha < 58) { c = print(moji,cha+20,c,color); }
	else if (cha < 96) { c = print(moji,cha-9,c,color); }
	else { c = print(moji,cha-71,c,color);}//小文字
	return c;
}

int icon_print(unsigned short icon[][30][24], unsigned int arfa, int sp, int ent, struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color ) {
	for(int i=0;i<30;i++){
		for(int j=0;j<24;j++){
			if(icon[arfa][i][j]!=0){draw_pixel(j+sp*8,i+ent,color);}
		}
	}
	sp++;
	return sp;
}

void cls(void) {
			int wait = 0;
			int l = 0;
			for (int x = 0;x < 1000;x++) {		
				draw_pixel(0,x,black);
				draw_pixel(1000,x,black);
			} 
			for (int y = 0;y < 1000;y++) {	
				draw_pixel(y,0,black);
				draw_pixel(y,1000,black);
			}
			while (1) {
				for (int i = 0;i < 1000;i++) {
					draw_pixel(l,i,black);
				}
				if (l > 999) {
					puts(L"");
					l=0;
					break;
				}
				l++;
				
			}
}

void dialogue_get_filename(int idx)
{
	int i;

	ST->ConOut->ClearScreen(ST->ConOut);

	puts(L"New File Name: ");
	for (i = 0; i < MAX_FILE_NAME_LEN; i++) {
		file_list[idx].name[i] = getc();
		if (file_list[idx].name[i] != L'\r')
			putc(file_list[idx].name[i]);
		else
			break;
	}
	file_list[idx].name[i] = L'\0';
}

void touch(unsigned short *file_name) {
	int idx = ls();
	
	for (int i = 0; i < MAX_FILE_NAME_LEN; i++) {
		file_list[idx].name[i] = *file_name++;
		if (file_list[idx].name[i] == L'\0')
			break;
	}
	puts(L"\r\n");
}

void pstat(void)
{
	unsigned long long status;
	struct EFI_SIMPLE_POINTER_STATE s;
	unsigned long long waitidx;

	SPP->Reset(SPP, FALSE);

	while (1) {
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput), &waitidx);
		status = SPP->GetState(SPP, &s);
		if (!status) {
			puth(s.RelativeMovementX, 8);
			puts(L" ");
			puth(s.RelativeMovementY, 8);
			puts(L" ");
			puth(s.RelativeMovementZ, 8);
			puts(L" ");
			puth(s.LeftButton, 1);
			puts(L" ");
			puth(s.RightButton, 1);
			puts(L"\r\n");
		}
	}
}

int ls(void)
{
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	unsigned long long buf_size;
	unsigned char file_buf[MAX_FILE_BUF];
	struct EFI_FILE_INFO *file_info;
	int idx = 0;
	int file_num;

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	while (1) {
		buf_size = MAX_FILE_BUF;
		status = root->Read(root, &buf_size, (void *)file_buf);
		assert(status, L"root->Read");
		if (!buf_size) break;

		file_info = (struct EFI_FILE_INFO *)file_buf;
		strncpy(file_list[idx].name, file_info->FileName, MAX_FILE_NAME_LEN - 1);
		file_list[idx].name[MAX_FILE_NAME_LEN - 1] = L'\0';

		idx++;
	}
	file_num = idx;

	root->Close(root);

	return file_num;
}

void edit(unsigned short *file_name)
{
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];
	int i = 0;
	unsigned short ch;

	while (TRUE) {
		ch = getc();

		if (ch == SC_ESC)
			break;

		putc(ch);
		file_buf[i++] = ch;

		if (ch == L'\r') {
			putc(L'\n');
			file_buf[i++] = L'\n';
		}
	}
	file_buf[i] = L'\0';

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	status = root->Open(root, &file, file_name,
			    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
			    EFI_FILE_MODE_CREATE, 0);
	assert(status, L"root->Open");

	status = file->Write(file, &buf_size, (void *)file_buf);
	assert(status, L"file->Write");

	file->Flush(file);

	file->Close(file);
	root->Close(root);
}

/*
 *
 *
*/

struct CONSOLE *le(unsigned short *file_name, unsigned short moji[][12][8], struct CONSOLE *c) {
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_data[MAX_FILE_BUF];
	unsigned short file_buf[MAX_FILE_BUF];
	unsigned short *ch = 0;

	unsigned i = 0;
	int tmp = 0;
	int idx = 0;
	int file_num;
	int sp = c->sp=0;
	int ent = c->ent+=13;
	int enter_counter = 0;

	int k = 0;
	int mode = 0;
	int number = 0;
	unsigned short inp[256];
	unsigned short buf[] = {'n','u','m','b','e','r',' ','o','f',' ','l','i','n','e','s','>','\0'};
	unsigned short *num = 0;
	unsigned short *s1 = 0;
	

	while (1) {
		unsigned short com[MAX_FILE_BUF];
		buf_size = MAX_FILE_BUF;		
		tmp=0;

		for (;tmp<MAX_COMMAND_LEN-1;tmp++) {
			ch[tmp] = getc();
			if (ch[tmp] == L'\r') {
				c->sp=0;
				c->ent += 13;
				com[i] = L'\0';
				break;
			}
			if (ch[tmp] == 8) {
				c->sp-=9;
				c = print(moji,26,c,black);
				c->sp-=9;
				ch[tmp]=0;
				i--;
			}
			else {	
				c = putchar(moji,ch[tmp],c,c->char_color);
				com[i] = ch[tmp];
				i++;
			}
		}
		i=0;
		if (!strcmp(L"l",com)) {
			while (1) {
				ch = getc();
				if (ch == L'\r') {
					number = to_int(&num)-2;
					break;
				}
				if (ch == 8) {
					c->sp-=9;
					c = print(moji,51,c,black);
					c->sp-=9;
					ch=L'\0';
				}
				else {	
					c = putchar(moji,ch,c,c->char_color);
					num = ch;
					num++;			
				}
			}
			c->sp = 0;
			c->ent+=13;
		}
		if (!strcmp(L"p", com)) {
			unsigned short read_buf[MAX_FILE_BUF];
	
			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");

			status = root->Open(root, &file, file_name, EFI_FILE_MODE_READ, 0);
			assert(status, L"root->Open");
	
			status = file->Read(file, &buf_size, (void *)read_buf);
			assert(status, L"file->Read");

			int n = 0;
			int n2 = 0;
			int return_c = 0;

			for (;read_buf[n]!=L'\0';n++, n2++) {
				if (read_buf[n]==L'\r') { 
					if (return_c == number) {
						inp[n2] = L'\0';
						break;
					}
					n2 = 0;	
					return_c++; 
				}
				inp[n2] = read_buf[n]; 
			}
			
			for (int x=0;inp[x]!=L'\0';x++) {
				c = putchar(moji,inp[x],c,c->char_color);
			}

			file->Close(file);
			root->Close(root);
		}
		if (!strcmp(L"q", com)) { 
			return c; 
		}
		if (!strcmp(L"w", com)) {
			unsigned long long status;
			struct EFI_FILE_PROTOCOL *root;
			struct EFI_FILE_PROTOCOL *file;
			unsigned long long buf_size = MAX_FILE_BUF;
			unsigned short file_buf[MAX_FILE_BUF / 2];
			int i = 0;
			unsigned short ch;

			ST->ConOut->ClearScreen(ST->ConOut);

			while (TRUE) {
				ch = getc();

				if (ch == SC_ESC)
					break;
			
				c = putchar(moji, ch, c, c->char_color);
				file_buf[i++] = ch;
			
				if (ch == 8) {
					c->sp-=9;
					c = putchar(moji, K_SPACE, c, c->back_color);
					c->sp-+18;
				}

				if (ch == L'\r') {
					c->sp=0;
					c->ent+=13;
					file_buf[i++] = L'\n';
				}
			}
			file_buf[i] = L'\0';

			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");

			status = root->Open(root, &file, file_name,
						EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
			assert(status, L"root->Open");

			status = file->Write(file, &buf_size, (void *)file_buf);
			assert(status, L"file->Write");

			file->Flush(file);

			file->Close(file);
			root->Close(root);
		}
		if (!strcmp(L"an", com)) {

			int i = 0;
			unsigned short p;

			while (TRUE) {
				p = getc();

				if (p == SC_ESC)
					break;

				putc(p);
				file_buf[i++] = p;

				if (p == L'\r') {
					putc(L'\n');
					file_buf[i++] = L'\n';
				}
			}
			file_buf[i] = L'\0';

			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");

			status = root->Open(root, &file, file_name,
						EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
						EFI_FILE_MODE_CREATE, 0);
			assert(status, L"root->Open");

			status = file->Write(file, &buf_size, (void *)file_buf);
			assert(status, L"file->Write");

			file->Flush(file);

			file->Close(file);
			root->Close(root);
		}
		if (!strcmp(L"add r", com)) {
			unsigned short read_buf[MAX_FILE_BUF];
		
			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");

			status = root->Open(root, &file, file_name, EFI_FILE_MODE_READ, 0);
			assert(status, L"root->Open");
		
			status = file->Read(file, &buf_size, (void *)read_buf);
			assert(status, L"file->Read");

			int tmp = 0;

			for (;read_buf[tmp] != L'\0';tmp++) {}
			read_buf[tmp] = L'\n';


			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");
			status = root->Open(root, &file, file_name,
						EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
						EFI_FILE_MODE_CREATE, 0);
			assert(status, L"root->Open");
			status = file->Write(file, &buf_size, (void *)read_buf);
			assert(status, L"file->Write");

			file->Flush(file);
			file->Close(file);
			root->Close(root);

			c->sp=0;
			c->ent+=13;
		}
		if (!strcmp(L"new", com)) {
			int idx = ls();
			for (int i = 0; i < MAX_FILE_NAME_LEN; i++) {
				file_list[idx].name[i] = *file_name++;
				if (file_list[idx].name[i] == L'\0')
					break;
			}
			c->sp = 0;
			c->ent+=13;
		}
		if (!strcmp(L"h", com)) {	
			unsigned long long status;
			struct EFI_FILE_PROTOCOL *root;
			struct EFI_FILE_PROTOCOL *file;
			unsigned long long buf_size = MAX_FILE_BUF;
			unsigned short file_buf[MAX_FILE_BUF / 2];

			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");

			status = root->Open(root, &file, L".help", EFI_FILE_MODE_READ, 0);
			assert(status, L"root->Open");

			status = file->Read(file, &buf_size, (void *)file_buf);
			assert(status, L"file->Read");

			for (int n=0;file_buf[n]!=L'\0';n++) {
				c = putchar(moji,file_buf[n], c, c->char_color);
			}

			file->Close(file);
			root->Close(root);
		}
		else {}
	}
}

void cat(unsigned short *file_name) {}

unsigned short *command(unsigned short *s1, unsigned short *s2, int n) {
	strncpy(s1, s2, n);
	unsigned short *s3 = s1;
	return s3;
}

int to_int(unsigned short *str) {
	int num = 0;

	while (*str != L'\0') {
		if (*str < 48 || *str > 57) {
			break;
		}
		num += *str - 48;
		num *= 10;
		str++;
	}
	num /= 10;

	return num;
}

int get(void)
{
	struct EFI_INPUT_KEY key;

	if (ST->ConIn->ReadKeyStroke(ST->ConIn, &key))  { return 1; }
	if (ST->ConIn->ReadKeyStroke(ST->ConIn, &key)) { return 0; }
	else { return 1; }
}

void proto_run(unsigned short code[128],int j, unsigned short memory[512], struct CONSOLE *c, unsigned short moji[][12][8] ) {	
	unsigned short line[128];

	int l = 0;
	int space = 0;

	for (int k=0;k<j;k++,l++) {
		if (code[k] != L'\n') {
			line[l] = code[k];
		}
		else {	
			int m = 0;

			line[l] = L'\0';
		unsigned short* op = 0;
			strncpy(op,line,4);
			
			unsigned short left[50];
			unsigned short right[50];

			for (int n = 4; line[n] != L'\0';n++) {
				if (line[n] == L' ') { 
					left[m] = L'\0';
					space = 1;
					m = 0;
				}
				else {
					if (space == 0) { left[m] = line[n]; }
					if (space == 1) { right[m] = line[n]; }
					m++;
				}
			}

			right[m] = L'\0';
			space = 0;

			int r = to_int(right);
			int le = to_int(left);

			if (!strcmp(L"mov ",op)) {
				memory[le] = r;
			}

			else if (!strcmp(L"add ", op)) {
				memory[to_int(left)] = memory[to_int(left)] + memory[to_int(right)];
			}

			else if (!strcmp(L"min ", op)) {
				memory[to_int(left)] -= memory[r];
			}

			else if (!strcmp(L"mul ", op)) { 
				memory[to_int(left)] = memory[to_int(left)] * memory[to_int(right)];
			}
			
			else if (!strcmp(L"div ", op)) { 
				memory[to_int(left)] = memory[to_int(left)] / memory[to_int(right)];
			}

			else if (!strcmp(L"msg ",op)) {
				unsigned short char_num[50];
				int index = 0;
				int add = 48;
				int tmp = 0;
				int tmp2 = 1;
				int tc = 0;
				int enter = 0;
				int lank = 0;
				unsigned short number[10] = {
					L'0',
					L'1',
					L'2',
					L'3',
					L'4',
					L'5',
					L'6',
					L'7',
					L'8',
					L'9',
				};

				for (;tmp<memory[le];tmp++,tmp2++) {
					if (tmp2 > 9) {
						if (lank>9) {
							tc++;
							lank = 0;
						}
						else {
							tc = 1;
						}
						tmp2 = 0;
						lank++;
						char_num[0] = number[lank];

						for (int i=0;i<tc;i++) {
							char_num[i+1] = L'0';
						}
					}
					else {
						char_num[lank] = number[tmp2];
					}
				}

				char_num[lank+1] = L'\0';
				tmp=0;
				for (;char_num[tmp]!=L'\0';tmp++) {
					c = putchar(moji, char_num[tmp], c, c->char_color);
				}
			}


			else if (!strcmp(L"put ",op)) {		
				putc(memory[to_int(left)]+65);
			}

			else if (!strcmp(L"jmp ",op)) {
				if (k!=to_int(right)) { 
					k = to_int(left);		
				}
			}

			else if (!strcmp(L"gec ",op)) {
				unsigned short ch;
				unsigned short num;
				int index = to_int(left);
				while (TRUE) {
					ch = getc();
					num = getc();
					putc(ch);
					if (ch == L'\r') break;
					memory[index] = to_int(&num);
					index++;
				}
			}
			l=-1;
		}
	}
}

void proto(unsigned short *file_name, struct CONSOLE *c, unsigned short moji[][12][8]) {
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	status = root->Open(root, &file, file_name, EFI_FILE_MODE_READ, 0);
	assert(status, L"root->Open");

	status = file->Read(file, &buf_size, (void *)file_buf);
	assert(status, L"file->Read");
	unsigned short memory[512];
	proto_run(file_buf,sizeof(file_buf),memory,c,moji);

	file->Close(file);
	root->Close(root);
	while (1) { if (!get()) {break;} }
	cls();
}

struct CONSOLE *proc(unsigned short *file_name, unsigned short moji[][12][8], struct CONSOLE *c) {
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	status = root->Open(root, &file, file_name, EFI_FILE_MODE_READ, 0);
	assert(status, L"root->Open");

	status = file->Read(file, &buf_size, (void *)file_buf);
	assert(status, L"file->Read");

	for (int n=0;file_buf[n]!=L'\0';n++) {
		c = putchar(moji,file_buf[n], c, c->char_color);
	}

	file->Close(file);
	root->Close(root);
	c->sp=0;
	c->ent+=13;

	return c;
}

void bse(unsigned short *file_name) {
	ST->ConOut->ClearScreen(ST->ConOut);	

	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];
	int i = 0;
	unsigned short ch;

	for (i=0;i<80;i++) {
		puts(L"-");	
	}
	i = 0;
	
	unsigned short *inp = 0;
	
	while (1) {
		*inp = getc();
		if (!strcmp(inp,L"i")) {
			
			while (TRUE) {
				ch = getc();

				if (ch == SC_ESC)
					break;
	
				putc(ch);
				file_buf[i++] = ch;
	
				if (ch == L'\r') {
					putc(L'\n');
					file_buf[i++] = L'\n';
				}
			}
		}
		if (!strcmp(inp,L"w\r")) {
			
			file_buf[i] = L'\0';
		
			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");

			status = root->Open(root, &file, file_name,
				EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
				EFI_FILE_MODE_CREATE, 0);
			assert(status, L"root->Open");

			status = file->Write(file, &buf_size, (void *)file_buf);
			assert(status, L"file->Write");

			file->Flush(file);

			file->Close(file);
			root->Close(root);
		}
		if (!strcmp(inp,L"u")) {	
			ST->ConOut->ClearScreen(ST->ConOut);
			bse(file_name);	
		}
		if (!strcmp(inp,L"q")) {break;}
	}
	puts(L"\r\n");
}

void proto_inter(struct CONSOLE *c, unsigned short moji[][12][8]) {
	unsigned char is_exit = FALSE;
	unsigned short code_data[128];
	unsigned short *command = 0;
	unsigned short memory[512];
	int j = 0;
	
	cls();
	while (!is_exit) {
		if (gets(command, MAX_COMMAND_LEN) <= 0)
			continue;
		else if (!strcmp(L"run",command)) {
			proto_run(code_data, j,memory,c,moji);	
			puts(L"\r\n");
			continue;
		}
		else if (!strcmp(L"exit",command)) {
			break;
		}
		else  {
			for (;*command!=L'\0';j++) {
				if (*command==8) {
					j-=2;
				}
				else {
					unsigned short data = *command++;
					code_data[j] = data;
				}
			}
			code_data[j] = L'\n';
			j++;
			command = NULL;
		}
	}
}

/*
 *     x
 * (10,10)__________ (10,100)
 * y  |/////////////          
 *    |/////////////
 *    |/////////////
 *    |/////////////
 *    |///////////// (100,100) 
 * (100,10)
 * */

void qube() {
	cls();
	int i = 10;
	int l = 10;
	int wait = 0;
	while (1) {
		cls();
		if (!get()) break;
		if (wait > 1000) {
			wait = 0;
			for (int x = 10;x < 100;x++) {		
				draw_pixel(10,x,white);
				draw_pixel(100,x,white);
			} 
			for (int y = 10;y < 100;y++) {	
				draw_pixel(y,10,white);
				draw_pixel(y,100,white);
			}
			for (int i = 10;i < 100;i++) {
				draw_pixel(l,i,white);
			}
			if (l > 99) {
				puts(L"");
				l=10;
			}
			l++;
		}
		wait++;
	}
	cls();

}

void pic() {
}

void bane() {
}

void rogin() {	
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};
	unsigned char is_exit = FALSE;
	while (!is_exit) {

		puts(L"              ______ \r\n");	
		puts(L"             |  __  |    ___   ____   __   _   ___\r\n");
		puts(L"             | |  | |  /  =_|  | __|  | |/ /  ||  ||\r\n");
		puts(L"             /_/  L |  |____|  |_|    |_/\\_\\  L|__||\r\n\n\n\n\n\n");
		
		
		
		
		
		puts(L"                          pass > ");
		
		if (pass_gets(com, MAX_COMMAND_LEN) <= 0)
			continue;
		else if (!strcmp(L"dango1027",com)) {
			cls();
			return;
		}
		else {
			puts(L"\n\n\n\n\n\n\n\n\n\n\                      Password is incorrect. Try agein.\r\n");
			while (1) {
				if (!get()) {
					cls();
					break;
				}
			}
			continue;
		}
	}
	return;
}

void edit_mode(unsigned short* file_name, unsigned short moji[][12][8]) {	
	/*unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];
	unsigned short file_0[MAX_FILE_BUF / 2];
	unsigned short line[MAX_FILE_BUF / 2];
	int line_c = 0;
	int n = 0;
	int sp=0;
	int ent=0;
	int i = 0;
	int space = 0;
	int ent_c = 0;
	int ents_c2 = 0;
	unsigned short *MODE = L"mode> \0";
	unsigned short ch;	
	unsigned short *inp = 0;
	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");
	status = root->Open(root, &file, file_name, EFI_FILE_MODE_READ, 0);
	assert(status, L"root->Open");
	status = file->Read(file, &buf_size, (void *)file_0);
	assert(status, L"file->Read");
	for (;L'\0'!=file_0[i];i++) {
		if (file_0[i]==L'\r'){
			ent += 12;
			file_buf[i] = L'\r';
			n++;
			file_buf[i] = L'\n';
			n++;
			line[line_c] = ent_c;
			line_c++;
			ent_c=0;
		}
		else { 
			sp = putchar(moji,file_0[i],sp,ent,console->char_color);
			file_buf[i]=file_0[i];
			ent_c++;
		}
	}
	n=i;
	file->Close(file);
	root->Close(root);
	
	for (int i=0;MODE[i]!=L'\0';i++) {
		space = putchar(moji,MODE[i],1,i+450,white);
	}
	while (1) {
		putchar(moji,K_SPACE,1,456,black);
		putchar(moji,*inp,1,456,white);
		putchar(moji,K_SPACE,10,456,black);
		putchar(moji,file_buf[n+1],10,456,console->char_color);
		*inp = getc();
		sp--;
		sp = putchar(moji,K_SPACE,sp,ent,black);
		sp--;
		sp = putchar(moji,file_buf[n-1],sp,ent,console->char_color);
		sp = print(moji,K_SPACE,sp,ent,console->char_color);
		
		sp = putchar(moji,K_SPACE,sp,ent,black);
		sp--;
		sp = putchar(moji,file_buf[n+1],sp,ent,console->char_color);	
		sp--;
		if (!strcmp(inp,L"i")) {
			sp = print(moji,K_SPACE,sp,ent,console->char_color);
			sp--;
			while (1) {
				ch = getc();
				if (ch==SC_ESC) {sp++;break;}
				else if (ch == L'\r') {
					ent += 12;
					sp = 1;
					file_buf[n] = L'\r';
					n++;
					file_buf[n] = L'\n';
					n++;
				}
				else if (ch == 8 && sp>-1) {
					sp--;
					sp = print(moji,K_SPACE,sp,ent,black);
					n--;
					ch=0;
					sp-=2;
					sp = print(moji,K_SPACE,sp,ent,console->char_color);
//					sp--;
				}
				else {	
					sp--;
					sp = print(moji,K_SPACE,sp,ent,black);
					sp--;
					sp = putchar(moji,ch,sp,ent,console->char_color);
					file_buf[n] = ch;
					n++;
					sp = print(moji,K_SPACE,sp,ent,console->char_color);
//					sp--;
				}
			}
		}
		if (!strcmp(inp,L"c")) {
			file_buf[n] = L'\0';
			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");
			status = root->Open(root, &file, file_name,
			    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
			    EFI_FILE_MODE_CREATE, 0);
			assert(status, L"root->Open");
			status = file->Write(file, &buf_size, (void *)file_buf);
			assert(status, L"file->Write");
			file->Flush(file);
			file->Close(file);
			root->Close(root);
//			le(file_name,moji, console);
			
			edit_mode(file_name, moji);
			return;
			
			/*n--;
			if (n==L'\n') { 
				ent-=12;
				n--;
			}
		}
		if (!strcmp(inp,L"h")) {n--;sp-=2;}//ents_c--;n-=ents[ents_c];}
		if (!strcmp(inp,L"l")) {n++;}//ents_c++;n+=ents[ents_c];}
		if (!strcmp(inp,L"w")) {
			file_buf[n] = L'\0';
			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");
			status = root->Open(root, &file, file_name,
			    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
			    EFI_FILE_MODE_CREATE, 0);
			assert(status, L"root->Open");
			status = file->Write(file, &buf_size, (void *)file_buf);
			assert(status, L"file->Write");
			file->Flush(file);
			file->Close(file);
			root->Close(root);
		}
		if (!strcmp(inp,L"q")) {break;}
	}
	*/
}

void __chkstk_ms() {}

void draw_window(int w, int h, int px, int py) {
	int k = 0;
	for (int i = 0;i < h;i++) {	
			if (i<20) 
				draw_pixel(k+px, i+py,black); 
			else 
				draw_pixel(k+px, i+py,white);

			if (i > h-2) {
			if (k==w) {break;}
			i=0;
			k++;
		}
	}
}

void draw_tag(int w, unsigned short word[], unsigned short moji[][12][8]) {	
	
} 

void cha(int mode, struct CONSOLE *console) {
	cls();
	int w = 8;
	int h = 12;
	int list = 0;
	static unsigned short moji[][12][8] = {
		{//A
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0},
			{1,1,1,0,0,1,1,1}, },
		{//B
			{0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,0,0},
			{1,0,0,0,0,0,1,0},
			{1,0,0,0,0,0,1,0},
			{1,0,0,0,0,0,1,0},
			{1,0,0,0,0,1,0,0},
			{1,1,1,1,1,1,0,0},
			{1,0,0,0,0,0,1,0},
			{1,0,0,0,0,0,0,1},
			{1,0,0,0,0,0,0,1},
			{1,0,0,0,0,0,0,1},
			{1,0,0,0,0,0,1,0},
			{1,1,1,1,1,1,0,0},},
		{//C
			{0,0,0,0,0,0,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{0,1,0,0,0,0,0,1}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//D
			{1,1,1,1,1,1,0,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0.0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,1,1,1,1,1,0,0}, },
		{//E
			{1,1,1,1,1,1,1,0}, 
			{1,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,1,1,1,1,1,1,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,1,1,1,1,1,1,0}, },
		{//F
			{1,1,1,1,1,1,1,1}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,1,1,1,1,1,1,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, },
		{//G
			{0,0,1,1,1,1,0,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,1,1,1}, 
			{1,0,0,0,0,0,0,1}, 
			{0,1,0,0,0,0,1,1}, 
			{0,1,0,0,0,0,1,1}, 
			{0,0,1,1,1,1,0,1}, },
		{//H
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, },
		{//I
			{1,1,1,1,1,1,1,1}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{1,1,1,1,1,1,1,1}, },
		{//J
			{1,1,1,1,1,1,1,1}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{1,0,0,1,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//K
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,1,0,0}, 
			{1,0,0,1,1,0,0,0}, 
			{1,0,1,0,0,0,0,0}, 
			{1,1,0,0,0,0,0,0}, 
			{1,1,0,0,0,0,0,0}, 
			{1,0,1,0,0,0,0,0}, 
			{1,0,0,1,1,0,0,0}, 
			{1,0,0,0,0,1,0,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,0,1}, },
		{//L
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,1,1,1,1,1,1,1}, },
		{//M
			{0,0,0,0,0,0,0,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{1,0,0,1,1,0,0,1},
			{1,0,0,1,1,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, },
		{//N
			{1,0,0,0,0,0,0,1}, 
			{1,1,0,0,0,0,0,1}, 
			{1,1,1,0,0,0,0,1}, 
			{1,0,1,1,0,0,0,1}, 
			{1,0,0,1,0,0,0,1}, 
			{1,0,0,1,1,0,0,1}, 
			{1,0,0,0,1,0,0,1}, 
			{1,0,0,0,1,1,0,1}, 
			{1,0,0,0,0,1,0,1}, 
			{1,0,0,0,0,1,1,1}, 
			{1,0,0,0,0,0,1,1}, 
			{1,0,0,0,0,0,0,1}, },
		{//O
			{0,0,1,1,1,1,0,0}, 
			{0,1,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,1,1,1,1,0,0}, },
		{//P
			{1,1,1,1,1,0,0,0}, 
			{1,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,1,0,0}, 
			{1,1,1,1,1,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,0}, },
		{//Q
			{0,0,1,1,1,1,0,0}, 
			{0,1,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,1,0,0,0,1}, 
			{1,0,0,0,1,0,0,1}, 
			{1,0,0,0,0,1,0,1}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,1,1,1,1,0,1}, },
		{//R
			{1,1,1,1,1,1,0,0}, 
			{1,0,0,0,0,1,0,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,1,0,0}, 
			{1,1,1,1,1,0,0,0}, 
			{1,0,0,0,1,0,0,0}, 
			{1,0,0,0,0,1,0,0}, 
			{1,0,0,0,0,1,0,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,0,1}, },
		{//S
			{0,0,1,1,1,1,0,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,0,0}, 
			{0,1,0,0,0,0,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,0,0,0,1,0}, 
			{0,0,0,0,0,0,1,0}, 
			{0,0,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,1,1,1,1,0,0}, },
		{//T
			{0,0,0,0,0,0,0,0}, 
			{1,1,1,1,1,1,1,1}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, },
		{//U
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,1,1,1,1,0,0}, },
		{//V
			{0,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, },
		{//W
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,1,1,0,0,1}, 
			{1,0,0,1,1,0,0,1}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, },
		{//X
			{1,0,0,0,0,0,0,1}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{1,0,0,0,0,0,0,1}, },
		{//Y
			{1,0,0,0,0,0,0,1}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, },
		{//Z
			{0,0,0,0,0,0,0,0}, 
			{1,1,1,1,1,1,1,1}, 
			{0,0,0,0,0,0,1,1}, 
			{0,0,0,0,0,1,1,0}, 
			{0,0,0,0,1,1,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,1,1,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{1,1,0,0,0,0,0,0}, 
			{1,1,1,1,1,1,1,1}, 
			{0,0,0,0,0,0,0,0}, },
		
		{//a
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0},
			{0,1,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0},
			{0,0,1,1,1,0,1,0},
			{0,1,0,0,0,1,1,0},
			{0,1,0,0,0,1,1,0},
			{0,1,0,0,0,1,1,0},
			{0,0,1,1,1,0,1,1},
		},
		{//b
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,1,1,1,0,0},
			{0,1,1,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,1,0,0,0,1,0},
			{0,1,0,1,1,1,0,0},
		},
		{//c
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,1,0},
			{0,0,1,1,1,1,0,0},
		},
		{//d
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,0,1,0},
			{0,0,1,1,1,0,1,0},
			{0,1,0,0,0,1,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,1,1,0},
			{0,0,1,1,1,0,1,0},
		},

		{//e
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,0,0,0,0,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,1,0},
			{0,0,1,1,1,1,0,0},
		},

		{//f
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,1,0,0,1,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,1,0,0,0,0},
		},

		{//g
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,0,0,0,1,1,0},
			{0,1,0,0,0,1,1,0},
			{0,0,1,1,1,0,1,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,0,1,0},
			{0,0,1,0,0,0,1,0},
			{0,0,0,1,1,1,0,0},
		},

		{//h
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,1,1,1,0,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,0,0,0,1,0,0},
		},

		{//i
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
		},

		{//j
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,1,0,0,1,0,0,0},
			{0,0,1,1,0,0,0,0},
		},

		{//k
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,1,1,0,0},
			{0,1,0,1,1,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,1,0,0,0,0},
			{0,1,0,0,1,0,0,0},
			{0,1,0,0,1,1,0,0},
			{0,1,0,0,0,0,1,0},
		},

		{//l
			{0,0,0,1,1,0,0,0},
			{0,0,1,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,1,0,1,0,0},
			{0,0,0,1,1,0,0,0},
		},

		{//m
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{1,0,1,0,1,0,0,0},
			{0,1,0,1,0,1,0,0},
			{0,1,0,1,0,1,0,0},
			{0,1,0,1,0,1,0,0},
			{0,1,0,1,0,1,1,0},
			{0,1,0,1,0,1,0,0},
		},

		{//n
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{1,0,1,1,1,1,0,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
		},

		{//o
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,0,1,1,1,1,0,0},
		},

		{//p
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,1,1,1,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
		},

		{//q
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,0,0,0,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,1,0,0},
		},

		{//r
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{1,1,0,0,0,0,0,0},
			{1,1,0,0,1,1,0,0},
			{1,1,0,1,1,1,1,0},
			{1,1,1,1,0,0,1,0},
			{1,1,1,0,0,0,0,0},
			{1,1,0,0,0,0,0,0},
			{1,1,0,0,0,0,0,0},
			{1,1,0,0,0,0,0,0},
		},

		{//s
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,1,0},
			{0,1,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,0,1,0},
			{0,1,1,1,1,1,0,0},
		},

		{//t
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{1,1,1,1,1,1,1,1},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,1,0,0},
			{0,0,0,1,1,1,0,0},
		},

		{//u
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,1,1,1,1,0},
		},

		{//v
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,1,0,0,0,1,1},
			{0,1,1,0,0,0,1,1},
			{0,1,1,0,0,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,1,1,0,0,0,0},
		},

		{//w
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,1,0,0},
			{0,1,0,1,0,1,0,0},
			{0,1,0,1,0,1,0,0},
			{0,1,0,1,0,1,0,0},
			{0,0,1,0,1,0,0,0},
			{0,0,1,0,1,0,0,0},
		},

		{//x
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,1,0,0,1,1,0},
			{0,0,1,0,0,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,1,0,0,1,0,0},
			{0,1,1,0,0,1,1,0},
		},

		{//y
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,0,1,0,0,1,1,0},
			{0,0,1,0,0,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,1,1,0,0,0,0},
			{0,1,1,0,0,0,0,0},
		},

		{//z
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,1,1,1,1,0,0},
		},

		{//SPACE
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, 
			{1,1,1,1,1,1,1,1}, },
		{	
			{0,0,0,0,0,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,0,0,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,0,0,0,0,0}, },
		{//28
			
			{0,0,0,0,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,0,1,1,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,1,1,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0}, 
			},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,0,0,1,0},
			{0,0,0,1,0,0,1,0},
			{0,0,1,0,0,1,0,0},
			{1,1,1,1,1,1,1,1},
			{0,0,1,0,0,1,0,0},
			{0,0,1,0,0,1,0,0},
			{1,1,1,1,1,1,1,1},
			{0,0,1,0,0,1,0,0},
			{0,1,0,0,1,0,0,0},
			{0,1,0,0,1,0,0,0},
			{0,0,0,0,0,0,0,0}, },
		{
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0}, },
		{//31
			
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,1},
			{0,1,1,0,0,0,0,1},
			{1,0,0,1,0,0,1,1},
			{1,0,0,1,0,1,1,0},
			{0,1,1,0,1,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,1,1,0,0,0,0},
			{0,1,1,0,0,1,1,0},
			{1,1,0,0,1,0,0,1},	
			{1,0,0,0,1,0,0,1},	
			{1,0,0,0,0,1,1,0},	

			},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0},
			{0,0,0,0,0,1,0,0},
			{0,0,0,0,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,1,1,0,1,1,0},
			{0,1,1,1,0,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,0,1,1,1,0},
			{0,1,1,0,1,1,1,0},
			{0,1,1,0,1,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,1,1,1,0,0,0},
			{0,1,1,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,1,1,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,1,1,1,1,1,0,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,1,0,0,0,1,1,0},
			{0,1,1,0,1,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,1,1,1,0},
			{0,0,0,1,1,1,1,0},
			{0,0,1,1,0,1,1,0},
			{0,0,1,1,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,1,0,0,0,0},
			{0,1,1,1,1,0,0,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,1,1,0,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,1,0},
			{0,0,1,1,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,1,1,1,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,0,1,1,1,1,0,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,1,0,0,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,1,0,0,1,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,0,0,0,0,1,0},
			{0,1,1,0,0,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,1,1,0,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,1,1,0,1,1,0},
			{0,1,1,1,0,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,0,1,1,1,0},
			{0,1,1,0,1,1,1,0},
			{0,1,1,0,1,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0},
		},
	};

	unsigned short buf[MAX_COMMAND_LEN];
	unsigned int sp = 0;
	unsigned int ent = 0;
	int n = 0;
	unsigned short *s1 = L"\0";

	while (1) {	
		unsigned short put[] = {'p','a','s','s'};
		int ind = 0;
		for (;ind<4;) {
				console = print(moji,put[ind]-97,console,white);
				ind++;
		}
			console = print(moji,28,console,white);
			while (1) { 
				buf[n] = getc();
				if (buf[n] == L'\r') {
					console->ent += 13;
					console->sp = 0;	
					buf[n] = L'\0';
					break;
				}
				if (buf[n] == 8) {
					if (sp > 2) {
						console->sp--;
						console = print(moji,51,console,black);
						console->sp--;
						n--;
						buf[n]=0;
					}
					else {}
				}
				else {	
					console = print(moji,27,console,white);
					n++;
				}
			}
		cls();
		console->sp=0;
		console->ent=0;
		n=0;
		if (!strcmp(L"dango1027",buf)) {break;}
	}
	
	unsigned short cha[] = {'s','y','s','t','e','m','3','\n','m','a','d','e',' ','b','y',' ','d','a','n','g','o','m','u','s','h','i','\n','\0'};

	for (int i=0;cha[i]!=L'\0';i++) {
		console = putchar(moji, cha[i], console, console->char_color);
	}
	
	while (1) {
		unsigned int n;

		unsigned short put[] = {'r','o','o','t', '!'};
		int ind = 0;
		for (;ind<5;) {
			console = putchar(moji,put[ind], console,console->char_color);
			ind++;
		}

		console = print(moji, 51, console, console->back_color);
		int upAndown = 0;

		for (n = 0; n < MAX_COMMAND_LEN - 1;) {
			buf[n] = getc();
			if (buf[n] == L'\r') {
				upAndown=0;
				console->ent += 13;
				console->sp = 0;
				buf[n] = L'\0';
				int tmp = 0;
				//for (;buf[tmp]!=L'\0';tmp++) {
				for (;tmp<n;tmp++) {
					console->com_his[console->comHis_c][tmp] = buf[tmp];	
				}
				console->com_his[console->comHis_c][tmp] = L'\0';
				console->comHis_c++;
				break;
			}
			if (buf[n] == 8) {
				//if (console->sp > 18) {
					console->sp-=9;
					console = putchar(moji, 52, console, console->back_color);
					puts(L"OK");
					console->sp-=18;
					n--;
					buf[n]=0;
					n--;
			//	}
				//else {}
			}
			if (buf[n] == L'%') {
					n=0;
					console->sp=0;
					for (int tmp=0;tmp<50;tmp++) { console=putchar(moji,52,console, console->back_color); }
					console->sp=0;
					int tmp = 0;
					for (;tmp<5;tmp++) {
						console = putchar(moji,put[tmp], console,console->char_color);
					}
					console->sp+=9;
					for (;console->com_his[console->comHis_c-1-upAndown][n] != L'\0';n++) {
						console=putchar(moji,console->com_his[console->comHis_c-1-upAndown][n],console,console->char_color);
						buf[n] = console->com_his[console->comHis_c-1-upAndown][n];
					}
					n--;
					console->sp-=9;
					console=putchar(moji,K_SPACE,console,console->back_color);
					console->sp-=9;
					upAndown++;
			}
			if (buf[n] == L'$') {
					n=0;
					console->sp=0;
					for (int tmp=0;tmp<50;tmp++) { console=putchar(moji,52,console, console->back_color); }
					console->sp=0;
					for (int tmp = 0;tmp<5;tmp++) {
						console = putchar(moji,put[tmp], console,console->char_color);
					}
					console->sp+=9;
					for (;console->com_his[console->comHis_c-1-upAndown][n] != L'\0';n++) {
						buf[n] = console->com_his[console->comHis_c-1-upAndown][n];
						console=putchar(moji,buf[n],console,console->char_color);
					}
					n--;
					upAndown--;
			}
					

			else {
					console=putchar(moji,buf[n],console,console->char_color);
					n++;
			}
		}
		buf[n] = L'\0';

		
		if (!strcmp(L"exit",buf))
			break;
		else if (!strcmp(L"edit ", command(s1,buf,5))) {
			cls();
			edit_mode(buf+5,moji);
			cls();
			console->sp=0;
			console->ent=0;
		}
		else if (!strcmp(L"key",buf)) {
			while (1) {
				unsigned short num = getc();
				console = print(moji,num-6, console,console->char_color);
			}	
		}
		else if (!strcmp(L"cls", buf)) {
			console->ent = 0;
			console->sp = 0;		
			int wait = 0;
			int l = 0;
			for (int x = 0;x < 1000;x++) {		
				draw_pixel(0,x,black);
				draw_pixel(1000,x,black);
			} 
			for (int y = 0;y < 1000;y++) {	
				draw_pixel(y,0,black);
				draw_pixel(y,1000,black);
			}
		
			while (1) {
				for (int i = 0;i < 1000;i++) {
					draw_pixel(l,i,black);
				}
				if (l > 999) {
					puts(L"");
					l=0;
					break;
				}
				l++;
				
			}
		}
		else if (!strcmp(L"pstat", buf))
			pstat();
		else if (!strcmp(L"ls", buf)) {
				unsigned long long status;
				struct EFI_FILE_PROTOCOL *root;
				unsigned long long buf_size;
				unsigned char file_buf[MAX_FILE_BUF];
				struct EFI_FILE_INFO *file_info;
				int idx = 0;
				int file_num;
	
				status = SFSP->OpenVolume(SFSP, &root);
				assert(status, L"SFSP->OpenVolume");

				while (1) {
					buf_size = MAX_FILE_BUF;
					status = root->Read(root, &buf_size, (void *)file_buf);
					assert(status, L"root->Read");
					if (!buf_size) break;
	
					file_info = (struct EFI_FILE_INFO *)file_buf;
					strncpy(file_list[idx].name, file_info->FileName, MAX_FILE_NAME_LEN - 1);
					file_list[idx].name[MAX_FILE_NAME_LEN - 1] = L'\0';

					for (int i=0;i<MAX_FILE_NAME_LEN-1;i++) {
						console=putchar(moji, file_list[idx].name[i],console,console->char_color);
						if (file_list[idx].name[i]==L'\0') { break; }
					}	
					idx++;
					console->sp = 0;
					console->ent+=13;
				}
				console->sp=0;
				console->ent+=13;
				file_num = idx;
				root->Close(root);
			}
		else if (!strcmp(L"proto",buf))
			proto_inter(console,moji);
		else if(!strcmp(L"qube",buf)) 
			qube();
		else if (!strcmp(L"echo ", command(s1,buf,5))) {
			unsigned short echo[100];
			for (int i=0;buf[i+5]!=L'\0';i++) {
				console= putchar(moji, buf[i+5], console, console->char_color);
			}	
			console->ent += 13;
			console->sp = 0;
		}
		else if (!strcmp(L"touch ", command(s1,buf,6))) 
			touch(buf+6);
		else if (!strcmp(L"cat ", command(s1,buf,4))) {
			unsigned long long status;
			struct EFI_FILE_PROTOCOL *root;
			struct EFI_FILE_PROTOCOL *file;
			unsigned long long buf_size = MAX_FILE_BUF;
			unsigned short file_buf[MAX_FILE_BUF / 2];

			status = SFSP->OpenVolume(SFSP, &root);
			assert(status, L"SFSP->OpenVolume");

			status = root->Open(root, &file, buf+4, EFI_FILE_MODE_READ, 0);
			assert(status, L"root->Open");

			status = file->Read(file, &buf_size, (void *)file_buf);
			assert(status, L"file->Read");

			for (int n=0;file_buf[n]!=L'\0';n++) {
				console = putchar(moji,file_buf[n], console, console->char_color);
			}

			file->Close(file);
			root->Close(root);
			console->sp=0;
			console->ent+=13;
		}

		else if (!strcmp(L"proto ",command(s1,buf,6))) {
			console->sp=0;
			console->ent=0;
			cls();
			proto(buf+6,console,moji);
		}
		else if (!strcmp(L"proc ",command(s1,buf,5))) {
			console = proc(buf+5,moji,console);
			console->sp=0;
			console->ent+=13;
		}
		else if (!strcmp(L"le ",command(s1,buf,3))) {
			console = le(buf+3,moji, console);
		}
		//else if(!strcmp(L"vse ",command(s1,buf,4))) 
		//	bse(buf+4);
		else {

			unsigned short err[] = {'e','r','r',' ','i','n','v','i','d',' ', 'c','o','m','m','a','n','d', '\0'};
			
			for (int i=0;err[i]!=L'\0';i++) { 
				console = putchar(moji,err[i],console,console->char_color); 
			}
			console = print(moji,26, console,black);
			console->ent += 13;
			console->sp = 0;
		}	
	}
}

struct CONSOLE *startup(struct CONSOLE *cons) {
	cons->back_color = black;
	cons->char_color = green;
	return cons;
}

void shell(void) {
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};
	unsigned char is_exit = FALSE;
	unsigned short *s1 = L"\0";

	//rogin();
	int mode = 0;
	struct CONSOLE c, *cons;
	cons = &c;
	cons->sp = 0;
	cons->ent = 0;
	cons->comHis_c = 0;
	cons = startup(cons);
	cha(mode, cons);
}
