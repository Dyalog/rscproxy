/*******************************************************************************
 *  BDX: Binary Data eXchange format library
 *  Copyright (C) 1999--2009 Thomas Baier
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  helper functions for BDX (e.g. memory management)
 *
 ******************************************************************************/

#ifndef _BDX_UTIL_H_
#define _BDX_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "SC_system.h"

/* tracing */
#ifdef _IN_RPROXY_
#include "rproxy.h"
#define BDX_TRACE RPROXY_TRACE
#define BDX_ERR RPROXY_ERR
#else
#define BDX_TRACE(x) bdx_trace_ ## x
#define BDX_ERR(x)   bdx_trace_ ## x
int bdx_trace_printf(char const*,...);
#endif

/* forward declaration */
struct _BDX_Data;

/*
 * allocate/release the BDX memory
 */
struct _BDX_Data* SYSCALL bdx_alloc(void);
void SYSCALL bdx_free(struct _BDX_Data* data);
void SYSCALL bdx_trace(struct _BDX_Data* data); /* trace using OutputDebugString() */

/*
 * control BDX data conversion
 */
void bdx_set_datamode(unsigned long pDM);
unsigned long bdx_get_datamode(void);

#ifdef __cplusplus
}
#endif

#endif
