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

#include <hat/engine/q_shared.h>

#if defined(USE_JAVA)

#ifdef WIN32
#define DEFAULT_JAVA_LIB "jre/bin/client/jvm.dll"
#elif defined(MACOS_X)
#define DEFAULT_JAVA_LIB "java.dylib"
#else
#ifdef __x86_64__
#define DEFAULT_JAVA_LIB "/usr/lib/jvm/java-6-openjdk/jre/lib/amd64/server/libjvm.so"
#else
#define DEFAULT_JAVA_LIB "/usr/lib/jvm/java-6-openjdk/jre/lib/i386/client/libjvm.so"
#endif
#endif

#include <hat/engine/qcommon.h>
#include "vm_java.h"
#include "../sys/sys_loadlib.h"

#include "java/xreal_Engine.h"
#include "java/xreal_CVar.h"

static cvar_t  *jvm_javaLib;
static cvar_t  *jvm_useJITCompiler;
static cvar_t  *jvm_useJAR;
static cvar_t  *jvm_remoteDebugging;
static cvar_t  *jvm_profiling;
static cvar_t  *jvm_verboseJNI;
static cvar_t  *jvm_verboseClass;
static cvar_t  *jvm_verboseGC;
static cvar_t  *jvm_policyFile;

JNIEnv         *javaEnv;
JavaVM         *javaVM;
static void    *javaLib = NULL;
qboolean        javaEnabled = qfalse;
qboolean        javaVMIsOurs = qfalse;

#define USE_JAVA_DLOPEN 1

#if defined(USE_JAVA_DLOPEN)

static jint (JNICALL *QJNI_CreateJavaVM) (JavaVM ** p_vm, void ** p_env, void *vm_args);
static jint (JNICALL *QJNI_GetCreatedJavaVMs) (JavaVM ** vmBuf, jsize bufLen, jsize * nVMs);

static void    *GPA(char *str)
{
	void           *rv;

	rv = Sys_LoadFunction(javaLib, str);
	if(!rv)
	{
		Com_Printf("Can't load symbol %s\n", str);
		javaEnabled = qfalse;
		return NULL;
	}
	else
	{
		Com_DPrintf("Loaded symbol %s (0x%p)\n", str, rv);
		return rv;
	}
}


static void JVM_JNI_Shutdown(void)
{
	if(javaLib)
	{
		Sys_UnloadLibrary(javaLib);
		javaLib = NULL;
	}

	QJNI_CreateJavaVM = NULL;
	QJNI_GetCreatedJavaVMs = NULL;
}

static qboolean JVM_JNI_Init()
{
	if(javaLib)
		return qtrue;

	Com_Printf("Loading \"%s\"...\n", jvm_javaLib->string);
	if((javaLib = Sys_LoadLibrary(jvm_javaLib->string)) == 0)
	{
#ifdef _WIN32
		return qfalse;
#else
		char            fn[1024];

		Com_Printf("JVM_JNI_Init() failed:\n\"%s\"\n", Sys_LibraryError());

		Q_strncpyz(fn, Sys_Cwd(), sizeof(fn));
		strncat(fn, "/", sizeof(fn) - strlen(fn) - 1);
		strncat(fn, jvm_javaLib->string, sizeof(fn) - strlen(fn) - 1);

		if((javaLib = Sys_LoadLibrary(fn)) == 0)
		{
			Com_Printf("JVM_JNI_Init() failed:\n\"%s\"\n", Sys_LibraryError());
			return qfalse;
		}
#endif							/* _WIN32 */
	}

	javaEnabled = qtrue;

	QJNI_CreateJavaVM = GPA("JNI_CreateJavaVM");
	QJNI_GetCreatedJavaVMs = GPA("JNI_GetCreatedJavaVMs");

	if(!javaEnabled)
	{
		//JVM_JNI_Shutdown();
		return qfalse;
	}

	return qtrue;
}

#endif

// ====================================================================================


// handles to java.lang.Throwable class
static jclass   class_Throwable = NULL;
static jmethodID method_Throwable_printStackTrace = NULL;
//static jmethodID method_Throwable_fillInStackTrace = NULL;
static jmethodID method_Throwable_toString = NULL;
jmethodID method_Throwable_getMessage = NULL;


// handles to java.io.StringWriter class
static jclass	class_StringWriter = NULL;
static jmethodID method_StringWriter_ctor = NULL;
static jmethodID method_StringWriter_toString = NULL;

// handles to java.io.PrintWriter class
static jclass	class_PrintWriter = NULL;
static jmethodID method_PrintWriter_ctor = NULL;


// handles to java.lang.String class
static jclass	class_String = NULL;

// handles to the javax.vecmath.Tuple3f class
static jclass   class_Tuple3f = NULL;
static jmethodID method_Tuple3f_ctor = NULL;

// handles to the javax.vecmath.Point3f class
static jclass   class_Point3f = NULL;
static jmethodID method_Point3f_ctor = NULL;

