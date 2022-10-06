#ifndef PLATFORM_H
#define PLATFORM_H

#include "common.h"

#define FU64 "%llu"
#define FS64 "%lld"
#define FSTR "%.*s"

namespace file_type {
enum file_type {
    Directory,
    File,
    Invalid
};
}

struct file {
    str Name;
    usize Size;
    enum file_type::file_type Type;
};

void Exit(int ExitCode);
// void * Malloc(usize Size);
#define Malloc(Size) Malloc_(Size, (char*)__FUNCTION__)
void *Malloc_(usize Size, char *CallerName);
void Free(void * Memory);
file_type::file_type FileType(str *Path);
void Delete(str *Path);
str GetCwd();

#if (__APPLE__ && __MACH__ && __x86_64__) // ---------------------------------------------
    // MacOS x64.
    #define MACINTOSH_X64 1
    int RunCommandLineProgram(array<str> Command);

#elif (_WIN64) // ------------------------------------------------------------------------
    // Windows x64.
    #define WIN_X64 1

    struct strw {
        wchar_t *Wchars;
        usize Size;
    };

    str WideToUTF8(strw *Wide);
    str WideToUTF8(wchar_t *Wide);
    wchar_t *UTFToWide(wchar_t *Dest, usize DestSize, str *Str);
    strw GetCwdW();
    void TerminalInit();
    void TerminalCleanup();
#else // ---------------------------------------------------------------------------------
    #error "Unsupported platform."
#endif // --------------------------------------------------------------------------------

array<file> ReadDirectory(str &Directory, bool GetFileSizes = false);
array<file> ReadDirectory(str *Directory, bool GetFileSizes = false);

// ---------------------------------------------------------------------------------------

int RunCommandLineProgram(str *Command);
void Copy(void * Dst, const void * RESTRICT Src, usize Size);
PRINTFLIKE(1,2) int Printf(const char *Format, ...);

#endif