/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// vm.c -- virtual machine

/*


intermix code and data
symbol table

a dll has one imported function: VM_SystemCall
and one exported function: Perform


*/

#include "vm_local.h"

vm_t *currentVM = NULL;
vm_t *lastVM	= NULL;
int	  vm_debugLevel;

// used by Com_Error to get rid of running vm's before longjmp
static int forced_unload;

#define MAX_VM 3
vm_t vmTable[MAX_VM];

void VM_VmInfo_f(void);
void VM_VmProfile_f(void);

#if 0 // 64bit!
// converts a VM pointer to a C pointer and
// checks to make sure that the range is acceptable
void	*VM_VM2C( vmptr_t p, int length ) {
	return (void *)p;
}
#endif

void VM_Debug(int level) { vm_debugLevel = level; }

/*
==============
VM_Init // ICY CHANGED
==============
*/
void VM_Init(void) {
	Cvar_Get("vm_cgame", "2", CVAR_ARCHIVE); // !@# SHIP WITH SET TO 2
	Cvar_Get("vm_game", "2", CVAR_ARCHIVE);	 // !@# SHIP WITH SET TO 2
	Cvar_Get("vm_ui", "2", CVAR_ARCHIVE);	 // !@# SHIP WITH SET TO 2

	Cmd_AddCommand("vmprofile", VM_VmProfile_f);
	Cmd_AddCommand("vminfo", VM_VmInfo_f);

	memset(vmTable, 0, sizeof(vmTable));
}

/*
===============
VM_ValueToSymbol // ICY CHANGED

Assumes a program counter value
===============
*/
const char *VM_ValueToSymbol(vm_t *vm, int value) {
	if (!vm) { return "NULL VM"; }
	if (!vm->symbols) { return "NO SYMBOLS"; }

	vmSymbol_t *sym = vm->symbols;

	// find the symbol
	while (sym->next && sym->next->symValue <= value) { sym = sym->next; }

	static char text[MAX_TOKEN_CHARS];
	if (value == sym->symValue) {
		// symbol found
		return sym->symName;
	} else {
		// symbol not found
		snprintf(text, sizeof(text), "%s+%i", sym->symName, value - sym->symValue);
		return text;
	}
}

/*
===============
VM_ValueToFunctionSymbol // ICY CHANGED

For profiling, find the symbol behind this value
===============
*/
vmSymbol_t *VM_ValueToFunctionSymbol(vm_t *vm, int value) {
	if (!vm) { return NULL; }

	vmSymbol_t *sym = vm->symbols;
	if (!sym) { return NULL; }

	while (sym->next && sym->next->symValue <= value) { sym = sym->next; }

	return sym;
}

/*
===============
VM_SymbolToValue // ICY CHANGED
===============
*/
int VM_SymbolToValue(vm_t *vm, const char *symbol) {
	// Check for null pointer
	if (!vm || !symbol) { return 0; }

	// Loop through linked list of symbols
	for (vmSymbol_t *sym = vm->symbols; sym; sym = sym->next) {
		// Check for null pointer and compare symbol names
		if (sym->symName && strcmp(symbol, sym->symName) == 0) { return sym->symValue; }
	}

	// Symbol not found
	return 0;
}

/*
=====================
VM_SymbolForCompiledPointer // ICY CHANGED
=====================
*/
#if 0 // 64bit!
const char *VM_SymbolForCompiledPointer(vm_t *vm, void *code) {
    if (code < (void *)vm->codeBase) {
        return "Before code block";
    }
    if (code >= (void *)(vm->codeBase + vm->codeLength)) {
        return "After code block";
    }

    // find which original instruction it is after
    int i = (int)((uintptr_t)code - (uintptr_t)vm->codeBase) / sizeof(vm->instructionPointers[0]) - 1;
    if (i < 0 || i >= vm->codeLength) {
        return "Invalid bytecode instruction pointer";
    }

    // now look up the bytecode instruction pointer
    return VM_ValueToSymbol(vm, i);
}
#endif

/*
===============
ParseHex // ICY CHANGED
===============
*/
int32_t ParseHex(const char *text) {
	int32_t value = 0;
	char	c;

	while ((c = *text++) != '\0') {
		if (isxdigit(c)) {
			value = (value << 4) | (isdigit(c) ? c - '0' : (tolower(c) - 'a') + 10);
		} else {
			// Invalid character
			return -1;
		}
	}

	return value;
}

