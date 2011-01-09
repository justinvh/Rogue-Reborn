/*
Rogue Reborn "System Call Setup" source from src/sv_physics.cpp

Copyright 2011 Justin Bruce Van Horne.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <hat/engine/physics.h>
#include <hat/physics/public.h>
#include <hat/engine/server.h>

vm_t* pvm;

/*
====================
PHYS_InitVM

Called for both a full init and a restart
====================
*/
void PHYS_InitVM(qboolean restart)
{
  VM_Call(pvm, PHYSICS_INIT, sv.time, Com_Milliseconds(), restart);
}

/*
====================
PHYS_SystemCall

The module is making a system call
====================
*/
intptr_t PHYS_SystemCalls(intptr_t *args)
{
  return 0;
}

/*
===============
PHYS_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void PHYS_InitGameProgs(void)
{
#if defined(USE_LLVM)
  pvm = VM_Create("physics", PHYS_SystemCalls, Cvar_VariableValue("vm_physics"));
#else
  pvm = VM_Create("physics", PHYS_SystemCalls, VMI_NATIVE);
#endif

  if (!pvm)
    Com_Error(ERR_FATAL, "VM_Create on physics failed");

  PHYS_InitVM(qfalse);
}

