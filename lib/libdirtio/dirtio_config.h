#ifndef _sys_dirtio_config_h
#define _sys_dirtio_config_h

/*
 * The following is the interface for a dirtio device configuration,
 * and is heavily inspired from the virtio's equivalent interface.
 */

/* This header, excluding the #ifdef __KERNEL__ part, is BSD licensed so
 * anyone can use the definitions to implement compatible drivers/servers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. */

/* As the virtio counterpart, dirtio devices also use a standardized
 * configuration space to define their features and pass configuration
 * information, but each implementation can store and access that
 * space differently. */
#include <sys/types.h>

/* Status byte for guest to report progress, and synchronize features. */
/* We have seen device and processed generic fields (DIRTIO_CONFIG_F_DIRTIO) */
#define DIRTIO_CONFIG_S_ACKNOWLEDGE	1
/* We have found a driver for the device. */
#define DIRTIO_CONFIG_S_DRIVER		2
/* Driver has used its parts of the config, and is happy */
#define DIRTIO_CONFIG_S_DRIVER_OK	4
/* Driver has finished configuring features */
#define DIRTIO_CONFIG_S_FEATURES_OK	8
/* We've given up on this device. */
#define DIRTIO_CONFIG_S_FAILED		0x80

/* Some dirtio feature bits (currently bits 28 through 32) are reserved for the
 * transport being used (eg. dirtio_ring), the rest are per-device feature
 * bits. */
#define DIRTIO_TRANSPORT_F_START	28
#define DIRTIO_TRANSPORT_F_END		33

/* v1.0 compliant. */
#define DIRTIO_F_VERSION_1		32

#endif
