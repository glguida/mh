#include <stdio.h>
#include <mrg.h>
#include <mrg/consio.h>
#include "internal.h"

int console_kbd_raw = 0;

#define MAX_SCAN_CODE 0x55

static int fix_keymap[MAX_SCAN_CODE] = {
	[0x01] = K_ESC,
	[0x0e] = K_BS,
	[0x0f] = K_HT,
	[0x1c] = K_ENT,
	[0x3a] = K_CAP,
	[0x3b] = K_F1,
	[0x3c] = K_F2,
	[0x3d] = K_F3,
	[0x3e] = K_F4,
	[0x3f] = K_F5,
	[0x40] = K_F6,
	[0x41] = K_F7,
	[0x42] = K_F8,
	[0x43] = K_F9,
	[0x44] = K_F10,
	[0x45] = K_NUM,
	[0x46] = K_SCL,
	[0x54] = K_SYS,
};

static int mod_keymap[MAX_SCAN_CODE] = {
	[0x1d] = K_CTL,
	[0x2a] = K_SFT,
	[0x36] = K_SFT,
	[0x38] = K_ALT,
};

static int ext_keymap[MAX_SCAN_CODE] = {
	[0x1c] = K_ENT,
	[0x1d] = K_CTL,
	/* Ingore fake shift. [0x2a] = K_SFT, */
	[0x35] = '/',
	/* Ignore fake shift. [0x36] = K_SFT, */
	[0x37] = K_PSCR,
	[0x38] = K_ALT,
	[0x46] = K_BRK,
	[0x47] = K_HOME,
	[0x48] = K_UP,
	[0x49] = K_PGUP,
	[0x4b] = K_LT,
	[0x4d] = K_RT,
	[0x4f] = K_END,
	[0x50] = K_DN,
	[0x51] = K_PGDN,
	[0x52] = K_INS,
	[0x53] = K_DEL,
};

static int pad_keymap_n[MAX_SCAN_CODE] = {
	[0x37] = '*',
	[0x47] = '7',
	[0x48] = '8',
	[0x49] = '9',
	[0x4a] = '-',
	[0x4b] = '4',
	[0x4c] = '5',
	[0x4d] = '6',
	[0x4e] = '+',
	[0x4f] = '1',
	[0x50] = '2',
	[0x51] = '3',
	[0x52] = '0',
	[0x53] = '.',
};

static int pad_keymap[MAX_SCAN_CODE] = {
	[0x37] = '*',
	[0x47] = K_HOME,
	[0x48] = K_UP,
	[0x49] = K_PGUP,
	[0x4a] = '-',
	[0x4b] = K_LT,
	[0x4c] = 0,
	[0x4d] = K_RT,
	[0x4e] = '+',
	[0x4f] = K_END,
	[0x50] = K_DN,
	[0x51] = K_PGDN,
	[0x52] = 0,
	[0x53] = '.',
};

static int us_keymap[MAX_SCAN_CODE] = { 0,
  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
  0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,
   0 , 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
  '`',  0 ,'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0 ,
  0,  0 , ' ',
};

static int us_keymap_s[MAX_SCAN_CODE] = { 0,
  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
  0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,
   0 , 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',
  '~',  0 ,'|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
  0,  0 , ' ',
};

#define SC_REL 0x80

#define ST_SFT 1
#define ST_CTL 2
#define ST_ALT 4
#define ST_EX0 8
#define ST_CAP 32
#define ST_NUM 64
int state = 0;


int getkey(char sc)
{
	int key;
	int rel;

	rel = sc & SC_REL;
	sc &= ~SC_REL;

	if (sc == 0xe0) {
		state |= ST_EX0;
		return 0;
	}
	
	if (sc >= MAX_SCAN_CODE) {
		/* Ignore unknown scan codes */
		return 0;
	}

	if (state & ST_EX0) {
		key = ext_keymap[sc];
		state &= ~ST_EX0;
	} else {
		key = state & ST_SFT ? us_keymap_s[sc] : us_keymap[sc];
		if (!key)
			key = fix_keymap[sc];
		if (!key)
			key = mod_keymap[sc];
		if (!key)
			key = state & ST_NUM ? pad_keymap_n[sc] : pad_keymap[sc];
	}

	/* Process mod keys */
	if (key == K_CTL) {
		if (rel)
			state &= ~ST_CTL;
		else
			state |= ST_CTL;
		if (!console_kbd_raw)
			key = 0;
	}
	if (key == K_SFT) {
		if (rel)
			state &= ~ST_SFT;
		else
			state |= ST_SFT;
		if (!console_kbd_raw)
			key = 0;
	}
	if (key == K_ALT) {
		if (rel)
			state &= ~ST_ALT;
		else
			state |= ST_ALT;
		if (!console_kbd_raw)
			key = 0;
	}

	if (!console_kbd_raw && rel)
		return 0;

	if (key == 0)
		return 0;

	if (!console_kbd_raw) {
		key |= state & ST_ALT ? M_ALT : 0;
		key |= state & ST_SFT ? M_SFT : 0;
		key |= state & ST_CTL ? M_CTL : 0;

		/* Ignore mods for K_ENT and K_BS */
		if (key == K_ENT || key == K_BS)
			key &= ~(M_ALT|M_SFT|M_CTL);
	}
//	printf("key = %02x\n", key);
	return key;
}

static DEVICE *kbdd = NULL;
static int kbdevt;

static void __kbd_ast(void)
{
	uint64_t val;
	int c;

	din(kbdd, IOPORT_BYTE(0x64), &val);
	while (val & 0x1) {
		din(kbdd, IOPORT_BYTE(0x60), &val);
		if (c = getkey(val)) {
			if (c >= K_F1 && c <= K_F10) {
				console_switch(c - K_F1);
			} else {
				kbdbuf_add(c);
			}
		}
		din(kbdd, IOPORT_BYTE(0x64), &val);
	}
	evtclear(kbdevt);
}

void *console_kbd_save(void)
{
	return NULL;
}

void console_kbd_restore(void *opq)
{
}

void console_kbd_close(void *opq)
{
}

int
console_kbd_init(uint64_t nameid)
{
	int ret, irq;
	uint64_t pnpid = squoze("PNP0303");
	struct device *tmp, *d = NULL;
	struct dinfo info;

	char name[13];
	unsquozelen(nameid, 13, name);
	kbdd = dopen(name);
	if (kbdd == NULL) {
		printf("NO KBD_ FOUND");
		return -ENOENT;
	}

	ret = dgetinfo(kbdd, &info);
	if (ret)
		return ret;

	/* Get the first IRQ. We require one. */
	if (info.nirqs == 0)
		return -EINVAL;
	irq = dgetirq(kbdd, 0);
	assert(irq >= 0);

	kbdevt = evtalloc();
	evtast(kbdevt, __kbd_ast);

	ret = dmapirq(kbdd, irq, kbdevt);
	if (ret)
		return ret;

	dout(kbdd, IOPORT_BYTE(0x64), 0xad);
	dout(kbdd, IOPORT_BYTE(0x64), 0xae);

	return 0;
}