/*
===============
VM_LoadSymbols // ICY CHANGED
===============
*/
void VM_LoadSymbols(vm_t *vm) {
	char		 name[MAX_QPATH];
	char		 symbols[MAX_QPATH];
	vmSymbol_t **prev = &vm->symbols;
	vmSymbol_t	*sym  = NULL;
	int			 segment, numInstructions;
	int			 value, chars, count = 0;

	// don't load symbols if not developer
	if (!com_developer->integer) { return; }

	COM_StripExtension(vm->name, name, sizeof(name));
	Com_sprintf(symbols, sizeof(symbols), "vm/%s.map", name);

	void *mapfile = NULL;
	if (!FS_ReadFile(symbols, &mapfile)) {
		Com_Printf("Couldn't load symbol file: %s\n", symbols);
		return;
	}

	numInstructions = vm->instructionCount;

	// parse the symbols
	char *text_p = mapfile;
	while (*text_p) {
		segment = ParseHex(COM_Parse(&text_p));
		if (segment) {
			COM_Parse(&text_p);
			COM_Parse(&text_p);
			continue; // only load code segment values
		}

		value = ParseHex(COM_Parse(&text_p));
		chars = strlen(COM_Parse(&text_p));
		if (chars <= 0) {
			Com_Printf("WARNING: incomplete line at end of file\n");
			break;
		}

		sym		  = Hunk_Alloc(sizeof(vmSymbol_t) + chars, h_high);
		sym->next = NULL;
		*prev	  = sym;
		prev	  = &sym->next;

		// convert value from an instruction number to a code offset
		if (value >= 0 && value < numInstructions) { value = vm->instructionPointers[value]; }
		sym->symValue = value;

		Q_strncpyz(sym->symName, text_p, chars);
		sym->symName[chars] = '\0';

		count++;
		text_p += chars;
	}

	vm->numSymbols = count;
	Com_Printf("%i symbols parsed from %s\n", count, symbols);
	FS_FreeFile(mapfile);
}

/*
============
VM_DllSyscall

Dlls will call this directly

 rcg010206 The horror; the horror.

  The syscall mechanism relies on stack manipulation to get its args.
   This is likely due to C's inability to pass "..." parameters to
   a function in one clean chunk. On PowerPC Linux, these parameters
   are not necessarily passed on the stack, so while (&arg[0] == arg)
   is true, (&arg[1] == 2nd function parameter) is not necessarily
   accurate, as arg's value might have been stored to the stack or
   other piece of scratch memory to give it a valid address, but the
   next parameter might still be sitting in a register.

  Quake's syscall system also assumes that the stack grows downward,
   and that any needed types can be squeezed, safely, into a signed int.

  This hack below copies all needed values for an argument to a
   array in memory, so that Quake can get the correct values. This can
   also be used on systems where the stack grows upwards, as the
   presumably standard and safe stdargs.h macros are used.

  As for having enough space in a signed int for your datatypes, well,
   it might be better to wait for DOOM 3 before you start porting.  :)

  The original code, while probably still inherently dangerous, seems
   to work well enough for the platforms it already works on. Rather
   than add the performance hit for those platforms, the original code
   is still in use there.

  For speed, we just grab 15 arguments, and don't worry about exactly
   how many the syscall actually needs; the extra is thrown away.

============
*/
int32_t QDECL VM_DllSyscall(intptr_t arg, ...) { // ICY CHANGED
	intptr_t args[MAX_VMSYSCALL_ARGS];
	args[0] = arg;

	va_list ap;
	va_start(ap, arg);

	for (size_t i = 1; i < sizeof(args) / sizeof(args[0]); ++i) { args[i] = va_arg(ap, intptr_t); }

	va_end(ap);

	return currentVM->systemCall(args);
}

