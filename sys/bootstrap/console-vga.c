#include <mrg.h>
#include <stdlib.h>
#include <string.h>
#include "vgahw.h"
#include "internal.h"
#include <vtty/window.h>
#include <vtty/vtdrv.h>

#define VGA_COLORATTR 0x07

static DEVICE *vgad = NULL;

#define VGAINB(_port, _ptr) do { uint64_t tmp; din(vgad, IOPORT_BYTE(_port), &tmp); *(_ptr) = (uint8_t)tmp; } while (0)
#define VGAOUTB(_val, _port) dout(vgad, IOPORT_BYTE(_port), (_val))

struct vga_state {
	uint8_t vmem[80 * 25 * 2];

	uint8_t seq_clk;
	uint8_t seq_map;
	uint8_t seq_fnt;
	uint8_t seq_mod;

	uint8_t gfx_map;
	uint8_t gfx_mod;
	uint8_t gfx_mis;

	uint8_t crt_max_scan;
	uint8_t crt_crs_start;
	uint8_t crt_crs_end;
	uint8_t crt_crs_hi;
	uint8_t crt_crs_lo;

	uint8_t attr_mod;
};

volatile uint8_t *__vga_mem = NULL;
static struct vga_state __vga_state;
uint8_t xpos = 0;
uint8_t ypos = 0;

struct console_vga_state {
	uint8_t attr_mod;
  	uint8_t crt_crs_start;
	uint8_t xpos;
	uint8_t ypos;
};

static void vga_save(void)
{
	struct vga_state *s = &__vga_state;
	void *vgamem;

	/* Sequencer */
	VGAOUTB(VGA_SEQ_CLOCK_MODE, VGA_SEQ_ADDR_REG);
	VGAINB(VGA_SEQ_DATA_REG, &s->seq_clk);
	VGAOUTB(VGA_SEQ_MAP, VGA_SEQ_ADDR_REG);
	VGAINB(VGA_SEQ_DATA_REG, &s->seq_map);
	VGAOUTB(VGA_SEQ_FONT, VGA_SEQ_ADDR_REG);
	VGAINB(VGA_SEQ_DATA_REG, &s->seq_fnt);
	VGAOUTB(VGA_SEQ_MODE, VGA_SEQ_ADDR_REG);
	VGAINB(VGA_SEQ_DATA_REG, &s->seq_mod);

	/* GFX */
	VGAOUTB(VGA_GFX_MAP, VGA_GFX_ADDR_REG);
	VGAINB(VGA_GFX_DATA_REG, &s->gfx_map);
	VGAOUTB(VGA_GFX_MODE, VGA_GFX_ADDR_REG);
	VGAINB(VGA_GFX_DATA_REG, &s->gfx_mod);
	VGAOUTB(VGA_GFX_MISC, VGA_GFX_ADDR_REG);
	VGAINB(VGA_GFX_DATA_REG, &s->gfx_mis);

	/* CRTC */
	VGAOUTB(VGA_CRT_MAX_SCAN_LINE, VGA_CRT_ADDR_REG);
	VGAINB(VGA_CRT_DATA_REG, &s->crt_max_scan);
	VGAOUTB(VGA_CRT_CURSOR_START, VGA_CRT_ADDR_REG);
	VGAINB(VGA_CRT_DATA_REG, &s->crt_crs_start);
	VGAOUTB(VGA_CRT_CURSOR_END, VGA_CRT_ADDR_REG);
	VGAINB(VGA_CRT_DATA_REG, &s->crt_crs_end);
	VGAOUTB(VGA_CRT_CURSOR_HIGH, VGA_CRT_ADDR_REG);
	VGAINB(VGA_CRT_DATA_REG, &s->crt_crs_hi);
	VGAOUTB(VGA_CRT_CURSOR_LOW, VGA_CRT_ADDR_REG);
	VGAINB(VGA_CRT_DATA_REG, &s->crt_crs_lo);

	/* Attr */
	VGAINB(VGA_INPUT_STATUS_1_REG, &s->attr_mod);
	VGAOUTB(VGA_ATTR_MODE, VGA_ATTR_ADDR_DATA_REG);
	VGAINB(VGA_ATTR_DATA_READ_REG, &s->attr_mod);

	memcpy(s->vmem, __vga_mem, 80 * 25 * 2);
}