// handles to the javax.vecmath.Vector3f class
static jclass   class_Vector3f = NULL;
static jmethodID method_Vector3f_ctor = NULL;

// handles to the xreal.Angle3f class
static jclass   class_Angle3f = NULL;
static jmethodID method_Angle3f_ctor = NULL;

// handles to the javax.vecmath.Quat4f class
static jclass   class_Quat4f = NULL;
static jmethodID method_Quat4f_ctor = NULL;

// handles to the xreal.Trajectory class
static jclass   class_Trajectory = NULL;
static jmethodID method_Trajectory_ctor = NULL;




void Misc_javaRegister()
{
	class_Throwable = (*javaEnv)->FindClass(javaEnv, "java/lang/Throwable");
	if(!class_Throwable)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.lang.Throwable");
	}

	method_Throwable_printStackTrace = (*javaEnv)->GetMethodID(javaEnv, class_Throwable, "printStackTrace", "(Ljava/io/PrintWriter;)V");
	if(!method_Throwable_printStackTrace)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.lang.Throwable.printStackTrace() method");
	}

	/*
	method_Throwable_fillInStackTrace = (*javaEnv)->GetMethodID(javaEnv, class_Throwable, "fillInStackTrace", "()Ljava/lang/Throwable;");
	if(!method_Throwable_fillInStackTrace)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.lang.Throwable.fillInStackTrace() method");
	}
	*/

	method_Throwable_getMessage = (*javaEnv)->GetMethodID(javaEnv, class_Throwable, "getMessage", "()Ljava/lang/String;");
	if(!method_Throwable_getMessage)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.lang.Throwable.getMessage() method");
	}

	method_Throwable_toString = (*javaEnv)->GetMethodID(javaEnv, class_Throwable, "toString", "()Ljava/lang/String;");
	if(!method_Throwable_toString)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.lang.Throwable.toString() method");
	}


	// get class java.io.StringWriter
	class_StringWriter = (*javaEnv)->FindClass(javaEnv, "java/io/StringWriter");
	if(!class_StringWriter)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.io.StringWriter");
	}

	method_StringWriter_ctor = (*javaEnv)->GetMethodID(javaEnv, class_StringWriter, "<init>", "()V");
	if(!method_StringWriter_ctor)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.io.StringWriter constructor method");
	}

	method_StringWriter_toString = (*javaEnv)->GetMethodID(javaEnv, class_StringWriter, "toString", "()Ljava/lang/String;");
	if(!method_StringWriter_toString)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.io.StringWriter.toString() method");
	}




	// get class java.io.PrintWriter
	class_PrintWriter = (*javaEnv)->FindClass(javaEnv, "java/io/PrintWriter");
	if(!class_PrintWriter)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.io.PrintWriter");
	}

	method_PrintWriter_ctor = (*javaEnv)->GetMethodID(javaEnv, class_PrintWriter, "<init>", "(Ljava/io/Writer;)V");
	if(!method_PrintWriter_ctor)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.io.PrintWriter constructor method");
	}



	// get class java.lang.String
	class_String = (*javaEnv)->FindClass(javaEnv, "java/lang/String");
	if(!class_String)
	{
		Com_Error(ERR_FATAL, "Couldn't find java.lang.String");
	}



	// now that the java.lang.Class, java.lang.Throwable handles are obtained
	// we can start checking for exceptions

	class_Tuple3f = (*javaEnv)->FindClass(javaEnv, "javax/vecmath/Tuple3f");
	if(CheckException() || !class_Tuple3f)
	{
		Com_Error(ERR_FATAL, "Couldn't find javax.vecmath.Tuple3f");
	}

	method_Tuple3f_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Tuple3f, "<init>", "(FFF)V");
	if(CheckException() || !method_Tuple3f_ctor)
	{
		Com_Error(ERR_FATAL, "Couldn't find javax.vecmath.Tuple3f constructor method");
	}



	class_Point3f = (*javaEnv)->FindClass(javaEnv, "javax/vecmath/Point3f");
	if(CheckException() || !class_Point3f)
	{
		Com_Error(ERR_FATAL, "Couldn't find javax.vecmath.Point3f");
	}

	method_Point3f_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Point3f, "<init>", "(FFF)V");
	if(CheckException() || !method_Point3f_ctor)
	{
		Com_Error(ERR_FATAL, "Couldn't find javax.vecmath.Point3f constructor method");
	}



	class_Vector3f = (*javaEnv)->FindClass(javaEnv, "javax/vecmath/Vector3f");
	if(CheckException() || !class_Vector3f)
	{
		Com_Error(ERR_FATAL, "Couldn't find javax.vecmath.Vector3f");
	}

	method_Vector3f_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Vector3f, "<init>", "(FFF)V");
	if(CheckException() || !method_Vector3f_ctor)
	{
		Com_Error(ERR_FATAL, "Couldn't find javax.vecmath.Vector3f constructor method");
	}



	class_Angle3f = (*javaEnv)->FindClass(javaEnv, "xreal/Angle3f");
	if(CheckException() || !class_Angle3f)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.Angle3f");
	}

	method_Angle3f_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Angle3f, "<init>", "(FFF)V");
	if(CheckException() || !method_Angle3f_ctor)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.Angle3f constructor method");
	}


	class_Quat4f = (*javaEnv)->FindClass(javaEnv, "javax/vecmath/Quat4f");
	if(CheckException() || !class_Quat4f)
	{
		Com_Error(ERR_FATAL, "Couldn't find javax.vecmath.Quat4f");
	}

	method_Quat4f_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Quat4f, "<init>", "(FFFF)V");
	if(CheckException() || !method_Quat4f_ctor)
	{
		Com_Error(ERR_FATAL, "Couldn't find javax.vecmath.Quat4f constructor method");
	}



	class_Trajectory = (*javaEnv)->FindClass(javaEnv, "xreal/Trajectory");
	if(CheckException() || !class_Trajectory)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.Trajectory");
	}

	// int trType, int trTime, int trDuration, float trAcceleration,
	//float trBaseX, float trBaseY, float trBaseZ, float trBaseW,
	//float trDeltaX, float trDeltaY, float trDeltaZ, float trDeltaW)
	method_Trajectory_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Trajectory, "<init>", "(IIIFFFFFFFFF)V");
	if(CheckException() || !method_Trajectory_ctor)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.Trajectory constructor method");
	}
}

