/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

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
//
/**********************************************************************
    UI_ATOMS.C

    User interface building blocks and support functions.
**********************************************************************/
#include <hat/gui/ui_local.h>
#include <hat/v8/easy.hpp>
#include <string>

uiStatic_t      uis;
qboolean        m_entersound;	// after a frame, so caching won't disrupt the sound

void QDECL Com_Error(int level, const char *error, ...)
{
    va_list         argptr;
    char            text[1024];

    va_start(argptr, error);
    Q_vsnprintf(text, sizeof(text), error, argptr);
    va_end(argptr);

    trap_Error(level, va("%s", text));
}

void QDECL Com_Printf(const char *msg, ...)
{
    va_list         argptr;
    char            text[1024];

    va_start(argptr, msg);
    Q_vsnprintf(text, sizeof(text), msg, argptr);
    va_end(argptr);

    trap_Print(va("%s", text));
}

void QDECL Com_Warning(const char *msg, ...)
{
    va_list         argptr;
    char            text[1024];

    va_start(argptr, msg);
    Q_vsnprintf(text, sizeof(text), msg, argptr);
    va_end(argptr);

    trap_Warning(va("%s", text));
}

