Sym-PPC = -sym Full
Sym-68K = -sym Full

COptions = -proto strict
COptions-PPC = {COptions} {Sym-PPC}
COptions-68K = {COptions} {Sym-68K}

### Source Files ###

SrcFiles = ∂
	sync.c

### Object Files ###

ObjFiles-PPC = ∂
	sync.c.x

ObjFiles-68K = ∂
	sync.c.o

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
		-append {MAKEFILE} ∂
		-ignore "{CIncludes}" ∂
		-objext .x ∂
		-objext .o ∂
		{SrcFiles}
