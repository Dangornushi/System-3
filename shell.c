#include "efi.h"
#include "common.h"
#include "file.h"
#include "graphics.h"
#include "shell.h"
#include "gui.h"

#define WIDTH_PER_CH	8
#define HEIGHT_PER_CH	20
#define MAX_COMMAND_LEN	100
#define K_SPACE 26



int to_int(unsigned short *str);

struct SYSTEM3*print(unsigned short moji[][12][8], unsigned int arfa, struct SYSTEM3 *system3, struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color ) {
	for(int i=0;i<12;i++){
		for(int j=0;j<8;j++){
			if(moji[arfa][i][j]!=0){draw_pixel(j+system3->cons->sp*8,i+system3->cons->ent,color);}
		}
	}
	system3->cons->sp++;
	return system3;
}
 struct SYSTEM3*putchar(unsigned short moji[][12][8], unsigned short cha, struct SYSTEM3 *system3, struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color ) {	
	if (cha==L'\r') {print(moji,26,system3,black);}
	else if (cha==L'\n') {system3->cons->sp=0;system3->cons->ent+=12;}
	else if (cha==8) { system3->cons->sp--;print(moji,K_SPACE,system3,black);system3->cons->sp--;}
	else if (cha==L' ') { print(moji,K_SPACE,system3,black);}
	else if (cha==K_SPACE) { print(moji,K_SPACE,system3,black);}
	//else if (cha > 60 && cha < 96) {print(moji,12/*cha-65*/,system3,color); }
	else if (cha < 58) { print(moji,cha-3,system3,color); }
	else if (cha < 96) { print(moji,0,system3,color); }//記号
	else { print(moji,cha-97,system3,color);}
	return system3;
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
		putc(file_list[idx].name[i]);
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
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput),
					       &waitidx);
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

struct SYSTEM3*le(unsigned short *file_name, unsigned short moji[][12][8], struct SYSTEM3 *system3) {
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF];
	unsigned short read_buf[MAX_FILE_BUF];

	int i = 0;
	unsigned short *ch;
	int idx = 0;
	int file_num;
	int sp = system3->cons->sp=0;
	int ent = system3->cons->ent+=12;

	int k = 0;
	int number = 0;
	unsigned short inp[256];
	unsigned short buf[] = {'n','u','m','b','e','r',' ','o','f',' ','l','i','n','e','s','>','\0'};
	unsigned short *num = L'\0';

	//warning msg
	for (int o=0;;o++) {
		if (buf[o]==L'\0') { break; }
		else putchar(moji, buf[o], system3 ,green);
	}

	//line num
	while (TRUE) {
		ch = getc();
		if (ch == L'\r') {
			number = to_int(&num)-2;
			break;
		}
		if (ch == 8) {
			system3->cons->sp--;
			print(moji,26,system3,black);
			system3->cons->sp--;
			ch=L'\0';
		}
		else {	
			putchar(moji,ch,system3,green);
			num = ch;
			num++;
			
		}

	}

	system3->cons->sp = 0;
	system3->cons->ent+=12;

	//line h
	
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
			n2 = 0;	
			if (return_c == number) {
				inp[n2] = L'\0';
				break;
			}
			return_c++; 
		}
		inp[n2] = read_buf[n]; 
	}
	
	file->Close(file);
	root->Close(root);
	
	for (int x=0;inp[x]!=L'\0';x++) {
		c = putchar(moji,inp[x],c,green);
	}
	
	system3->cons->sp-=2;
	
	i = n2;
	
	while (TRUE) {
		c = putchar(moji,K_SPACE,c,black);
		system3->cons->sp--;
		c = putchar(moji,inp[i-1],c,green);
	
		print(moji,K_SPACE,c,green);
		
		c = putchar(moji,K_SPACE,c,black);
		system3->cons->sp-=2;
	
		ch = getc();
	
		if (ch == L'\r') {
			system3->cons->sp=0;
			system3->cons->ent+=12;
			break;
		}
	
		if (ch == 8) {
			system3->cons->sp--;
			print(moji,26,c,black);
			system3->cons->sp-=2;
			i--;
			ch=0;
		}
	
		else {	
			c = putchar(moji,ch,c,green);
			system3->cons->sp--;
			inp[i] = ch;
			i++;
		}

	}

	system3->cons->sp = 0;
	system3->cons->ent+=12;

	inp[i] = L'\0';

	i = 0;

	for (int i = 0;i<MAX_FILE_BUF;i++) {
		if (i != number) {}

		else {
			for (int s=0;inp[s]!=L'\0';s++) { 
				file_buf[i] = inp[s];
				i++;
			}
		}
	}

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

	system3->cons->sp = 0;
	system3->cons->ent = ent+=12;

	return c;
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