/*
=================
VM_LoadQVM

Load a .qvm file
=================
*/
vmHeader_t *VM_LoadQVM(vm_t *vm, qboolean alloc, qboolean unpure) {
	int32_t dataLength;
	int32_t i;
	char	filename[MAX_QPATH];
	union {
		vmHeader_t *h;
		void	   *v;
	} header;

	// load the image
	Com_sprintf(filename, sizeof(filename), "vm/%s.qvm", vm->name);
	Com_Printf("Loading vm file %s...\n", filename);

	FS_ReadFileDir(filename, vm->searchPath, unpure, &header.v);

	if (!header.h) {
		Com_Printf("Failed.\n");
		VM_Free(vm);

		Com_Printf(S_COLOR_YELLOW "Warning: Couldn't open VM file %s\n", filename);

		return NULL;
	}

	// show where the qvm was loaded from
	FS_Which(filename, vm->searchPath);

	if (LittleLong(header.h->vmMagic) == VM_MAGIC_VER2) {
		Com_Printf("...which has vmMagic VM_MAGIC_VER2\n");

		// byte swap the header
		for (i = 0; i < sizeof(vmHeader_t) / 4; i++) { ((int *)header.h)[i] = LittleLong(((int *)header.h)[i]); }

		// validate
		if (header.h->jtrgLength < 0 || header.h->bssLength < 0 || header.h->dataLength < 0 ||
			header.h->litLength < 0 || header.h->codeLength <= 0) {
			VM_Free(vm);
			FS_FreeFile(header.v);

			Com_Printf(S_COLOR_YELLOW "Warning: %s has bad header\n", filename);
			return NULL;
		}
	} else if (LittleLong(header.h->vmMagic) == VM_MAGIC) {
		// byte swap the header
		// sizeof( vmHeader_t ) - sizeof( int ) is the 1.32b vm header size
		for (i = 0; i < (sizeof(vmHeader_t) - sizeof(int)) / 4; i++) {
			((int *)header.h)[i] = LittleLong(((int *)header.h)[i]);
		}

		// validate
		if (header.h->bssLength < 0 || header.h->dataLength < 0 || header.h->litLength < 0 ||
			header.h->codeLength <= 0) {
			VM_Free(vm);
			FS_FreeFile(header.v);

			Com_Printf(S_COLOR_YELLOW "Warning: %s has bad header\n", filename);
			return NULL;
		}
	} else {
		VM_Free(vm);
		FS_FreeFile(header.v);

		Com_Printf(S_COLOR_YELLOW "Warning: %s does not have a recognisable "
								  "magic number in its header\n",
				   filename);
		return NULL;
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header.h->dataLength + header.h->litLength + header.h->bssLength;
	for (i = 0; dataLength > (1 << i); i++) {}
	dataLength = 1 << i;

	if (alloc) {
		// allocate zero filled space for initialized and uninitialized data
		// leave some space beyond data mask so we can secure all mask
		// operations
		vm->dataAlloc = dataLength + 4;
		vm->dataBase  = Hunk_Alloc(vm->dataAlloc, h_high);
		vm->dataMask  = dataLength - 1;
	} else {
		// clear the data, but make sure we're not clearing more than allocated
		if (vm->dataAlloc != dataLength + 4) {
			VM_Free(vm);
			FS_FreeFile(header.v);

			Com_Printf(S_COLOR_YELLOW "Warning: Data region size of %s not matching after "
									  "VM_Restart()\n",
					   filename);
			return NULL;
		}

		memset(vm->dataBase, 0, vm->dataAlloc);
	}

	// copy the intialized data
	memcpy(vm->dataBase, (byte *)header.h + header.h->dataOffset, header.h->dataLength + header.h->litLength);

	// byte swap the longs
	for (i = 0; i < header.h->dataLength; i += 4) {
		*(int *)(vm->dataBase + i) = LittleLong(*(int *)(vm->dataBase + i));
	}

	if (header.h->vmMagic == VM_MAGIC_VER2) {
		int previousNumJumpTableTargets = vm->numJumpTableTargets;

		header.h->jtrgLength &= ~0x03;

		vm->numJumpTableTargets = header.h->jtrgLength >> 2;
		Com_Printf("Loading %d jump table targets\n", vm->numJumpTableTargets);

		if (alloc) {
			vm->jumpTableTargets = Hunk_Alloc(header.h->jtrgLength, h_high);
		} else {
			if (vm->numJumpTableTargets != previousNumJumpTableTargets) {
				VM_Free(vm);
				FS_FreeFile(header.v);

				Com_Printf(S_COLOR_YELLOW "Warning: Jump table size of %s not matching after "
										  "VM_Restart()\n",
						   filename);
				return NULL;
			}

			memset(vm->jumpTableTargets, 0, header.h->jtrgLength);
		}

		memcpy(vm->jumpTableTargets,
				   (byte *)header.h + header.h->dataOffset + header.h->dataLength + header.h->litLength,
				   header.h->jtrgLength);

		// byte swap the longs
		for (i = 0; i < header.h->jtrgLength; i += 4) {
			*(int *)(vm->jumpTableTargets + i) = LittleLong(*(int *)(vm->jumpTableTargets + i));
		}
	}

	return header.h;
}

/*
=================
VM_Restart  // ICY CHANGED

Reload the data, but leave everything else in place
This allows a server to do a map_restart without changing memory allocation

We need to make sure that servers can access unpure QVMs (not contained in any
pak) even if the client is pure, so take "unpure" as argument.
=================
*/
vm_t *VM_Restart(vm_t *vm, qboolean unpure) {
	vmHeader_t *header;
	char		name[MAX_QPATH];
	intptr_t (*systemCall)(intptr_t * parms);

	if (vm->dllHandle) {
		systemCall = vm->systemCall;
		Q_strncpyz(name, vm->name, sizeof(name));
		VM_Free(vm);
		vm = VM_Create(name, systemCall, VMI_NATIVE);
		return vm;
	}

	Com_Printf("VM_Restart()\n");

	if (!(header = VM_LoadQVM(vm, qfalse, unpure))) {
		Com_Error(ERR_DROP, "VM_Restart failed");
		return NULL;
	}

	FS_FreeFile(header);

	return vm;
}

/*
================
VM_Create

If image ends in .qvm it will be interpreted, otherwise
it will attempt to load as a system dll
================
*/
vm_t *VM_Create(const char *module, intptr_t (*systemCalls)(intptr_t *), vmInterpret_t interpret) {
	vm_t	   *vm;
	vmHeader_t *header;
	int			i, remaining, retval;
	char		filename[MAX_OSPATH];
	void	   *startSearch = NULL;

	if (!module || !module[0] || !systemCalls) { Com_Error(ERR_FATAL, "VM_Create: bad parms"); }

	remaining = Hunk_MemoryRemaining();

	// see if we already have the VM
	for (i = 0; i < MAX_VM; i++) {
		if (!Q_stricmp(vmTable[i].name, module)) {
			vm = &vmTable[i];
			return vm;
		}
	}

	// find a free vm
	for (i = 0; i < MAX_VM; i++) {
		if (!vmTable[i].name[0]) { break; }
	}

	if (i == MAX_VM) { Com_Error(ERR_FATAL, "VM_Create: no free vm_t"); }

	vm = &vmTable[i];

	Q_strncpyz(vm->name, module, sizeof(vm->name));

	do {
		retval = FS_FindVM(&startSearch, filename, sizeof(filename), module, (interpret == VMI_NATIVE));

		if (retval == VMI_NATIVE) {
			Com_Printf("Try loading dll file %s\n", filename);

			vm->dllHandle = Sys_LoadGameDll(filename, &vm->entryPoint, VM_DllSyscall);

			if (vm->dllHandle) {
				vm->systemCall = systemCalls;
				return vm;
			}

			Com_Printf("Failed loading dll, trying next\n");
		} else if (retval == VMI_COMPILED) {
			vm->searchPath = startSearch;
			if ((header = VM_LoadQVM(vm, qtrue, qfalse))) break;

			// VM_Free overwrites the name on failed load
			Q_strncpyz(vm->name, module, sizeof(vm->name));
		}
	} while (retval >= 0);

	if (retval < 0) return NULL;

	vm->systemCall = systemCalls;

	// allocate space for the jump targets, which will be filled in by the
	// compile/prep functions
	vm->instructionCount	= header->instructionCount;
	vm->instructionPointers = Hunk_Alloc(vm->instructionCount * sizeof(*vm->instructionPointers), h_high);

	// copy or compile the instructions
	vm->codeLength = header->codeLength;

	vm->compiled = qfalse;

#ifdef NO_VM_COMPILED
	if (interpret >= VMI_COMPILED) {
		Com_Printf("Architecture doesn't have a bytecode compiler, using "
				   "interpreter\n");
		interpret = VMI_BYTECODE;
	}
#else
	if (interpret != VMI_BYTECODE) {
		vm->compiled = qtrue;
		VM_Compile(vm, header);
	}
#endif
	// VM_Compile may have reset vm->compiled if compilation failed
	if (!vm->compiled) { VM_PrepareInterpreter(vm, header); }

	// free the original file
	FS_FreeFile(header);

	// load the map file
	VM_LoadSymbols(vm);

	// the stack is implicitly at the end of the image
	vm->programStack = vm->dataMask + 1;
	vm->stackBottom	 = vm->programStack - PROGRAM_STACK_SIZE;

	Com_Printf("%s loaded in %d bytes on the hunk\n", module, remaining - Hunk_MemoryRemaining());

	return vm;
}

/*
==============
VM_Free
==============
*/
void VM_Free(vm_t *vm) {

	if (!vm) { return; }

	if (vm->callLevel) {
		if (!forced_unload) {
			Com_Error(ERR_FATAL, "VM_Free(%s) on running vm", vm->name);
			return;
		} else {
			Com_Printf("forcefully unloading %s vm\n", vm->name);
		}
	}

	if (vm->destroy) vm->destroy(vm);

	if (vm->dllHandle) {
		Sys_UnloadDll(vm->dllHandle);
		memset(vm, 0, sizeof(*vm));
	}
#if 0 // now automatically freed by hunk
	if ( vm->codeBase ) {
		Z_Free( vm->codeBase );
	}
	if ( vm->dataBase ) {
		Z_Free( vm->dataBase );
	}
	if ( vm->instructionPointers ) {
		Z_Free( vm->instructionPointers );
	}
#endif
	memset(vm, 0, sizeof(*vm));

	currentVM = NULL;
	lastVM	  = NULL;
}

void VM_Clear(void) {
	int i;
	for (i = 0; i < MAX_VM; i++) { VM_Free(&vmTable[i]); }
}

void VM_Forced_Unload_Start(void) { forced_unload = 1; }

void VM_Forced_Unload_Done(void) { forced_unload = 0; }

void *VM_ArgPtr(intptr_t intValue) {
	if (!intValue) { return NULL; }
	// currentVM is missing on reconnect
	if (currentVM == NULL) return NULL;

	if (currentVM->entryPoint) {
		return (void *)(currentVM->dataBase + intValue);
	} else {
		return (void *)(currentVM->dataBase + (intValue & currentVM->dataMask));
	}
}

void *VM_ExplicitArgPtr(vm_t *vm, intptr_t intValue) {
	if (!intValue) { return NULL; }

	// currentVM is missing on reconnect here as well?
	if (currentVM == NULL) return NULL;

	//
	if (vm->entryPoint) {
		return (void *)(vm->dataBase + intValue);
	} else {
		return (void *)(vm->dataBase + (intValue & vm->dataMask));
	}
}

/*
==============
VM_Call


Upon a system call, the stack will look like:

sp+32	parm1
sp+28	parm0
sp+24	return value
sp+20	return address
sp+16	local1
sp+14	local0
sp+12	arg1
sp+8	arg0
sp+4	return stack
sp		return address

An interpreted function will immediately execute
an OP_ENTER instruction, which will subtract space for
locals from sp
==============
*/

intptr_t QDECL VM_Call(vm_t *vm, int callnum, ...) {
	vm_t	*oldVM;
	intptr_t r;
	int		 i;

	if (callnum < 0) { Com_Error(ERR_FATAL, "VM_Call with negative callnum"); }

	if (!vm || !vm->name[0]) Com_Error(ERR_FATAL, "VM_Call with NULL vm");

	oldVM	  = currentVM;
	currentVM = vm;
	lastVM	  = vm;

	if (vm_debugLevel) { Com_Printf("VM_Call( %d )\n", callnum); }

	++vm->callLevel;
	// if we have a dll loaded, call it directly
	if (vm->entryPoint) {
		// rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
		int		args[MAX_VMMAIN_ARGS - 1];
		va_list ap;
		va_start(ap, callnum);
		for (i = 0; i < ARRAY_LEN(args); i++) { args[i] = va_arg(ap, int); }
		va_end(ap);

		r = vm->entryPoint(callnum, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8],
						   args[9], args[10], args[11]);
	} else {
#if (id386 || idsparc) && !defined __clang__ // calling convention doesn't need conversion in some cases
#ifndef NO_VM_COMPILED
		if (vm->compiled)
			r = VM_CallCompiled(vm, (int *)&callnum);
		else
#endif
			r = VM_CallInterpreted(vm, (int *)&callnum);
#else
		struct {
			int callnum;
			int args[MAX_VMMAIN_ARGS - 1];
		} a;
		va_list ap;

		a.callnum = callnum;
		va_start(ap, callnum);
		for (i = 0; i < ARRAY_LEN(a.args); i++) { a.args[i] = __builtin_va_arg(ap, int); }
		va_end(ap);
#ifndef NO_VM_COMPILED
		if (vm->compiled)
			r = VM_CallCompiled(vm, &a.callnum);
		else
#endif
			r = VM_CallInterpreted(vm, &a.callnum);
#endif
	}
	--vm->callLevel;

	if (oldVM != NULL) currentVM = oldVM;
	return r;
}

