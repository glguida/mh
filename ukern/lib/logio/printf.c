/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <uk/types.h>
#include <uk/stdarg.h>
#include <uk/logio.h>

extern void _boot_putc(int);
extern void _boot_sysputc(int);
extern void klog_putc(int);
void (*kernputcfn) (int) = _boot_putc;
void (*sysputcfn) (int) = _boot_sysputc;

void putc(int ch)
{
	klog_putc(ch);
	if (kernputcfn)
		kernputcfn(ch);
}

void sysputc(int ch)
{
	klog_putc(ch);
	if (sysputcfn)
		sysputcfn(ch);
}

void _setputcfn(void (*fn) (int), void (*sysfn) (int))
{
	kernputcfn = fn;
	sysputcfn = sysfn;
}

int __printflike(1, 2) printf(const char *fmt, ...)
{
	int r;
	va_list ap;

	va_start(ap, fmt);
	r = vprintf(fmt, ap);
	va_end(ap);
	return r;
}

#define O_ZERO  (1 << 0)
#define O_SPACE (1 << 1)
#define O_SIGN  (1 << 2)
#define O_LEFT  (1 << 3)
#define O_ALT   (1 << 4)
#define _BUFSZ 65

static char *__toa(char *buf, uint64_t val, char pmode, int opt)
{
	char *ptr = buf + _BUFSZ - 1;
	int base, rem;
	int sgnd = 0;
	char loc = '0';
	char hic = 'a' - 10;

	*ptr = '\0';

	switch (pmode) {
	case 'X':
		hic = 'A' - 10;
	case 'p':
	case 'x':
		base = 16;
		break;
	case 'i':
	case 'd':
		sgnd = 1;
		base = 10;
		break;
	case 'u':
		base = 10;
		break;
	case 'o':
		base = 8;
		break;
	case 'b':
		base = 2;
		break;
	default:
		return "<?>";
	}

	if (sgnd && ((int64_t) val < 0))
		val = -(int64_t) val;
	else
		sgnd = 0;

	do {
		rem = val % base;
		if ((ptr - 1) >= buf)
			*--ptr = rem < 10 ? rem + loc : rem + hic;
	} while (val /= base);

	if ((ptr - 1) >= buf) {
		if (sgnd)
			*--ptr = '-';
		else if (opt & O_SPACE)
			*--ptr = ' ';
		else if (opt & O_SIGN)
			*--ptr = '+';
	}

	return ptr;
}

int vprintf(const char *fmt, va_list ap)
{
	const char *p = fmt;
	unsigned lmod, hmod, opt, mwidth, len, maxlen;
	char *str, buf[_BUFSZ];

	while (1) {
		while (*p && *p != '%')
			putc(*p++);

		if (!*p)
			return 0;
		p++;

		opt = 0;
		while (*p) {
			switch (*p) {
			case '0':
				opt |= O_ZERO;
				break;
			case ' ':
				opt |= O_SPACE;
				break;
			case '+':
				opt |= O_SIGN;
				break;
			case '-':
				opt |= O_LEFT;
				break;
			case '#':
				opt |= O_ALT;
				break;
			default:
				goto _width;
			}
			p++;
		}

	      _width:
		maxlen = 0;
		mwidth = 0;
		while (*p) {
			switch (*p) {
			case '0'...'9':
				mwidth *= 10;
				mwidth += *p - '0';
				break;
			case '.':
				p++;
				goto _prec;
			default:
				goto _size;
			}
			p++;
		}

	      _prec:
		while (*p) {
			switch (*p) {
			case '0'...'9':
				maxlen *= 10;
				maxlen += *p - '0';
				break;
			default:
				goto _size;
			}
			p++;
		}

	      _size:
		lmod = 0;
		hmod = 0;
		while (*p) {
			switch (*p) {
			case 'l':
				lmod++;
				break;
			case 'h':
				hmod++;
				break;
			default:
				goto _types;
			}
			p++;
		}

	      _types:
#define __addchar(_c) do { buf[0] = _c; buf[1] = '\0'; str = buf; } while(0)
		str = NULL;

		switch (*p) {
		case '%':
			__addchar('%');
			break;
		case 'c':
			__addchar(va_arg(ap, int));
			break;
		case 's':
			str = va_arg(ap, char *);
			if (!str)
				str = "(NULL)";
			break;
		case 'p':
			lmod = 1;
		case 'd':
		case 'X':
		case 'x':
		case 'u':
		case 'o':
		case 'b':{
#define OP_VA_ARG(_ap, _op, _type1, _type2) \
	(_op ? va_arg(_ap, _type1) : va_arg(_ap, _type2))
				int sgnd = 0;
				uint64_t val;

				if (*p == 'd')
					sgnd = 1;

				if (lmod >= 2)
					val = OP_VA_ARG(ap, sgnd, int64_t,
							uint64_t);
				else if (lmod)
					val = OP_VA_ARG(ap, sgnd, long,
							unsigned long);
				else if (hmod)
					val = va_arg(ap, int) & 0xffff;
				else
					val = OP_VA_ARG(ap, sgnd, int,
							unsigned);

				str = __toa(buf, val, *p, opt);
				break;
			}
		default:
			putc('<');
			putc('?');
			putc('>');
		case '\0':
			return 0;
		}

		len = 0;
		if (mwidth >= 0) {
			char *ptr = str;
			while (*ptr++)
				len++;
		}

		if (maxlen == 0)
			maxlen = len;
		if (len > maxlen)
			len = maxlen;

		if (~opt & O_LEFT)
			while (len++ < mwidth)
				if (opt & O_ZERO)
					putc('0');
				else
					putc(' ');

		while (maxlen--)
			putc(*str++);

		if (opt & O_LEFT)
			while (len++ < mwidth)
				putc(' ');

		p++;
	}
}