static void vga_setup(void)
{
	int i;
	uint64_t dummy;

	memset(__vga_mem, 0, 80 * 25 * 2);

	/* Enable blinking */
	VGAINB(VGA_INPUT_STATUS_1_REG, &dummy);
	VGAOUTB(VGA_ATTR_MODE, VGA_ATTR_ADDR_DATA_REG);
	VGAOUTB(__vga_state.attr_mod | (1 << 3), VGA_ATTR_ADDR_DATA_REG);
	
	/* Enable screen */
	VGAINB(VGA_INPUT_STATUS_1_REG, &dummy);
	VGAOUTB(VGA_ATTR_ENABLE, VGA_ATTR_ADDR_DATA_REG);
	VGAOUTB(0x00, VGA_ATTR_ADDR_DATA_REG);

	VGAOUTB(VGA_GFX_MISC, VGA_GFX_ADDR_REG);
	VGAOUTB(VGA_GFX_MISC_CHAINOE | VGA_GFX_MISC_B8TOBF,
		VGA_GFX_DATA_REG);
	VGAOUTB(VGA_GFX_MODE, VGA_GFX_ADDR_REG);
	VGAOUTB(VGA_GFX_MODE_HOSTOE, VGA_GFX_DATA_REG);

	for (i = 0; i < 80 * 25; i++)
		__vga_mem[i * 2 + 1] = VGA_COLORATTR;
}

static void scroll(void)
{
	int i;
	memmove((void *) __vga_mem,
		(void *) (__vga_mem + 80 * 2), 80 * 24 * 2);

	for (i = 0; i < 80; i++) {
		(__vga_mem + 80 * 24 * 2)[i * 2] = 0;
		(__vga_mem + 80 * 24 * 2)[i * 2 + 1] = VGA_COLORATTR;
	}
}

static void upscroll(void)
{
	int i;
	memmove((void *) (__vga_mem + 80 * 2),
		(void *) __vga_mem, 80 * 24 * 2);
}

static void update_cursor(void)
{
	uint16_t pos = ypos * 80 + xpos;

	VGAOUTB(VGA_CRT_CURSOR_HIGH, VGA_CRT_ADDR_REG);
	VGAOUTB(pos >> 8, VGA_CRT_DATA_REG);
	VGAOUTB(VGA_CRT_CURSOR_LOW, VGA_CRT_ADDR_REG);
	VGAOUTB(pos & 0xff, VGA_CRT_DATA_REG);
}

int console_vga_init(uint64_t nameid)
{
	char name[13];

	unsquozelen(nameid, 13, name);

	vgad = dopen(name);
	if (vgad == NULL)
		return -ENOENT;

	__vga_mem = diomap(vgad, VGA_VIDEO_MEM_BASE, VGA_VIDEO_MEM_LENGTH);
	vga_save();
	vga_setup();
	return 0;
}

void *console_vga_save(void)
{
	struct console_vga_state *sv;

	sv = malloc(sizeof(*sv));
	sv->xpos = xpos;
	sv->ypos = ypos;

	VGAOUTB(VGA_CRT_CURSOR_START, VGA_CRT_ADDR_REG);
	VGAINB(VGA_CRT_DATA_REG, &sv->crt_crs_start);
	return (void *)sv;
}

void console_vga_restore(void *opq)
{
	struct console_vga_state *sv = (struct console_vga_state *)opq;

	if (sv == NULL)
		return;

	xpos = sv->xpos;
	ypos = sv->ypos;
	VGAOUTB(VGA_CRT_CURSOR_START, VGA_CRT_ADDR_REG);
	VGAOUTB(sv->crt_crs_start, VGA_CRT_DATA_REG);
	free(opq);
}

