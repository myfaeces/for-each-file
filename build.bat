@echo off
setlocal

del /q/f bin\*
FOR /D %%p IN ("bin\*") DO rmdir "%%p" /s/q

where clang++ > nul 2>nul
IF %ERRORLEVEL% NEQ 0 (echo ERROR: clang++ is not in the path - please install LLVM) && exit /b -1

@REM ------------------------------------------------------------------------------

@REM #### Uncomment one options:

@REM #### Option A: Single traslation unit.
@REM set src="-DSINGLE_TLU ./src/main.c "

@REM #### Option B: Multiple transtation units.
set src=src\main.cpp

@REM ------------------------------------------------------------------------------

@REM   CMOV, CMPXCHG8B, FPU, FXSR, MMX, FXSR, SCE, SSE, SSE2
@REM commands+="-march=x86-64 "    (close to Nehalem) CMPXCHG16B, LAHF-SAHF, POPCNT, SSE3, SSE4.1, SSE4.2, SSSE3
@REM commands+="-march=x86-64-v2 " (close to Haswell) AVX, AVX2, BMI1, BMI2, F16C, FMA, LZCNT, MOVBE, XSAVE
@REM commands+="-march=x86-64-v3 " AVX512F, AVX512BW, AVX512CD, AVX512DQ, AVX512VL
@REM commands+="-march=x86-64-v4 "
set common=-x c++ -std=c++11 %src% -o bin/main.exe -DTERMCOLOR=1 -mno-incremental-linker-compatible -Wno-writable-strings -Wno-tautological-compare -Wno-unused-value -fwritable-strings -MJ compile_commands.json_
@REM common+="-march=x86-64-v2 -mavx2 -ffast-math "
@REM common+="-march=x86-64-v2 "

@REM ------------------------------------------------------------------------------
@REM #### Uncomment one option:


@REM #### Option A: Unoptimized build with debug symbols.
@REM clang++ -O0 -g -DASSERT_LEVEL=2 -DDEBUG -D_DEBUG -DDEBUG_PRINT -DDEBUG_ALLOCATIONS %common%

@REM #### Option B: Optimized.
clang++ -g -O2 -DNDEBUG -DASSERT_LEVEL=1 -DDEBUG_PRINT %common%

@REM #### (Example) Address sanitizer.
@REM clang++ -O0 -g %common% fsanitize=address -fno-omit-frame-pointer

@REM #### (Example) Dynamic linking.
@REM clang++ -O2 %common% Wl,-nodefaultlib:libcmt -D_DLL -lmsvcrt


set result=%ERRORLEVEL%

@REM ------------------------------------------------------------------------------

@echo [ > compile_commands.json
@type compile_commands.json_ >> compile_commands.json
@echo ] >> compile_commands.json
@del compile_commands.json_

@REM ------------------------------------------------------------------------------

exit /B %result%
