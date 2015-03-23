#ifndef __uk_types_h
#define __uk_types_h

#include <uk/cdefs.h>
/* XXX: feature test? */

#define NBBY 8

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef long intptr_t;
typedef unsigned long uintptr_t;

typedef unsigned long long intmax_t;

#include <machine/uk/types.h>

#include <machine/uk/ansi.h>
#include <machine/uk/int_types.h>

#if defined _UKERNEL
typedef uint64_t cpumask_t;
#endif

#endif