void console_vga_close(void *opq)
{
	if (opq != NULL)
		free(opq);
}

void console_vga_cursor(int on)
{
	if (on) {
		VGAOUTB(VGA_CRT_CURSOR_START, VGA_CRT_ADDR_REG);
		VGAOUTB(0, VGA_CRT_DATA_REG);
	} else {
		VGAOUTB(VGA_CRT_CURSOR_START, VGA_CRT_ADDR_REG);
		VGAOUTB((1 << 5), VGA_CRT_DATA_REG);
	}
}

void console_vga_scroll(void)
{
	upscroll();
}

void console_vga_upscroll(void)
{
	scroll();
}

void console_vga_bell(void)
{
}

void console_vga_goto(unsigned x, unsigned y)
{
	xpos = x;
	ypos = y;
	update_cursor();
}

static int coltrans(int col)
{
	switch (col) {
	case BLACK:
		return 0;
	case BLUE:
		return 1;
	case GREEN:
		return 2;
	case CYAN:
		return 3;
	case RED:
		return 4;
	case MAGENTA:
		return 5;
	case  YELLOW:
		return 6;
	case WHITE:
		return 7;
	default:
		return 7;
	}
}

void console_vga_putc(int c, int colattr, int xattr)
{
	uint8_t attr;
	int bg, fg;

	if (xattr & XA_BLANK) {
		attr = 0;
	} else {
		attr = 0;
		if (xattr & XA_BLINK) {
			attr |= (1 << 7);
		}
		if (xattr & XA_BOLD) {
			attr |= (1 << 3);
		}
	}

	if (xattr & XA_REVERSE) {
		fg = COLBG(colattr);
		bg = COLFG(colattr);
	} else {
		fg = COLFG(colattr);
		bg = COLBG(colattr);
	}

	bg = coltrans(bg);
	fg = coltrans(fg);

	attr |= (bg & 0x7);
	attr |= ((fg & 0x7) << 4);

	if (xattr & XA_ALTCHARSET) {
		switch (c) {
		case 'L':
			c = 201;
			break;
		case 'l':
			c = 218;
			break;
		case 'Q':
			c = 205;
			break;
		case 'q':
			c = 196;
			break;
		case 'K':
			c = 187;
			break;
		case 'k':
			c = 191;
			break;
		case 'M':
			c = 200;
			break;
		case 'm':
			c = 192;
			break;
		case 'X':
			c = 186;
			break;
		case 'x':
			c = 179;
			break;
		case 'J':
			c = 188;
			break;
		case 'j':
			c = 217;
			break;
		default:
			c = 15;
			break;
		}
	}

	__vga_mem[(xpos + ypos * 80) * 2] = c & 0xff;
	__vga_mem[(xpos + ypos * 80) * 2 + 1] = attr;
	xpos++;
	if (xpos >= 80) {
		xpos = 0;
		ypos >= (25-1) ? 24 : ypos++;
	}
	update_cursor();
}

unsigned console_vga_cols(void)
{
	return 80;
}

unsigned console_vga_lines(void)
{
	return 25;
}

/*
 * 'Autonomous' putchar: print and scroll.
 */

static void newline()
{
	xpos = 0;
	ypos++;
	if (ypos >= 25) {
		ypos = 24;
		scroll();
	}
	return;
}

void console_vga_putchar(int c)
{
	if (c == '\n' || c == '\r') {
		newline();
		update_cursor();
		return;
	}

	if (c == '\t') {
		/* 8 pos tab */
		xpos = (xpos & 0xfffffff8) + 8;
		if (xpos >= 80)
			newline();
		update_cursor();
		return;
	}

	__vga_mem[(xpos + ypos * 80) * 2] = c & 0xff;
	__vga_mem[(xpos + ypos * 80) * 2 + 1] = VGA_COLORATTR;

	xpos++;
	if (xpos >= 80)
		newline();
	update_cursor();
}