void proto_run(unsigned short code[128],int j, unsigned short memory[512]) {	
	unsigned short line[128];
	unsigned short left[50];
	unsigned short right[50];

	int l = 0;

	for (int k=0;k<j;k++,l++) {
		if (code[k] != L'\n') {
			line[l] = code[k];
		}
		else {	
			unsigned short* op = 0;
			int space = 0;
			int m = 0;

			line[l] = L'\0';
			strncpy(op,line,4);
			for (int n = 4; line[n] != L'\0';n++) {
				if (line[n] == L' ') { 
					space = 1;
					m = 0;
					continue;
				}
				if (space == 0) left[m] = line[n]; 
				if (space == 1) right[m] = line[n];
				m++;
			}
			if (!strcmp(L"mov ",op)) {
				memory[to_int(left)] = to_int(right);
			}
			
			if (!strcmp(L"msg ",op)) {		
				putc(memory[to_int(left)]/10+48);
				int f_place = memory[to_int(left)]-((memory[to_int(left)]/10)*10);
				putc(f_place+48);
			}


			if (!strcmp(L"put ",op)) {		
				putc(memory[to_int(left)]+65);
			}

			if (!strcmp(L"jmp ",op)) {
				if (k!=to_int(right)) { 
					k = to_int(left);		
				}
			}

			if (!strcmp(L"gec ",op)) {
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

void proto(unsigned short *file_name) {
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
	proto_run(file_buf,sizeof(file_buf),memory);

	file->Close(file);
	root->Close(root);
	while (1) { if (!get()) {break;} }
	cls();
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

void proto_inter() {
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
			proto_run(code_data, j,memory);	
			puts(L"\r\n");
			continue;
		}
		else if (!strcmp(L"exit",command)) {
			break;
		}
		else  {
			for (;*command!=L'\0';j++) {
				unsigned short data = *command++;
				code_data[j] = data;
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
			sp = putchar(moji,file_0[i],sp,ent,green);
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
		putchar(moji,file_buf[n+1],10,456,green);

		*inp = getc();

		sp--;
		sp = putchar(moji,K_SPACE,sp,ent,black);
		sp--;
		sp = putchar(moji,file_buf[n-1],sp,ent,green);

		print(moji,K_SPACE,sp,ent,green);
		
		sp = putchar(moji,K_SPACE,sp,ent,black);
		sp--;
		sp = putchar(moji,file_buf[n+1],sp,ent,green);	

		sp--;

		if (!strcmp(inp,L"i")) {
			print(moji,K_SPACE,sp,ent,green);
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
					print(moji,K_SPACE,sp,ent,black);
					n--;
					ch=0;
					sp-=2;
					print(moji,K_SPACE,sp,ent,green);
//					sp--;
				}
				else {	
					sp--;
					print(moji,K_SPACE,sp,ent,black);
					sp--;
					sp = putchar(moji,ch,sp,ent,green);
					file_buf[n] = ch;
					n++;
					print(moji,K_SPACE,sp,ent,green);
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

//			le(file_name,moji, system3);
			
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

/*
struct COSNOLE *gui_mode(unsigned short moji[][12][8], struct SYSTEM3 *system3) {
	unsigned long long status;
	struct EFI_SIMPLE_POINTER_STATE s;
	int px = 500, py = 500;
	unsigned long long waitidx;
	int file_num;
	int idx;
	unsigned char prev_lb = s.LeftButton;
	unsigned char prev_rb = s.RightButton, executed_rb;

	int wait = 0;
	int l = 0;


	static unsigned short icon[][30][24] = {
		{
			{0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{0,0,0,0,0,1,1,1,0,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,},
			{0,0,0,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,1,},
			{0,0,0,1,1,1,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0,1,1,1,},
			{0,0,1,1,1,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0,1,1,1,},
			{0,1,1,1,0,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
		},
		{
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,1,0,0,0,0,1,0,0,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
		},
		{
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,1,1,},
			{1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,1,1,0,1,1,0,0,0,0,0,1,0,1,1,1,1,0,1,1,1,},
			{1,1,1,1,1,0,0,0,0,0,0,1,1,0,0,1,0,0,0,1,0,1,1,1,},
			{1,1,1,1,1,0,0,0,1,0,1,0,0,1,0,1,0,0,0,1,0,1,1,1,},
			{1,1,1,0,1,1,0,1,1,0,1,0,0,1,0,1,0,0,0,1,0,1,1,1,},
			{1,1,1,0,0,1,1,1,0,0,0,1,1,0,0,1,0,0,0,1,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
			{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,},
		},
	};

	int mode = 0;
	struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color;
	struct WINDOW {
		int w;
		int h;
	};
	struct WINDOW disk_window;

	SPP->Reset(SPP, FALSE);
	file_num = ls();

	//背景とタスクバー
	for (int i = 0;i < 500;i++) {
			if (i<439) {draw_pixel(l,i,blue);}
			else draw_pixel(l,i,black);
			if (i > 498) {
				if (l==700) 
					break;
				i=0;
				l++;
			}
	}	
	l=0;
	int k = 0;
	int old_px = 0;
	int old_py = 0;
	int old_win_w = 0;
	int old_win_h = 0;
	int open = 0;
	int win_c = 0;

	while (1) {
		int sp = 0;
		int ent = 0;

		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput), &waitidx);
		status = SPP->GetState(SPP, &s);

		/* 右クリックの実行済フラグをクリア
		executed_rb = FALSE;
			//マウスカーソル描画 
			for ( int i=0;i<7;i++) { 
				if ( px<disk_window.w && px>disk_window.w-300 && py<disk_window.h && py>disk_window.h-200 ) {
					if (py > disk_window.h-180)
						color = white;
					else if (py > disk_window.h-200)
						color = black;
				}
				else {
					if (py>439) color = black;
					if (py<439 ) color = blue;
				}
				if (i<5)draw_pixel(px+1, py+i+1, color);
				draw_pixel(px+i+1, py+i, color);
			}
		
			//ディスクモード
			struct RECT disk = {10, 10, 20, 30};
			system3->cons->sp = icon_print(icon,0,1,8,white);	

			//system3モード
			struct RECT cons = {30, 10, 20, 30};
			system3->cons->sp = icon_print(icon,2,8,8,white);	
			
			k=0;
			int j=0;
			if (mode == 1) {
				if (win_c>1 && px+300 > old_win_w && py+200 > old_win_h) {
					mode=0;
					px = 0;
					py = 0;
				}
				else {
					for (int i = 0;i < 200;i++,j++) {
						if (py+i>439) draw_pixel(px+k, py+i,black); 
						else if (py+i<439) draw_pixel(px+k, py+i,blue);
					
						if (i > 198) {
							if (k==300) {break;}
							i=0;
							k++;
						}
					}	
				}
			}	
			
			/* マウスカーソル座標更新 
			px += s.RelativeMovementX >> 13;
			if (px < 0)
				px = 0;
			else if (GOP->Mode->Info->HorizontalResolution <= (unsigned int)px)
				px = GOP->Mode->Info->HorizontalResolution - 1;
			py += s.RelativeMovementY >> 13;
			if (py < 0)
				py = 0;
			else if (GOP->Mode->Info->VerticalResolution <= (unsigned int)py)
				py = GOP->Mode->Info->VerticalResolution - 1;

			//マウスカーソル描画 
			for ( int i=0;i<7;i++) { 
				if (i<5)draw_pixel(px+1, py+i+1, green);
				draw_pixel(px+i+1, py+i, green);
			}

			if ((is_in_rect(px, py, disk) && s.LeftButton) || mode == 1 ) {
				//draw_window(300, 200, px, py);
				if (mode!=1) win_c++;
				k = 0;
				for (int i = 0;i < 200;i++) {	
					disk_window.w = k+px;
					disk_window.h = i+py;
					if (i<20) 
						draw_pixel(k+px, i+py,black); 
					else 
						draw_pixel(k+px, i+py,white);

					if (i > 198-2) {
						if (k==300) {break;}
						i=0;
						k++;
					}
				}
				mode = 1;
				open = 1;	
			}

			if ((is_in_rect(px, py, cons) && s.LeftButton) || mode == 2 ) {
				if (mode!=2) win_c++;
				k = 0;
				cha(1);
				mode = 1;
				open = 1;	
			}
			
			if (open == 1) {
				int w,h;
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
		
				root->Close(root);
		
				idx=0;
				int tmp = 0;
				int i =0;
				for (int idx=0;idx<ls();idx++) {
					w = ((disk_window.w-300)+32+idx*32)+16;
					h = disk_window.h-200+30;
					system3->cons->sp = w/8;
					tmp = icon_print(icon,1,w/8, h, black);
					for (int i=0;i<MAX_FILE_NAME_LEN-1;i++) {
						if (i>2) break;
						if (file_list[idx].name[i]==L'\0') { break; }
						else if (file_list[idx].name[i] > 60 && file_list[idx].name[i] < 96) {
							print(moji,file_list[idx].name[i]-65,c/*sp,h+40,green); }
						else if (file_list[idx].name[i] < 96) { print(moji,file_list[idx].name[i]-9,c/*sp,h+40,green); }
						else { print(moji,file_list[idx].name[i]-97,c/*sp,h+40,green);}
					}	
					struct RECT file = {w, h, 20, 30};
					sp=0;
					if (is_in_rect(px, py, file) ) {
						for (int i=0;i<11;i++) print(moji,26,c/*sp,440,black);
						system3->cons->sp = 0;	
						for (int i=0;i<11;i++) {
							if (file_list[idx].name[i]==L'\0') { break; }
							else if (file_list[idx].name[i] > 60 && file_list[idx].name[i] < 96) {
								print(moji,file_list[idx].name[i]-65,c/*sp,440,green); }
							else if (file_list[idx].name[i] < 96) { 
								print(moji,file_list[idx].name[i]-9,c/*sp,440,green); }
							else { print(moji,file_list[idx].name[i]-97,c/*sp,440,green);}
						}
					}
				}
				struct RECT close = {disk_window.w-300+14, disk_window.h-200+6, 10, 14,};
				draw_rect(close,white);
				print(moji,L'c'-97,c/*((disk_window.w-300)+16)/8,disk_window.h-200+8,green);
				if (is_in_rect(px,py,close) && s.LeftButton) {				
					k=0;
					for (int i = 0;i < 200;i++) {
						if (py+i>439) draw_pixel(px-20+k, py-10+i,black); 
						else if (py+i<439) draw_pixel(px-20+k, py-10+i,blue);
					
						if (i > 198) {
							if (k==300) {break;}
							i=0;
							k++;
						}
					}
					disk_window.w=old_win_w;
					disk_window.h=old_win_h;
					old_win_w = 0;
					old_win_h = 0;

					win_c--;
					open=0;
					mode = 0;
					continue;
				}
			}

			if (mode==0 && s.LeftButton && 
					py<disk_window.h-180 && py > disk_window.h-200 && 
					px<disk_window.w && px > disk_window.w-300 ) {
				mode = 1;
				px = disk_window.w-300;
				py = disk_window.h-200;
				old_win_w = disk_window.w;	
				old_win_h = disk_window.h;
			}

			else if (mode==1 && s.LeftButton && py<disk_window.h-180 ) {
				mode = 0;
			}
			

			//exit
			struct RECT r = {10, 450, 20, 20};

			draw_rect(r,white);
			if (is_in_rect(px, py, r) && prev_lb && !s.LeftButton) {
				break;
			}
			prev_lb = s.LeftButton;
			prev_rb = s.RightButton;
	}
}
*/
void cha(int mode, struct SYSTEM3 *system3) {
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
			{0,1,1,1,1,1,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,1,0,0,0,0,1,0},
			{1,1,1,0,0,1,1,1}, },
		{//B
			{0,1,1,1,1,1,0,0},
			{0,1,1,1,1,1,0,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,1,1,0,0},
			{0,1,1,1,1,1,0,1},
			{0,1,1,1,1,1,0,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,1,1,1,0,0},
			{0,1,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0}, },
		{//C
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,0,0,0}, 
			{1,1,0,0,0,0,0,0}, 
			{1,1,0,0,0,0,0,0}, 
			{1,1,0,0,0,0,0,0}, 
			{1,1,0,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//D
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,0,1,1}, 
			{0,1,1,0,0,0,1,1}, 
			{0,1,1,0,0,0,1,1}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//E
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//F
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//G
			{0,0,0,0,0,0,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{1,1,0,0,0,0,0,0}, 
			{1,1,0,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,0,1,0}, 
			{0,0,1,1,1,1,1,0}, 
			{0,0,1,1,1,0,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//H
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//I
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//J
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{1,0,1,1,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//K
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,1,1,0,0}, 
			{0,1,1,0,1,1,0,0}, 
			{0,1,1,1,1,0,0,0}, 
			{0,1,1,1,0,0,0,0}, 
			{0,1,1,1,0,0,0,0}, 
			{0,1,1,1,1,0,0,0}, 
			{0,1,1,0,1,1,0,0}, 
			{0,1,1,0,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//L
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//M
			{0,0,0,0,0,0,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,1,0,0,0,0,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//N
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,1,0,1,1,0}, 
			{0,1,1,1,0,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,0,1,1,1,0}, 
			{0,1,1,0,1,1,1,0}, 
			{0,1,1,0,1,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//O
			{0,0,0,0,0,0,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,1,1,1,1,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//P
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//Q
			{0,0,0,0,0,0,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,1,0,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,0,1,1,1,0}, 
			{0,0,1,1,1,1,1,0}, 
			{0,0,1,1,1,1,1,1}, 
			{0,0,0,0,0,0,0,0}, },
		{//R
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,1,1,1,0,0}, 
			{0,1,1,0,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,0,1,1}, 
			{0,0,0,0,0,0,0,0}, },
		{//S
			{0,0,0,0,0,0,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,1,0}, 
			{0,1,1,0,0,0,1,0}, 
			{1,1,1,0,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,0,1,1,1,0,0,0}, 
			{0,0,0,0,1,1,0,0}, 
			{0,0,0,0,0,1,1,1}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//T
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//U
			{0,0,0,0,0,0,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//V
			{0,0,0,0,0,0,0,0}, 
			{1,1,0,0,0,0,1,1}, 
			{1,1,0,0,0,0,1,1}, 
			{1,1,0,0,0,0,1,1}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//W
			{0,0,0,0,0,0,0,0}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,0,0,0,0,1}, 
			{1,0,0,1,1,0,0,1}, 
			{1,0,1,1,1,0,0,1}, 
			{0,1,0,1,0,0,1,0}, 
			{0,1,0,1,0,0,1,0}, 
			{0,1,0,1,0,0,1,0}, 
			{0,1,0,1,1,0,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//X
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//Y
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,1,1,0,0,1,1,0}, 
			{0,0,1,0,0,1,0,0}, 
			{0,0,1,1,1,1,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,0,0,0,0,0}, },
		{//Z
			{0,0,0,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,0,0,0,1,1,0}, 
			{0,0,0,0,1,1,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,0,1,1,0,0,0}, 
			{0,0,1,1,0,0,0,0}, 
			{0,1,1,0,0,0,0,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,1,1,1,1,1,1,0}, 
			{0,0,0,0,0,0,0,0}, },
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
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
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
			{0,0,0,0,0,0,0,0},
		},
		{//31
			{0,0,0,0,0,0,0,0},	
			{0,0,0,0,0,0,0,0},	
			{0,0,0,0,0,0,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,1,1,1,1,0,0},	
			{0,1,1,1,1,1,1,0},	
			{0,1,1,1,1,1,1,0},	
			{0,0,1,1,1,1,0,0},	
			{0,0,0,1,1,0,0,0},	
			{0,0,0,0,0,0,0,0},	
			{0,0,0,0,0,0,0,0},	
			{0,0,0,0,0,0,0,0}, 
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
			{0,0,1,1,1,0,0,0},
			{0,0,1,1,1,0,0,0},
			{0,1,1,1,1,0,0,0},
			{0,1,1,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,1,1,1,1,1,1,0},
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
			{0,0,1,1,1,0,0,0},
			{0,1,1,1,1,1,0,0},
			{0,0,0,0,1,1,1,0},
			{0,0,0,1,1,1,0,0},
			{0,0,1,1,1,0,0,0},
			{0,0,1,1,1,0,0,0},
			{0,0,0,1,1,1,0,0},
			{0,0,0,0,1,1,1,0},
			{0,1,1,1,1,1,0,0},
			{0,0,1,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,1,1,1,0,0},
			{0,0,1,1,0,1,0,0},
			{0,0,1,1,0,1,0,0},
			{0,1,1,0,0,1,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,1,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,0,1,1,0},
			{0,0,0,0,1,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,0,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
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
			{0,1,1,1,1,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,0,1,0,0,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
		{
			{0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,1,1,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,0,0,1,1,0},
			{0,1,1,1,1,1,0,0},
			{0,0,1,1,1,1,0,0},
			{0,0,0,0,1,1,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0,0},
		},
	};

	unsigned short buf[MAX_COMMAND_LEN];
	unsigned int sp = 0;
	unsigned int ent = 0;
	int n = 0;
	unsigned short *s1 = L"\0";

	system3->cons->sp = 0;
	system3->cons->ent = 0;	

	while (1) {	
		unsigned short put[] = {'p','a','s','s'};
		int ind = 0;
		for (;ind<4;) {
				print(moji,put[ind]-97,system3,white);
				ind++;
		}
			system3 = print(moji,28,system3,white);
			while (1) { 
				buf[n] = getc();
				if (buf[n] == L'\r') {
					system3->cons->ent += 12;
					system3->cons->sp = 0;	
					buf[n] = L'\0';
					break;
				}
				if (buf[n] == 8) {
					if (sp > 2) {
						system3->cons->sp--;
						system3 = print(moji,26,system3,black);
						system3->cons->sp--;
						n--;
						buf[n]=0;
					}
					else {}
				}
				else {	
					system3 = print(moji,27,system3,white);
					n++;
				}
			}
		cls();
		system3->cons->sp=0;
		system3->cons->ent=0;
		n=0;
		if (!strcmp(L"dango1027",buf)) {break;}
	}
	
	unsigned short cha[] = {'s','y','s','t','e','m','3','\n','m','a','d','e',' ','b','y',' ','d','a','n','g','o','m','u','s','h','i','\n','\0'};

	for (int i=0;cha[i]!=L'\0';i++) {
		system3 = putchar(moji, cha[i], system3, green);
	}
	
	while (1) {
		unsigned int n;

		unsigned short put[] = {'r','o','o','t'};
		int ind = 0;
		for (;ind<4;) {
			system3 = print(moji,put[ind]-97, system3,green);
			ind++;
		}

		system3 = print(moji,28, system3,green);
		system3 = print(moji, 26, system3, black);

		for (n = 0; n < MAX_COMMAND_LEN - 1;) {
			buf[n] = getc();
			if (buf[n] == L'\r') {
				system3->cons->ent += 12;
				system3->cons->sp = 0;
				break;
			}
			if (buf[n] == 8) {
				if (system3->cons->sp > 2) {
					system3->cons->sp--;
					system3 = print(moji, 26, system3, black);
					system3->cons->sp--;
					n--;
					buf[n]=0;
				}
				else {}
			}
			else {	
				if (buf[n]==L'\0') { break; }
				else system3=putchar(moji,buf[n],system3,green);
				n++;
			}
		}
		buf[n] = L'\0';
		
		if (!strcmp(L"edit ", command(s1,buf,5))) {
			cls();
			edit_mode(buf+5,moji);
			cls();
			system3->cons->sp=0;
			system3->cons->ent=0;
		}
		
		else if (!strcmp(L"exit",buf))
			break;
		else if (!strcmp(L"key",buf)) {
			while (1) {
				unsigned short num = getc();
				system3 = print(moji,num-6, system3,green);
			}	
		}
		else if (!strcmp(L"gui", buf)) {
			gui_mode(moji, system3);
			break;
		}
		else if (!strcmp(L"cls", buf)) {
			system3->cons->ent = 0;
			system3->cons->sp = 0;		
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
						system3=putchar(moji, file_list[idx].name[i],system3,green);
						if (file_list[idx].name[i]==L'\0') { break; }
					}	
					idx++;
					system3->cons->sp = 0;
					system3->cons->ent+=12;
				}
				system3->cons->sp=0;
				system3->cons->ent+=12;
				file_num = idx;
				root->Close(root);
			}
		else if (!strcmp(L"proto",buf))
			proto_inter();
		else if(!strcmp(L"qube",buf)) 
			qube();
		else if (!strcmp(L"echo ", command(s1,buf,5))) {
			unsigned short echo[100];
			for (int i=0;buf[i+5]!=L'\0';i++) {
				system3= putchar(moji, buf[i+5], system3, green);
			}	
			system3->cons->ent += 12;
			system3->cons->sp = 0;
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
				system3 = putchar(moji,file_buf[n], system3, green);
			}

			file->Close(file);
			root->Close(root);
			system3->cons->sp=0;
			system3->cons->ent+=12;
		}

		else if (!strcmp(L"proto ",command(s1,buf,6))) {
			system3->cons->sp=0;
			system3->cons->ent=0;
			cls();
			proto(buf+6);
		}
		else if (!strcmp(L"le ",command(s1,buf,3))) {
			system3 = le(buf+3,moji, system3);
		}
		//else if(!strcmp(L"vse ",command(s1,buf,4))) 
		//	bse(buf+4);
		else {

			unsigned short err[] = {'e','r','r',' ','i','n','v','i','d',' ', 'c','o','m','m','a','n','d', '\0'};
			
			for (int i=0;err[i]!=L'\0';i++) { 
				system3 = print(moji,err[i]-97,system3,green); 
			}
			system3 = print(moji,26, system3,black);
			system3->cons->ent += 12;
			system3->cons->sp = 0;
		}	
	}
}

void startup(struct SYSTEM3 *system3) {
	/*
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	status = root->Open(root, &file, "set.conf", EFI_FILE_MODE_READ, 0);
	assert(status, L"root->Open");

	status = file->Read(file, &buf_size, (void *)file_buf);
	assert(status, L"file->Read");

	while (1) {

		system3->back_color = black;
		system3->char_color = green;
		
		if (idx == 0) {
			if (strcmp(file_list[idx].name, L"r")) {
				system3->char_color = red;
			}
		}

		idx++;
	}
	file_num = idx;
	for (int n=0;file_buf[n]!=L'\0';n++) {
		system3 = putchar(moji,file_buf[n], system3, green);
	}

	file->Close(file);
	root->Close(root);
	system3->cons->sp=0;
	system3->cons->ent+=12;
*/
}

void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};
	unsigned char is_exit = FALSE;
	unsigned short *s1 = L"\0";
	struct SYSTEM3 sys3, *system3;
	system3 = &sys3;

	//rogin();
	int mode = 0;
	cha(mode, system3);
}