//=================================================================

static int QDECL VM_ProfileSort(const void *a, const void *b) {
	vmSymbol_t *sa, *sb;

	sa = *(vmSymbol_t **)a;
	sb = *(vmSymbol_t **)b;

	if (sa->profileCount < sb->profileCount) { return -1; }
	if (sa->profileCount > sb->profileCount) { return 1; }
	return 0;
}

/*
==============
VM_VmProfile_f

==============
*/
void VM_VmProfile_f(void) {
	vm_t		*vm;
	vmSymbol_t **sorted, *sym;
	int			 i;
	double		 total;

	if (!lastVM) { return; }

	vm = lastVM;

	if (!vm->numSymbols) { return; }

	sorted	  = Z_Malloc(vm->numSymbols * sizeof(*sorted));
	sorted[0] = vm->symbols;
	total	  = sorted[0]->profileCount;
	for (i = 1; i < vm->numSymbols; i++) {
		sorted[i] = sorted[i - 1]->next;
		total += sorted[i]->profileCount;
	}

	qsort(sorted, vm->numSymbols, sizeof(*sorted), VM_ProfileSort);

	for (i = 0; i < vm->numSymbols; i++) {
		int perc;

		sym = sorted[i];

		perc = 100 * (float)sym->profileCount / total;
		Com_Printf("%2i%% %9i %s\n", perc, sym->profileCount, sym->symName);
		sym->profileCount = 0;
	}

	Com_Printf("    %9.0f total\n", total);

	Z_Free(sorted);
}

