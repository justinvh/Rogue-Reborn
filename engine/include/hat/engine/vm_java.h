/*
===========================================================================
Copyright (C) 2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
#ifndef VM_JAVA_H
#define VM_HAVA_H

#if defined(USE_JAVA)

#ifdef USE_LOCAL_HEADERS
#include "../java/jni.h"
#else
#include <jni.h>
#endif

JNIEnv         *javaEnv;
JavaVM         *javaVM;


void			JVM_Init(void);
void			JVM_Shutdown(void);

void			ConvertJavaString(char *dest, jstring jstr, int destsize);

#define CheckException() CheckException_(__FILE__, __LINE__)
qboolean		CheckException_(char *filename, int linenum);

jobject			Java_NewVector3f(const vec3_t v);
jobject			Java_NewAngle3f(float pitch, float yaw, float roll);
jobject         Java_NewQuat4f(const quat_t q);
jobject			Java_NewTrajectory(const trajectory_t * t);
jobject			Java_NewUserCommand(const usercmd_t * ucmd);
jobjectArray	Java_NewConsoleArgs();

#endif

#endif // VM_JAVA_H
