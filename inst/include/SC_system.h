/*******************************************************************************
 *  StatConn: Connector interface between application and interpreter language
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
 ******************************************************************************/

#ifndef _SC_SYSTEM_H_
#define _SC_SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* set our own defines for operating systems */
#if defined(Win32)
#  define __WINDOWS__ 1
#else
#  define __LINUX__ 1
#endif

/* system-specifics should be moved to some include file */
#if defined(__WINDOWS__)
#include <windows.h>
#define SYSCALL WINAPI
#define EXPORT

/* entry points */
#if defined(_WIN64)
#define SC_PROXY_GET_OBJECT_FUN "SC_Proxy_get_object"
#define BDX_GET_VTBL_FUN "BDX_get_vtbl"
#define BDX_GET_VTBL2_FUN "BDX_get_vtbl2"
#else
#define SC_PROXY_GET_OBJECT_FUN "SC_Proxy_get_object@8"
#define BDX_GET_VTBL_FUN "_BDX_get_vtbl@8"
#define BDX_GET_VTBL2_FUN "_BDX_get_vtbl2@20"
#endif

#else
#define SYSCALL
#define EXPORT

/* entry points */
#define SC_PROXY_GET_OBJECT_FUN "SC_Proxy_get_object"
#define BDX_GET_VTBL_FUN "BDX_get_vtbl"
#define BDX_GET_VTBL2_FUN "BDX_get_vtbl2"

#endif


#ifdef __cplusplus
}
#endif

#endif
