#ifndef _SHELL_H_
#define _SHELL_H_

struct CONSOLE {
    struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL back_color;
    struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL char_color;
	unsigned int sp;
	unsigned int ent;
    unsigned short com_his[50][256];
    int comHis_c;
    unsigned short *chr;
};

void dialogue_get_filename(int idx);
void pstat(void);
int ls(void);
void cat();
void edit(unsigned short *file_name);
void shell(void);

#endif
