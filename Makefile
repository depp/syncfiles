Sym-PPC = -sym Full
Sym-68K = -sym Full

COptions = -proto strict
COptions-PPC = {COptions} {Sym-PPC}
COptions-68K = {COptions} {Sym-68K}

### Source Files ###

SrcFiles = ∂
	convert.c ∂
	copy.c ∂
	file.c ∂
	mac_from_unix.c ∂
	mac_to_unix.c ∂
	sync.c ∂
	util.c

### Object Files ###

ObjFiles-PPC = ∂
	convert.c.x ∂
	copy.c.x ∂
	file.c.x ∂
	mac_from_unix.c.x ∂
	mac_to_unix.c.x ∂
	sync.c.x ∂
	util.c.x

ObjFiles-68K = ∂
	convert.c.o ∂
	copy.c.o ∂
	file.c.o ∂
	mac_from_unix.c.o ∂
	mac_to_unix.c.o ∂
	sync.c.o ∂
	util.c.o

### Libraries ###

LibFiles-PPC = ∂
	"{SharedLibraries}InterfaceLib" ∂
	"{SharedLibraries}StdCLib" ∂
	"{PPCLibraries}StdCRuntime.o" ∂
	"{PPCLibraries}PPCCRuntime.o" ∂
	"{PPCLibraries}PPCToolLibs.o"

LibFiles-68K = ∂
	"{Libraries}Stubs.o" ∂
	"{CLibraries}StdCLib.o" ∂
	"{Libraries}MacRuntime.o" ∂
	"{Libraries}IntEnv.o" ∂
	"{Libraries}ToolLibs.o" ∂
	"{Libraries}Interface.o"

### Default Rules ###

.c.x ƒ .c
	{PPCC} {depDir}{default}.c -o {targDir}{default}.c.x {COptions-PPC}

.c.o ƒ .c
	{C} {depDir}{default}.c -o {targDir}{default}.c.o {COptions-68K}

### Build Rules ###

SyncFiles ƒƒ {ObjFiles-PPC} {LibFiles-PPC}
	PPCLink ∂
		-o {Targ} ∂
		{ObjFiles-PPC} ∂
		{LibFiles-PPC} ∂
		{Sym-PPC} ∂
		-mf -d ∂
		-t 'MPST' ∂
		-c 'MPS '

SyncFiles ƒƒ {ObjFiles-68K} {LibFiles-68K}
	Link ∂
		-o {Targ} ∂
		{ObjFiles-68K} ∂
		{LibFiles-68K} ∂
		{Sym-68K} ∂
		-mf -d ∂
		-t 'MPST' ∂
		-c 'MPS '

### Build this target to generate "include file" dependencies. ###

Dependencies  ƒ  $OutOfDate
	MakeDepend ∂
		-append Makefile ∂
		-skip "{CIncludes}" ∂
		-objext .x ∂
		-objext .o ∂
		{SrcFiles}

#*** Dependencies: Cut here ***
# These dependencies were produced at 4:16:36 AM on Wed, Mar 24, 2021 by MakeDepend

:convert.c.x :convert.c.o	ƒ  ∂
	:convert.c ∂
	:convert.h ∂
	:defs.h ∂
	:mac_from_unix_data.h

:copy.c.x :copy.c.o	ƒ  ∂
	:copy.c ∂
	:convert.h

:file.c.x :file.c.o	ƒ  ∂
	:file.c ∂
	:defs.h ∂
	:convert.h

:mac_from_unix.c.x :mac_from_unix.c.o	ƒ  ∂
	:mac_from_unix.c ∂
	:convert.h

:mac_to_unix.c.x :mac_to_unix.c.o	ƒ  ∂
	:mac_to_unix.c ∂
	:convert.h

:sync.c.x :sync.c.o	ƒ  ∂
	:sync.c ∂
	:defs.h

:util.c.x :util.c.o	ƒ  ∂
	:util.c ∂
	:defs.h

