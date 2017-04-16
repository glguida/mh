#ifndef __VTDRV_H__
#define __VTDRV_H__



int vtdrv_init(void);
void vtdrv_exit(void);
short vtdrv_lines(void);
short vtdrv_columns(void);

void vtdrv_set_attr(char attr);
void vtdrv_set_color(int colattr);

void vtdrv_cursor(int on);
void vtdrv_goto(short x, short y);
void vtdrv_scroll(void);
void vtdrv_upscroll(void);
void vtdrv_putc(char c);
void vtdrv_bell(void);

#endif /* !__VTDRV_H__ */
