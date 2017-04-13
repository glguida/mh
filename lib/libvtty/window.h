/*
 * window.h	Constants, prototypes etc. for the window routines.
 *
 *		$Id: window.h,v 1.16 2007-10-10 20:18:20 al-guest Exp $
 *
 *		This file is part of the minicom communications package,
 *		Copyright 1991-1996 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * fmg 1/11/94 colors
 * fmg 8/20/97 added stuff for history search section
 * fmg 8/21/97 added kludged F_key support for RedHat4.1
 */

#include <stddef.h>

/*
 * One character is contained in a "ELM"
 */
typedef struct _elm {
  char value;
  char attr;
  char color;
} ELM;

/*
 * Control struct of a window
 */
typedef struct _win {
  int x1, y1, x2, y2;	/* Coordinates */
  int sy1, sy2;		/* Scrolling region */
  int xs, ys;		/* x and y size */
  char border;		/* type of border */
  char cursor;		/* Does it have a cursor */
  char attr;		/* Current attribute of window */
  char color;		/* Current color of window */
  char autocr;		/* With '\n', do an automatic '\r' */
  char doscroll;	/* Automatically scroll up */
  char wrap;		/* Wrap around edge */
  char direct;		/* Direct write to screen ? */
  short curx, cury;	/* current x and y */
  char o_cursor;
  short o_curx;
  short o_cury;
  char o_attr;
  char o_color;		/* Position & attributes before window was opened */
  ELM *map;		/* Map of contents */
  ELM *histbuf;		/* History buffer. */
  int histlines;	/* How many lines we keep in the history buffer */
  int histline;		/* Current line in the history buffer. */
} WIN;

/*
 * Stdwin is the whole screen
 */
extern WIN *stdwin;	/* Whole screen */
extern int LINES, COLS; /* Size of sreen */
extern int usecolor;	/* Use ansi color escape sequences */
extern int useattr;	/* Use attributes (reverse, bold etc. ) */
extern int dirflush;	/* Direct flush after write */
extern int screen_ibmpc;  /* Literal pass-through of all characters? */
extern int screen_iso;	/* Literal pass-through of all characters? */
extern int w_init;	/* Already initialized? */
extern char *_tptr;
extern int use_status;	/* Turned on in main() */

/* fmg 1/11/94 colors */

extern int mfcolor;     /* Menu Foreground Color */
extern int mbcolor;     /* Menu Background Color */
extern int tfcolor;     /* Terminal Foreground Color */
extern int tbcolor;     /* Terminal Background Color */
extern int sfcolor;     /* Status Bar Foreground Color */
extern int sbcolor;     /* Status Bar Background Color */
extern const char *J_col[]; /* Color's names */

/*
 * Possible attributes.
 */
#define XA_NORMAL	 0
#define XA_BLINK	 1
#define XA_BOLD		 2
#define XA_REVERSE	 4
#define XA_UNDERLINE	 8
#define XA_ALTCHARSET	16
#define XA_BLANK	32

/*
 * Possible colors
 */
#define BLACK		0
#define RED		1
#define GREEN		2
#define YELLOW		3
#define BLUE		4
#define MAGENTA		5
#define CYAN		6
#define WHITE		7

#define COLATTR(fg, bg) (((fg) << 4) + (bg))
#define COLFG(ca)	((ca) >> 4)
#define COLBG(ca)	((ca) % 16)

/*
 * Possible borders.
 */
#define BNONE	0
#define BSINGLE 1
#define BDOUBLE 2

/*
 * Scrolling directions.
 */
#define S_UP	1
#define S_DOWN	2

/*
 * Cursor types.
 */
#define CNONE	0
#define CNORMAL	1

/*
 * Title Positions
 */
#define TLEFT	0
#define TMID	1
#define TRIGHT	2

/*
 * Function prototypes.
 */

int wxgetch(void);

void vtty_wflush(void);
WIN *vtty_wopen(int x1, int y1, int x2, int y2, int border,
           int attr, int fg, int bg, int direct, int hl, int rel);