void Misc_javaDetach()
{
	if(class_Throwable)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Throwable);
		class_Throwable = NULL;
	}

	if(class_String)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_String);
		class_String = NULL;
	}

	if(class_Tuple3f)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Tuple3f);
		class_Tuple3f = NULL;
	}

	if(class_Point3f)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Point3f);
		class_Point3f = NULL;
	}

	if(class_Vector3f)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Vector3f);
		class_Vector3f = NULL;
	}

	if(class_Angle3f)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Angle3f);
		class_Angle3f = NULL;
	}

	if(class_Quat4f)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Quat4f);
		class_Quat4f = NULL;
	}

	if(class_Trajectory)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Trajectory);
		class_Trajectory = NULL;
	}
}

jobject Java_NewVector3f(const vec3_t v)
{
	jobject obj = NULL;

	if(!v)
		return NULL;

	if(class_Vector3f)
	{
		//Com_Printf("Java_NewVector3f(%i, %i, %i)\n", (int)v[0], (int)v[1], (int)v[2]);

		obj = (*javaEnv)->NewObject(javaEnv, class_Vector3f, method_Vector3f_ctor, v[0], v[1], v[2]);
	}

	return obj;
}

jobject Java_NewAngle3f(float pitch, float yaw, float roll)
{
	jobject obj = NULL;

	if(class_Angle3f)
	{
		obj = (*javaEnv)->NewObject(javaEnv, class_Angle3f, method_Angle3f_ctor, pitch, yaw, roll);
	}

	return obj;
}


// OPTIMIZE: Quat4f constructor normalizes the input and calls sqrt() ..

jobject Java_NewQuat4f(const quat_t q)
{
	jobject obj = NULL;

	if(class_Quat4f)
	{
		obj = (*javaEnv)->NewObject(javaEnv, class_Quat4f, method_Quat4f_ctor, q[0], q[1], q[2], q[3]);
	}

	return obj;
}

jobject Java_NewTrajectory(const trajectory_t * t)
{
	jobject obj = NULL;

	if(class_Trajectory)
	{
		obj = (*javaEnv)->NewObject(javaEnv, class_Trajectory, method_Trajectory_ctor, t->trType, t->trTime, t->trDuration, t->trAcceleration,
				t->trBase[0], t->trBase[1], t->trBase[2], t->trBase[3],
				t->trDelta[0], t->trDelta[1], t->trDelta[2], t->trDelta[3]);
	}

	return obj;
}


// ====================================================================================

/*
 * Class:     xreal_CVar
 * Method:    register0
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
jint JNICALL Java_xreal_CVar_register0(JNIEnv * env, jclass cls, jstring jname, jstring jvalue, jint flags)
{
	//Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);

	char           *varName;
	char           *defaultValue;
	cvar_t         *cv;

	varName = (char *)((*env)->GetStringUTFChars(env, jname, 0));
	defaultValue = (char *)((*env)->GetStringUTFChars(env, jvalue, 0));

	cv = Cvar_Get(varName, defaultValue, flags);

	(*env)->ReleaseStringUTFChars(env, jname, varName);
	(*env)->ReleaseStringUTFChars(env, jvalue, defaultValue);

	//CheckException();

	return (jint) cv;
}

/*
 * Class:     xreal_CVar
 * Method:    set0
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
void JNICALL Java_xreal_CVar_set0(JNIEnv * env, jclass cls, jstring jname, jstring jvalue)
{
	// Cvar_Set((const char *)VMA(1), (const char *)VMA(2));

	char           *varName;
	char           *value;

	varName = (char *)((*env)->GetStringUTFChars(env, jname, 0));
	value = (char *)((*env)->GetStringUTFChars(env, jvalue, 0));

	Cvar_Set(varName, value);

	(*env)->ReleaseStringUTFChars(env, jname, varName);
	(*env)->ReleaseStringUTFChars(env, jvalue, value);

	//CheckException();
}

/*
 * Class:     xreal_CVar
 * Method:    set0
 * Signature: (Ljava/lang/String;)V
 */
