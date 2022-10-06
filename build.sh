#!/bin/sh

mkdir -p bin   > /dev/null 2> /dev/null
rm -rf ./bin/* > /dev/null 2> /dev/null

which clang++ > /dev/null 2> /dev/null
result=$?

if [ $result -ne 0 ]; then
    echo "ERROR: clang++ is not in the path - please install LLVM"
    exit 1
fi

# ------------------------------------------------------------------------------
#### Uncomment one options:

#### Option A: Single traslation unit.
# src="-DSINGLE_TLU ./src/main.c "

#### Option B: Multiple transtation units.
src="./src/main.cpp " # ./src/platform.c ./src/common.c "

# ------------------------------------------------------------------------------

#   CMOV, CMPXCHG8B, FPU, FXSR, MMX, FXSR, SCE, SSE, SSE2
# commands+="-march=x86-64 "    (close to Nehalem) CMPXCHG16B, LAHF-SAHF, POPCNT, SSE3, SSE4.1, SSE4.2, SSSE3
# commands+="-march=x86-64-v2 " (close to Haswell) AVX, AVX2, BMI1, BMI2, F16C, FMA, LZCNT, MOVBE, XSAVE
# commands+="-march=x86-64-v3 " AVX512F, AVX512BW, AVX512CD, AVX512DQ, AVX512VL
# commands+="-march=x86-64-v4 "
common="-x c++ -std=c++11 $src -o bin/main -DTERMCOLOR=1 -mno-incremental-linker-compatible -Wno-writable-strings -Wno-tautological-compare -Wno-unused-value -fwritable-strings -MJ compile_commands.json_ "
# common+="-march=x86-64-v2 -mavx2 -ffast-math "
# common+="-march=x86-64-v2 "

# ------------------------------------------------------------------------------
#### Uncomment one option:

#### Option A: Unoptimized build with debug symbols.
# clang++ -O0 -g -DASSERT_LEVEL=2 -DDEBUG -D_DEBUG -DDEBUG_PRINT -DDEBUG_ALLOCATIONS $common

#### Option B: Optimized.
clang++ -g -O2 -DNDEBUG -DASSERT_LEVEL=1 -DDEBUG_PRINT $common

#### (Example) Address sanitizer.
# clang++ -O0 -g $common fsanitize=address -fno-omit-frame-pointer

#### (Example) Dynamic linking.
# clang++ -O2 $common Wl,-nodefaultlib:libcmt -D_DLL -lmsvcrt

result=$?

# ------------------------------------------------------------------------------

echo [ > compile_commands.json
cat compile_commands.json_ >> compile_commands.json
echo ] >> compile_commands.json
rm compile_commands.json_

# ------------------------------------------------------------------------------

exit $result
