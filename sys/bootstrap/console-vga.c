#include <mrg.h>
#include <string.h>
#include "vgahw.h"
#include "internal.h"

#define VGA_COLORATTR 0x18

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
	uint8_t dummy;

	memset(__vga_mem, 0, 80 * 25 * 2);

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

uint8_t xpos = 0;
uint8_t ypos = 0;

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

static void update_cursor(void)
{
	uint16_t pos = ypos * 80 + xpos;

	VGAOUTB(VGA_CRT_CURSOR_HIGH, VGA_CRT_ADDR_REG);
	VGAOUTB(pos >> 8, VGA_CRT_DATA_REG);
	VGAOUTB(VGA_CRT_CURSOR_LOW, VGA_CRT_ADDR_REG);
	VGAOUTB(pos & 0xff, VGA_CRT_DATA_REG);
}

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