void JNICALL Java_xreal_CVar_reset0(JNIEnv * env, jclass cls, jstring jname)
{
	char           *varName;

	varName = (char *)((*env)->GetStringUTFChars(env, jname, 0));
	
	Cvar_Reset(varName);

	(*env)->ReleaseStringUTFChars(env, jname, varName);

	//CheckException();
}

/*
 * Class:     xreal_CVar
 * Method:    getString0
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_xreal_CVar_getString0(JNIEnv * env, jclass cls, jint ptr)
{
	cvar_t         *cv = (cvar_t *) ptr;

	return (*env)->NewStringUTF(env, cv->string);
}

/*
 * Class:     xreal_CVar
 * Method:    getValue0
 * Signature: (I)F
 */
jfloat JNICALL Java_xreal_CVar_getValue0(JNIEnv * env, jclass cls, jint ptr)
{
	cvar_t         *cv = (cvar_t *) ptr;

	return cv->value;
}

/*
 * Class:     xreal_CVar
 * Method:    getInteger0
 * Signature: (I)I
 */
jint JNICALL Java_xreal_CVar_getInteger0(JNIEnv * env, jclass cls, jint ptr)
{
	cvar_t         *cv = (cvar_t *) ptr;

	return cv->integer;
}



// handle to CVar class
static jclass   class_CVar;
static JNINativeMethod CVar_methods[] = {
	{"register0", "(Ljava/lang/String;Ljava/lang/String;I)I", Java_xreal_CVar_register0},
	{"set0", "(Ljava/lang/String;Ljava/lang/String;)V", Java_xreal_CVar_set0},
	{"reset0", "(Ljava/lang/String;)V", Java_xreal_CVar_reset0},
	{"getString0", "(I)Ljava/lang/String;", Java_xreal_CVar_getString0},
	{"getValue0", "(I)F", Java_xreal_CVar_getValue0},
	{"getInteger0", "(I)I", Java_xreal_CVar_getInteger0},
};

void CVar_javaRegister()
{
	class_CVar = (*javaEnv)->FindClass(javaEnv, "xreal/CVar");
	if(CheckException() || !class_CVar)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.CVar");
	}

	(*javaEnv)->RegisterNatives(javaEnv, class_CVar, CVar_methods, sizeof(CVar_methods) / sizeof(CVar_methods[0]));
	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't register native methods for xreal.CVar");
	}
}


void CVar_javaDetach()
{
	if(class_CVar)
		(*javaEnv)->UnregisterNatives(javaEnv, class_CVar);

	(*javaEnv)->DeleteLocalRef(javaEnv, class_CVar);
}

// ====================================================================================


// handle to UserCommand class
static jclass   class_UserCommand = NULL;
static jmethodID method_UserCommand_ctor = NULL;

void UserCommand_javaRegister()
{
	class_UserCommand = (*javaEnv)->FindClass(javaEnv, "xreal/UserCommand");
	if(CheckException() || !class_UserCommand)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.UserCommand");
	}

	//UserCommand(int serverTime, int pitch, int yaw, int roll, int buttons, byte weapon, byte forwardmove, byte rightmove, byte upmove)
	method_UserCommand_ctor = (*javaEnv)->GetMethodID(javaEnv, class_UserCommand, "<init>", "(ISSSIBBBB)V");
}

void UserCommand_javaDetach()
{
	if(class_UserCommand)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_UserCommand);
		class_UserCommand = NULL;
	}
}

jobject Java_NewUserCommand(const usercmd_t * ucmd)
{
	jobject obj = NULL;

	if(class_UserCommand)
	{
		// create new player object
		obj = (*javaEnv)->NewObject(javaEnv, class_UserCommand, method_UserCommand_ctor,
				ucmd->serverTime,
				ucmd->angles[PITCH],
				ucmd->angles[YAW],
				ucmd->angles[ROLL],
				ucmd->buttons,
				ucmd->weapon,
				ucmd->forwardmove,
				ucmd->rightmove,
				ucmd->upmove);
	}

	return obj;
}

// ====================================================================================

/*
 * Class:     xreal_Engine
 * Method:    print
 * Signature: (Ljava/lang/String;)V
 */
void JNICALL Java_xreal_Engine_print(JNIEnv * env, jclass cls, jstring js)
{
//	char            string[MAXPRINTMSG];
	char           *s;

	if(js == NULL)
		return;

	s = (char *)((*env)->GetStringUTFChars(env, js, 0));

	Com_Printf("%s", s);

	(*env)->ReleaseStringUTFChars(env, js, s);

//	ConvertJavaString(string, js, sizeof(string));
//	Com_Printf("%s", string);
}

