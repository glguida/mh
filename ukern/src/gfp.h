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


#ifndef __ukern_gfp_h
#define __ukern_gfp_h

#include <uk/queue.h>

typedef struct pgzone {
	LIST_ENTRY(pgzone) list;
	u_long type;
	u_long ppn;
	u_long size;
} pgzone_t;

#define GFP_LOKERN_ONLY 1
#define GFP_KERN_ONLY   2
#define GFP_HIGH_ONLY   4

#define GFP_LOKERN (GFP_LOKERN_ONLY)
#define GFP_KERN   (GFP_KERN_ONLY | GFP_LOKERN)
#define GFP_HIGH   (GFP_HIGH_ONLY | GFP_KERN)
#define GFP_DEFAULT GFP_KERN

void getfreepages_init(void);
long getfreepages(u_long n, u_long type, u_long flags);
void freepages(u_long ppn, u_long n);

#define getfreepage(_t) getfreepages(1, (_t), GFP_KERN)
#define freepage(_p) freepages((_p), 1)

#endif
