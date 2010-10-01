/*
===========================================================================
Parts of this file are based on the ETPub source code under GPL.
http://trac2.assembla.com/etpub/browser/trunk/src/game/g_lua.h
rev 170 + rev 192
Code by quad and pheno

Ported to Xreal by Andrew "DerSaidin" Browne.

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// lua_et.h -- header for main library

#ifndef _LUA_ET_H
#define _LUA_ET_H

#include "g_local.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define FIELD_INT			0
#define FIELD_STRING		1
#define FIELD_FLOAT			2
#define FIELD_ENTITY		3
#define FIELD_VEC3			4
#define FIELD_INT_ARRAY		5
#define FIELD_TRAJECTORY	6
#define FIELD_FLOAT_ARRAY	7

#define FIELD_FLAG_GENTITY	1	// marks a gentity_s field
#define FIELD_FLAG_GCLIENT	2	// marks a gclient_s field
#define FIELD_FLAG_NOPTR	4
#define FIELD_FLAG_READONLY	8	// read-only access

// macros to add gentity and gclient fields
#define _et_gentity_addfield(n, t, f) {#n, t, offsetof(struct gentity_s, n), FIELD_FLAG_GENTITY + f}
#define _et_gentity_addfieldalias(n, a, t, f) {#n, t, offsetof(struct gentity_s, a), FIELD_FLAG_GENTITY + f}
#define _et_gclient_addfield(n, t, f) {#n, t, offsetof(struct gclient_s, n), FIELD_FLAG_GCLIENT + f}
#define _et_gclient_addfieldalias(n, a, t, f) {#n, t, offsetof(struct gclient_s, a), FIELD_FLAG_GCLIENT + f}

typedef struct
{
	const char     *name;
	int             type;
	unsigned long   mapping;
	int             flags;
} gentity_field_t;

#endif							/* ifndef _G_LUA_H */