/*
==============
VM_VmInfo_f

==============
*/
void VM_VmInfo_f(void) {
	vm_t *vm;
	int	  i;

	Com_Printf("Registered virtual machines:\n");
	for (i = 0; i < MAX_VM; i++) {
		vm = &vmTable[i];
		if (!vm->name[0]) { break; }
		Com_Printf("%s : ", vm->name);
		if (vm->dllHandle) {
			Com_Printf("native\n");
			continue;
		}
		if (vm->compiled) {
			Com_Printf("compiled on load\n");
		} else {
			Com_Printf("interpreted\n");
		}
		Com_Printf("    code length : %7i\n", vm->codeLength);
		Com_Printf("    table length: %7i\n", vm->instructionCount * 4);
		Com_Printf("    data length : %7i\n", vm->dataMask + 1);
	}
}

/*
===============
VM_LogSyscalls

Insert calls to this while debugging the vm compiler
===============
*/
void VM_LogSyscalls(int *args) {
	static int	 callnum;
	static FILE *f;

	if (!f) { f = fopen("syscalls.log", "w"); }
	callnum++;
	fprintf(f, "%i: %p (%i) = %i %i %i %i\n", callnum, (void *)(args - (int *)currentVM->dataBase), args[0], args[1],
			args[2], args[3], args[4]);
}

/*
=================
VM_BlockCopy
Executes a block copy operation within currentVM data space
=================
*/

void VM_BlockCopy(uint32_t dest, uint32_t src, size_t n) {
	uint32_t dataMask = currentVM->dataMask;

	if ((dest & dataMask) != dest || (src & dataMask) != src || ((dest + n) & dataMask) != dest + n ||
		((src + n) & dataMask) != src + n) {
		Com_Error(ERR_DROP, "OP_BLOCK_COPY out of range!");
	}

	memcpy(currentVM->dataBase + dest, currentVM->dataBase + src, n);
}