/*
 * Class:     xreal_Engine
 * Method:    error
 * Signature: (Ljava/lang/String;)V
 */
void JNICALL Java_xreal_Engine_error(JNIEnv *env, jclass cls, jstring js)
{
//	char            string[MAXPRINTMSG];
	char           *s;

	if(js == NULL)
		return;

	s = (char *)((*env)->GetStringUTFChars(env, js, 0));

	Com_Error(ERR_DROP, "%s", s);

	(*env)->ReleaseStringUTFChars(env, js, s);

//	ConvertJavaString(string, js, sizeof(string));
//	Com_Error(ERR_DROP, "%s", string);
}

/*
 * Class:     xreal_Engine
 * Method:    getTimeInMilliseconds
 * Signature: ()I
 */
jint JNICALL Java_xreal_Engine_getTimeInMilliseconds(JNIEnv *env, jclass cls)
{
	return Sys_Milliseconds();
}


/*
 * Class:     xreal_Engine
 * Method:    getConsoleArgc
 * Signature: ()I
 */
/*
jint JNICALL Java_xreal_Engine_getConsoleArgc(JNIEnv *env, jclass cls)
{
	return Cmd_Argc();
}
*/

/*
 * Class:     xreal_Engine
 * Method:    getConsoleArgv
 * Signature: (I)Ljava/lang/String;
 */
/*
jstring JNICALL Java_xreal_Engine_getConsoleArgv(JNIEnv *env, jclass cls, jint arg)
{
	return (*env)->NewStringUTF(env, Cmd_Argv(arg));
}
*/

/*
 * Class:     xreal_Engine
 * Method:    getConsoleArgs
 * Signature: ()Ljava/lang/String;
 */
/*
jstring JNICALL Java_xreal_Engine_getConsoleArgs(JNIEnv *env, jclass cls)
{
	return (*env)->NewStringUTF(env, Cmd_Args());
}
*/



jobjectArray Java_NewConsoleArgs()
{
#if 1
	int				i, argc;
	jobjectArray 	argsArray = NULL;

	argc = Cmd_Argc();
	if(argc <= 0)
		return NULL;

	argsArray = (*javaEnv)->NewObjectArray(javaEnv, argc, class_String, NULL);

	for(i = 0; i < argc; i++) {

		jstring	argv = (*javaEnv)->NewStringUTF(javaEnv, Cmd_Argv(i));

		(*javaEnv)->SetObjectArrayElement(javaEnv, argsArray, i, argv);
	}

	CheckException();

	return argsArray;
#else
	return NULL;
#endif
}


/*
 * Class:     xreal_Engine
 * Method:    getConsoleArgs
 * Signature: ()[Ljava/lang/String;
 */
jobjectArray JNICALL Java_xreal_Engine_getConsoleArgs(JNIEnv *env, jclass cls)
{
	return Java_NewConsoleArgs();
}

/*
 * Class:     xreal_Engine
 * Method:    sendConsoleCommand
 * Signature: (ILjava/lang/String;)V
 */
void JNICALL Java_xreal_Engine_sendConsoleCommand(JNIEnv *env, jclass cls, jint jexec_when, jstring jtext)
{
	char           *text;

	text = (char *)((*env)->GetStringUTFChars(env, jtext, 0));

	Cbuf_ExecuteText(jexec_when, text);

	(*env)->ReleaseStringUTFChars(env, jtext, text);
}

/*
 * Class:     xreal_Engine
 * Method:    readFile
 * Signature: (Ljava/lang/String;)[B
 */
jbyteArray JNICALL Java_xreal_Engine_readFile(JNIEnv *env, jclass cls, jstring jfileName)
{
	char           *fileName;
	jbyteArray		array;
	int				length;
	byte           *buf;

	fileName = (char *)((*env)->GetStringUTFChars(env, jfileName, 0));

	length = FS_ReadFile(fileName, (void **)&buf);
	if(!buf)
	{
		return NULL;
	}

	//Com_Printf("Java_xreal_Engine_readFile: file '%s' has length = %i\n", filename, length);

	array = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, array, 0, length, buf);

	(*env)->ReleaseStringUTFChars(env, jfileName, fileName);

	FS_FreeFile(buf);

	return array;
}

/*
 * Class:     xreal_Engine
 * Method:    writeFile
 * Signature: (Ljava/lang/String;[B)V
 */
void JNICALL Java_xreal_Engine_writeFile(JNIEnv *env, jclass cls, jstring jfileName, jbyteArray array)
{
	char           *fileName;
	int				length;
	byte           *buf = NULL;

	fileName = (char *)((*env)->GetStringUTFChars(env, jfileName, 0));
	length = (*env)->GetArrayLength(env, array);

	//Com_Printf("Java_xreal_Engine_writeFile: file '%s' has length = %i\n", fileName, length);

	buf = (byte*) malloc(length);

	(*env)->GetByteArrayRegion(env, array, 0, length, buf);

	FS_WriteFile(fileName, buf, length);

	(*env)->ReleaseStringUTFChars(env, jfileName, fileName);
	(*env)->ReleaseByteArrayElements(env, array, buf, 0);

//	Com_Dealloc(buf);
}