void vtty_wclose(WIN *win, int replace);
void vtty_wleave(void);
int vtty_wreturn(void);
void vtty_wresize(WIN *w, int x, int y);
void vtty_wredraw(WIN *w, int newdirect);
void vtty_wscroll(WIN *win, int dir);
void vtty_wlocate(WIN *win, int x, int y);
void vtty_wputc(WIN *win, char c);
void vtty_wdrawelm(WIN *win, int y, ELM *e);
void vtty_wputs(WIN *win, const char *s);
int vtty_wprintf(WIN *, const char *, ...)
        __attribute__((format(printf, 2, 3)));
void vtty_wbell(void);
void vtty_wcursor(WIN *win, int type);
void vtty_wtitle(WIN *w, int pos , const char *s);
void vtty_wcurbar(WIN *w, int y , int attr);
int vtty_wselect(int x, int y, const char *const *choices,
            void (*const *funlist)(void) ,
            const char *title , int attr , int fg , int bg );
void vtty_wclrch(WIN *w, int n);
void vtty_wclrel(WIN *w);
void vtty_wclreol(WIN *w);
void vtty_wclrbol(WIN *w);
void vtty_wclreos(WIN *w);
void vtty_wclrbos(WIN *w);
void vtty_winclr(WIN *w);
void vtty_winsline(WIN *w);
void vtty_wdelline(WIN *w);
void vtty_winschar(WIN *w);
void vtty_winschar2(WIN *w, char c, int move);
void vtty_wdelchar(WIN *w);
int  vtty_wgets(WIN *win, char *s, int linemax, int totmax);
int  vtty_wgetwcs(WIN *win, char *s, int linemax, int totmax);
void win_end(void);
#ifdef BBS
int win_init(char *term, int lines);
#else
int win_init(int fg, int bg, int attr);
#endif
/* fmg 8/20/97: both needed by history search section */
void vtty_wdrawelm_inverse( WIN *w, int y, ELM *e);
void vtty_wdrawelm_var(WIN *w, ELM *e, char *buf);

/*
 * Some macro's that can be used as functions.
 */
#define vtty_wsetregion(w, z1, z2) (((w)->sy1=(w)->y1+(z1)),((w)->sy2=(w)->y1+(z2)))
#define vtty_wresetregion(w) ( (w)->sy1 = (w)->y1, (w)->sy2 = (w)->y2 )
#define vtty_wgetattr(w) ( (w)->attr )
#define vtty_wsetattr(w, a) ( (w)->attr = (a) )
#define vtty_wsetfgcol(w, fg) ( (w)->color = ((w)->color & 15) + ((fg) << 4))
#define vtty_wsetbgcol(w, bg) ( (w)->color = ((w)->color & 240) + (bg) )
#define vtty_wsetam(w) ( (w)->wrap = 1 )
#define vtty_wresetam(w) ( (w)->wrap = 0 )

/*
 * Allright, now the macro's for our keyboard routines.
 */

#define K_BS		8
#define K_ESC		27
#define K_STOP		256
#define K_F1		257
#define K_F2		258
#define K_F3		259
#define K_F4		260
#define K_F5		261
#define K_F6		262
#define K_F7		263
#define K_F8		264
#define K_F9		265
#define K_F10		266
#define K_F11		277
#define K_F12		278

#define K_HOME		267
#define K_PGUP		268
#define K_UP		269
#define K_LT		270
#define K_RT		271
#define K_DN		272
#define K_END		273
#define K_PGDN		274
#define K_INS		275
#define K_DEL		276

#define NUM_KEYS	23
#define KEY_OFFS	256

/* Here's where the meta keycode start. (512 + keycode). */
#define K_META		512

#ifndef EOF
#  define EOF		((int) -1)
#endif
#define K_ERA		'\b'
#define K_KILL		((int) -2)

/* Internal structure. */
struct key {
  char *cap;
  char len;
};

