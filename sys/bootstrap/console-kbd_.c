#include <stdio.h>
#include <mrg.h>
#include "internal.h"

/*
 * drex--muramasa!
 */

struct 
{ 
  char normal;
  char shift;
} keymap_us[128] = {

  [01] = {'\27', 0 },
  [02] = {'1', '!' },
  [3] = {'2', '@' },
  [4] = {'3', '#' },
  [5] = {'4', '$' },
  [6] = {'5', '%' },
  [7] = {'6', '^' },
  [8] = {'7', '&' },
  [9] = {'8', '*' },
  [10]= {'9', '(' },
  [11]= {'0', ')' },
  [12]= {'-', '_' },
  [13]= {'=', '+' },
  [14]= {'\27', 0}, 
  [15]= {'\t', 0},

  [16] = {'q', 'Q'},
  [17] = {'w', 'W'},
  [18] = {'e', 'E'},
  [19] = {'r', 'R'},
  [20] = {'t', 'T'},
  [21] = {'y', 'Y'},
  [22] = {'u', 'U'},
  [23] = {'i', 'I'},
  [24] = {'o', 'O'},
  [25] = {'p', 'P'},

  [26] = {'[', '{'},
  [27] = {']', '}'},
  [28] = {'\n', 0},
  /* 29 is control */

  [30] = {'a', 'A'},
  [31] = {'s', 'S'},
  [32] = {'d', 'D'},
  [33] = {'f', 'F'},
  [34] = {'g', 'G'},
  [35] = {'h', 'H'},
  [36] = {'j', 'J'},
  [37] = {'k', 'K'},
  [38] = {'l', 'L'},

  [39] = {';', ':'},
  [40] = {'\'', '"'},
  [41] = {'`', '~'},
  /* 41 is alt */
  /* 42 is shift */
  [43] = {'\\', '|'},

  [44] = {'z', 'Z'},
  [45] = {'x', 'X'},
  [46] = {'c', 'C'},
  [47] = {'v', 'V'},
  [48] = {'b', 'Z'},
  [49] = {'n', 'N'},
  [50] = {'m', 'M'},

  [51] = {',', '<'},
  [52] = {'.', '>'},
  [53] = {'/', '?'},
  /* 54 is shift */
  /* 56 is alt */
  [57] = {' ', 0},
  /* 58 is capslck */
  /* 97 is control */
  [111] = {'\127', 0},
  {0, 0},
};

#define STATUS_SHIFT 1

static int
keymapper_us(char sc, char *c)
{
  static unsigned char state;
  int released = 0;

  if (sc < 0) 
    released = 1;

  sc &= 0x3f;

  if ((sc == 42)||(sc == 56)) {
    if (released)
      state &= ~STATUS_SHIFT;
    else
      state |= STATUS_SHIFT;
  }

  if (released)
    return 0;
  
  *c = (state & STATUS_SHIFT) ? keymap_us[(int)sc].shift : 
    keymap_us[(int)sc].normal;

  if (*c == 0)
    return 0;

  return 1;
}

static DEVICE *kbdd = NULL;

static void __kbd_ast(void)
{
	uint64_t val;
	char c;

	din(kbdd, IOPORT_BYTE(0x64), &val);
	while (val & 0x1) {
		din(kbdd, IOPORT_BYTE(0x60), &val);
		if (keymapper_us((char)val, &c)) {
			kbdbuf_add(0, c);
		}
		din(kbdd, IOPORT_BYTE(0x64), &val);
	}
}

int
console_kbd_init(void)
{
	int kbdevt;

	kbdd = dopen("KBD_");
	if (kbdd == NULL)
		return -ENOENT;

	kbdevt = evtalloc();
	evtast(kbdevt, __kbd_ast);

	dout(kbdd, IOPORT_BYTE(0x64), 0xad);
	dout(kbdd, IOPORT_BYTE(0x64), 0xae);

	/* XXX: CHECK FROM DEVICE */
	printf("%d = dirq\n", dirq(kbdd, 1, kbdevt));

	return 0;
}