// handle to Engine class
static jclass   class_Engine;
static JNINativeMethod Engine_methods[] = {
	{"print", "(Ljava/lang/String;)V", Java_xreal_Engine_print},
	{"error", "(Ljava/lang/String;)V", Java_xreal_Engine_error},
	{"getTimeInMilliseconds", "()I", Java_xreal_Engine_getTimeInMilliseconds},

//	{"getConsoleArgc", "()I", Java_xreal_Engine_getConsoleArgc},
//	{"getConsoleArgv", "(I)Ljava/lang/String;", Java_xreal_Engine_getConsoleArgv},
//	{"getConsoleArgs", "()Ljava/lang/String;", Java_xreal_Engine_getConsoleArgs},

	{"getConsoleArgs", "()[Ljava/lang/String;", Java_xreal_Engine_getConsoleArgs},


	{"sendConsoleCommand", "(ILjava/lang/String;)V", Java_xreal_Engine_sendConsoleCommand},
	{"readFile", "(Ljava/lang/String;)[B", Java_xreal_Engine_readFile},
	{"writeFile", "(Ljava/lang/String;[B)V", Java_xreal_Engine_writeFile}
};

void Engine_javaRegister()
{
	class_Engine = (*javaEnv)->FindClass(javaEnv, "xreal/Engine");
	if(CheckException() || !class_Engine)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.Engine");
	}

	(*javaEnv)->RegisterNatives(javaEnv, class_Engine, Engine_methods, sizeof(Engine_methods) / sizeof(Engine_methods[0]));
	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't register native methods for xreal.Engine");
	}
}


void Engine_javaDetach()
{
	if(class_Engine)
		(*javaEnv)->UnregisterNatives(javaEnv, class_Engine);

	(*javaEnv)->DeleteLocalRef(javaEnv, class_Engine);
}

// ====================================================================================


/**
 * @brief Convert a Java string (which is Unicode) to reasonable 7-bit ASCII.
 *
 * @author Berry Pederson
 */
void ConvertJavaString(char *dest, jstring jstr, int destsize)
{
	jsize           jStrLen;
	const jchar    *unicodeChars;

//  char           *result;
	int             i;
	char           *p;

	// table for translating accented latin-1 characters to closest non-accented chars
	// Icelandic thorn and eth are difficult, so I just used an asterisk
	// German sz ligature � is also difficult, chose to just put in 's'
	// AE ligatures are just replaced with *
	// perhaps some sort of multi-character substitution scheme would be helpful
	//
	// this translation table starts at decimal 192 (capital A, grave accent: �)
	static char    *translateTable = "AAAAAA*CEEEEIIIIDNOOOOOxOUUUUY*saaaaaa*ceeeeiiii*nooooo/ouuuuy*y";

	if(!jstr)
	{
		Com_Error(ERR_FATAL, "ConvertJavaString: NULL src");
	}

	if(destsize < 1)
	{
		Com_Error(ERR_FATAL, "ConvertJavaString: destsize < 1");
	}



	jStrLen = Q_min((*javaEnv)->GetStringLength(javaEnv, jstr), destsize);

	p = dest;					// = q2java_gi.TagMalloc(jStrLen + 1, TAG_GAME);
	unicodeChars = (*javaEnv)->GetStringChars(javaEnv, jstr, NULL);

	for(i = 0; i < jStrLen; i++)
	{
		jchar           ch = unicodeChars[i];

		if(ch < 192)
			*p++ = (char)ch;
		else
		{
			if(ch < 256)
				*p++ = translateTable[ch - 192];
			else
				*p++ = '*';
		}

	}

	(*javaEnv)->ReleaseStringChars(javaEnv, jstr, unicodeChars);
	*p = 0;

//  return result;
}



static void GetExceptionMessage(jthrowable ex, char *dest, int destsize)
{
	char           *message;
	jstring			msgObject;
	

	//(*javaEnv)->CallVoidMethod(javaEnv, ex, method_Throwable_fillInStackTrace);

	msgObject = (*javaEnv)->CallObjectMethod(javaEnv, ex, method_Throwable_toString);
	message = (char *)((*javaEnv)->GetStringUTFChars(javaEnv, msgObject, 0));

	Q_strncpyz(dest, message, destsize);

	(*javaEnv)->ReleaseStringUTFChars(javaEnv, msgObject, message);
}

