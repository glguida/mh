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


#ifndef __uk_types_h
#define __uk_types_h

#include <uk/cdefs.h>

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

typedef uint32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;

typedef unsigned pmap_prot_t;

#define	S_IRWU	0000600		/* RW mask for owner */
#define	S_IRUSR	0000400		/* R for owner */
#define	S_IWUSR	0000200		/* W for owner */
#define	S_IRWG	0000060		/* RW mask for group */
#define	S_IRGRP	0000040		/* R for group */
#define	S_IWGRP	0000020		/* W for group */
#define	S_IRWO	0000006		/* RW mask for other */
#define	S_IROTH	0000004		/* R for other */
#define	S_IWOTH	0000002		/* W for other */
typedef uint32_t mode_t;

#include <uk/exttypes.h>

#endif
