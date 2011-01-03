#ifndef HAT_ENGINE_Q_PRINT_H
#define HAT_ENGINE_Q_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hat/engine/q_platform.h>

#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

// this is only here so the functions in q_shared.c and bg_*.c can link
void QDECL      Com_Error(int level, const char *error, ...) __attribute__ ((format(printf, 2, 3)));
void QDECL      Com_Printf(const char *msg, ...) __attribute__ ((format(printf, 1, 2)));
void QDECL      Com_Warning(const char *msg, ...) __attribute__ ((format(printf, 1, 2)));

#ifdef __cplusplus
}
#endif

#endif