static void GetExceptionStackTrace(jthrowable ex, char *dest, int destsize)
{
	char           *message;
	jstring			stringObject;
	jobject			stringWriter;
	jobject			printWriter;

	//(*javaEnv)->CallVoidMethod(javaEnv, ex, method_Throwable_fillInStackTrace);

	stringWriter = (*javaEnv)->NewObject(javaEnv, class_StringWriter, method_StringWriter_ctor);
	printWriter = (*javaEnv)->NewObject(javaEnv, class_PrintWriter, method_PrintWriter_ctor, stringWriter);

	(*javaEnv)->CallVoidMethod(javaEnv, ex, method_Throwable_printStackTrace, printWriter);
	stringObject = (*javaEnv)->CallObjectMethod(javaEnv, stringWriter, method_StringWriter_toString);
	
	message = (char *)((*javaEnv)->GetStringUTFChars(javaEnv, stringObject, 0));

	Q_strncpyz(dest, message, destsize);

	(*javaEnv)->ReleaseStringUTFChars(javaEnv, stringObject, message);
}

qboolean CheckException_(char *filename, int linenum)
{
	jthrowable      ex;
	char			message[MAXPRINTMSG];

	ex = (*javaEnv)->ExceptionOccurred(javaEnv);
	if(!ex)
		return qfalse;

	(*javaEnv)->ExceptionClear(javaEnv);

	Com_Printf(S_COLOR_RED "%s line: %d\n-----------------\n", filename, linenum);

	//(*javaEnv)->CallVoidMethod(javaEnv, ex, method_Throwable_printStackTrace);

	GetExceptionMessage(ex, message, sizeof(message));
	Com_Printf(S_COLOR_RED "message: %s\n", message);

	GetExceptionStackTrace(ex, message, sizeof(message));
	Com_Printf(S_COLOR_RED "stacktrace: %s\n", message);
	Cvar_Set("com_stackTrace", message);

	return qtrue;
}




// ====================================================================================


void JVM_Shutdown(void)
{
	Com_Printf("------- JVM_Shutdown() -------\n");

	if(!javaEnv)
	{
		Com_Printf("Can't stop Java VM, javaEnv pointer was null\n");
		return;
	}

	CheckException();

	UserCommand_javaDetach();
	CVar_javaDetach();
	Engine_javaDetach();
	Misc_javaDetach();

	if(javaVMIsOurs)
	{
		JavaVM         *jvm;

		(*javaEnv)->GetJavaVM(javaEnv, &jvm);
		if((*jvm)->DestroyJavaVM(jvm))
		{
			Com_Printf("Error destroying Java VM\n");
		}
		else
		{
			Com_Printf("Java VM Destroyed\n");
		}

#if defined(USE_JAVA_DLOPEN)
		JVM_JNI_Shutdown();
#endif
	}
	else
	{
		if((*javaVM)->DetachCurrentThread(javaVM))
		{
			Com_Printf("Couldn't detach from existing VM\n");
		}
	}
}



static JavaVMOption* AllocOption(growList_t * growOptions)
{
	JavaVMOption   *option;

	option = Com_Allocate(sizeof(*option));
	Com_Memset(option, 0, sizeof(*option));

	Com_AddToGrowList(growOptions, option);

	return option;
}

