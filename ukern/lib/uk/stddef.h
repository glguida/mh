#ifndef _uk_stddef_h
#define _uk_stddef_h

#define	MIN(a,b)	((/*CONSTCOND*/(a)<(b))?(a):(b))
#define	MAX(a,b)	((/*CONSTCOND*/(a)>(b))?(a):(b))
#define offsetof(type, member)	__builtin_offsetof(type, member)

#endif