void JVM_Init(void)
{
	int				i;
	jint            nVMs = 0;		// number of VM's active
	JavaVMInitArgs  vm_args;

	growList_t      growOptions;
	JavaVMOption   *option;
	JavaVMOption   *options;

	char            classPath[MAX_QPATH];
	char            policyPath[MAX_OSPATH];

	Com_InitGrowList(&growOptions, 5);

	Com_Printf("------- JVM_Init() -------\n");

	jvm_javaLib = Cvar_Get("jvm_javaLib", DEFAULT_JAVA_LIB, CVAR_ARCHIVE | CVAR_LATCH);
	jvm_useJITCompiler = Cvar_Get("jvm_useJITCompiler", "1", CVAR_INIT);
	jvm_useJAR = Cvar_Get("jvm_useJAR", "0", CVAR_ARCHIVE | CVAR_LATCH);
	jvm_remoteDebugging = Cvar_Get("jvm_remoteDebugging", "0", CVAR_ARCHIVE | CVAR_LATCH);
	jvm_profiling = Cvar_Get("jvm_profiling", "0", CVAR_ARCHIVE | CVAR_LATCH);
	jvm_verboseJNI = Cvar_Get("jvm_verboseJNI", "0", CVAR_ARCHIVE | CVAR_LATCH);
	jvm_verboseClass = Cvar_Get("jvm_verboseClass", "0", CVAR_ARCHIVE | CVAR_LATCH);
	jvm_verboseGC = Cvar_Get("jvm_verboseGC", "0", CVAR_ARCHIVE | CVAR_LATCH);
	jvm_policyFile = Cvar_Get("jvm_policyFile", "", CVAR_ARCHIVE | CVAR_LATCH);

	option = AllocOption(&growOptions);

	if(!jvm_useJITCompiler->integer)
	{
		Com_Printf("Disabling Java JIT support\n");
		option->optionString = "-Djava.compiler=NONE";
	}
	else
	{
		option->optionString = "-Djava.compiler=YES";
	}

	// TODO support sv_pure
	if(jvm_useJAR->integer)
	{
		Com_sprintf(classPath, sizeof(classPath), "-Djava.class.path=%s",
					FS_BuildOSPath(Cvar_VariableString("fs_basepath"), Cvar_VariableString("fs_game"), "game.jar"));
	}
	else
	{
		Com_sprintf(classPath, sizeof(classPath), "-Djava.class.path=%s",
					FS_BuildOSPath(Cvar_VariableString("fs_basepath"), Cvar_VariableString("fs_game"), "classes"));
	}

	option = AllocOption(&growOptions);
	option->optionString = classPath;

	Com_Printf("Set main class path to '%s'\n", classPath);

	if(jvm_remoteDebugging->integer)
	{
		Com_Printf("Enabling remote debugging\n");

		option = AllocOption(&growOptions);
		option->optionString = "-Xdebug";

		option = AllocOption(&growOptions);
		option->optionString = "-Xrunjdwp:transport=dt_socket,server=y,address=8000,suspend=y";
	}

	if(jvm_verboseJNI->integer)
	{
		Com_Printf("Enabling verbose JNI prints\n");

		option = AllocOption(&growOptions);
		option->optionString = "-verbose:jni";
	}

	if(jvm_verboseClass->integer)
	{
		Com_Printf("Enabling displaying information about each class loaded.\n");

		option = AllocOption(&growOptions);
		option->optionString = "-verbose:class";
	}

	if(jvm_verboseGC->integer)
	{
		Com_Printf("Enabling reports on each garbage collection event.\n");

		option = AllocOption(&growOptions);
		option->optionString = "-verbose:gc";
	}

#if 1
	{
		Com_Printf("Enabling security manager\n");

		option = AllocOption(&growOptions);
		option->optionString = "-Djava.security.manager";
	}
#endif

	if(Q_stricmp(jvm_policyFile->string, "") != 0)
	{
		Com_sprintf(policyPath, sizeof(policyPath), "-Djava.security.policy=file:%s", FS_BuildOSPath(Cvar_VariableString("fs_basepath"), Cvar_VariableString("fs_game"), jvm_policyFile->string));

		option = AllocOption(&growOptions);
		option->optionString = policyPath;

		Com_Printf("Enabling '%s'\n", policyPath);
	}



	// convert growlist to array
	options = Com_Allocate(growOptions.currentElements * sizeof(JavaVMOption));
	for(i = 0; i < growOptions.currentElements; i++)
	{
		options[i] = *(JavaVMOption*) Com_GrowListElement(&growOptions, i);
	}

	vm_args.version = JNI_VERSION_1_4;
	vm_args.options = options;
	vm_args.nOptions = growOptions.currentElements;
	vm_args.ignoreUnrecognized = JNI_TRUE;

#if defined(USE_JAVA_DLOPEN)
	if(!JVM_JNI_Init())
	{
		Com_Error(ERR_FATAL, "JNI initialization failed");
	}
#endif

	Com_Printf("Searching for existing Java VM's ...");

	// look for an existing VM
#if defined(USE_JAVA_DLOPEN)
	if(QJNI_GetCreatedJavaVMs(&javaVM, 1, &nVMs) != JNI_OK)
#else
	if(JNI_GetCreatedJavaVMs(&javaVM, 1, &nVMs) != JNI_OK)
#endif
	{
		Com_Error(ERR_FATAL, "Search for existing VM's failed");
	}

	Com_Printf("found %i\n", nVMs);

#if 1
	if(nVMs > 0)
	{
		Com_Printf("Attaching to existing Java VM...");

		if((*javaVM)->AttachCurrentThread(javaVM, (void **)&javaEnv, NULL))
		{
			Com_Error(ERR_FATAL, "Couldn't attach to existing VM");
		}

		javaVMIsOurs = qfalse;
		//javaVM = jvm;

		Com_Printf("done\n");
	}
	else
#endif
	{
		int res;

		Com_Printf("Creating new Java VM...");

		// Create the Java VM
#if defined(USE_JAVA_DLOPEN)
		res = QJNI_CreateJavaVM(&javaVM, (void **)&javaEnv, &vm_args);
#else
		res = JNI_CreateJavaVM(&javaVM, (void **)&javaEnv, &vm_args);
#endif
		if(res != JNI_OK )
		{
			Com_Error(ERR_FATAL, "Can't create Java VM, JNI returned %i", res);
		}

		//javaVM = jvm;

		Com_Printf("done\n");
	}

	if(!javaVM)
	{
		Com_Error(ERR_FATAL, "JVM_Init failed");
	}

	// finally register the needed core modules
	Misc_javaRegister();
	Engine_javaRegister();
	CVar_javaRegister();
	UserCommand_javaRegister();


	// clean up allocated objects
	for(i = 0; i < growOptions.currentElements; i++)
	{
		option = Com_GrowListElement(&growOptions, i);
		Com_Dealloc(option);
	}
	Com_DestroyGrowList(&growOptions);

	Com_Dealloc(options);
}



#endif							//defined(USE_JAVA
