#include "platform.h"

#include <assert.h>
#include <corecrt_wstring.h>
#include <stdio.h>
#include <stdlib.h>
#include "Windows.h"

PRINTFLIKE(1,2) int Printf(const char *Format, ...) {
    __builtin_va_list Args;
    __builtin_va_start(Args, Format);
    auto Result = vprintf(Format, Args);
    __builtin_va_end(Args);
    return Result;
}

void Copy(void *Dest, const void * RESTRICT Src, usize Size) {
    memcpy(Dest, Src, Size);
}

str LastError() {
    wchar_t *Error = NULL;

    assert2(FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&Error,
        0,
        NULL
    ));

    // Windows "helpfully" terminates it's error messages with "/r/n", remove it.
    usize ErrorCount = wcslen(Error);
    while (Error[ErrorCount-1] == L'\r' || Error[ErrorCount-1] == L'\n') {
        Error[ErrorCount-- - 1] = L'\0';
    }

    str Result = str(WideToUTF8(Error));

    return Result;
}

str WideToUTF8(strw *Wide) {
    str Result = {};
    Result.Size = WideCharToMultiByte(CP_UTF8, 0, Wide->Wchars, Wide->Size / sizeof(wchar_t), NULL, 0, NULL, NULL);
    Result.Chars = (char*)Malloc(Result.Size+1);
    WideCharToMultiByte(CP_UTF8, 0, Wide->Wchars, Wide->Size / sizeof(wchar_t), Result.Chars, Result.Size, NULL, NULL);
    Result.Chars[Result.Size] = '\0';
    return Result;
}

str WideToUTF8(wchar_t *Wide) {
    strw WideString = {};
    WideString.Wchars = Wide;
    WideString.Size   = (uint)wcslen(Wide) * sizeof(wchar_t);

    str Result = WideToUTF8(&WideString);
    return Result;
}

wchar_t *UTF8ToWide(wchar_t *Dest, usize DestSize, str *Str) {
    if (Str->Size == 0) return Dest;

    auto WcharsAvailCount = DestSize / sizeof(wchar_t);
    auto WcharsWrittenCount = MultiByteToWideChar(CP_UTF8, 0, Str->Chars, Str->Size, Dest, WcharsAvailCount); // @CHECK: Does this zero-terminate?
    assert0(WcharsWrittenCount && (WcharsAvailCount > WcharsWrittenCount));
    return &Dest[WcharsWrittenCount];
}

strw UTF8ToWide(str *String) {
    strw Result;

    usize RequiredCount = MultiByteToWideChar(CP_UTF8, 0, String->Chars, String->Size, NULL, 0);
    Result.Size   = RequiredCount * sizeof(wchar_t);
    Result.Wchars = MallocCount<wchar_t>(RequiredCount + 1);

    MultiByteToWideChar(CP_UTF8, 0, String->Chars, String->Size, Result.Wchars, RequiredCount);
    Result.Wchars[RequiredCount] = L'\0';

    return Result;
}

wchar_t * StringSplitByChar(strw *Before, strw *After, strw *String, wchar_t Char) {
    Before->Wchars = String->Wchars;
    uint StringCount = String->Size / sizeof(wchar_t);
    for (uint I = 0; I < StringCount; ++I) {
        if (String->Wchars[I] == Char) {
            Before->Size = I;
            After->Size  = (StringCount - I - 1)*sizeof(wchar_t);
            After->Wchars = &String->Wchars[I+1];
            #ifdef DEBUG
                if (Before->Size == 0) Before->Wchars = NULL;
                if (After->Size  == 0) After->Wchars = NULL;
            #endif
            return &String->Wchars[I];
        }
    }
    Before->Size = String->Size;
    After->Size = 0;
    #ifdef DEBUG
        if (Before->Size == 0) Before->Wchars = NULL;
        if (After->Size  == 0) After->Wchars = NULL;
    #endif
    return NULL;
}

strw GetCwdW() {
    strw Result;

    auto CwdCount = GetCurrentDirectoryW(0, NULL);
    wchar_t *Cwd = (wchar_t *)LocalAlloc(LPTR, CwdCount*sizeof(wchar_t));
    GetCurrentDirectoryW(CwdCount, Cwd);
    assert(CwdCount > 0);
    CwdCount -= 1;

    if (Cwd[CwdCount-1] != L'\\') {
        wchar_t *NormalizedCwd = (wchar_t *)malloc((CwdCount+1)*sizeof(wchar_t));
        memcpy(NormalizedCwd, Cwd, CwdCount*sizeof(wchar_t));
        NormalizedCwd[CwdCount++] = L'\\';
        NormalizedCwd[CwdCount] = L'\0';
        LocalFree(Cwd);
        Cwd = NormalizedCwd;
    }

    Result.Size = CwdCount * sizeof(wchar_t);
    Result.Wchars = Cwd;

    return Result;
}

str GetCwd() {
    auto CwdW = GetCwdW();
    auto Result = WideToUTF8(CwdW.Wchars);
    return Result;
}

void Exit(int ExitCode) {
    ExitProcess(ExitCode);
}

#define GUARD_PAGE 0
#define GUARD_PAGE_BEFORE 0

struct ALIGN(1) memory_header {
    usize UserMemorySize;
    usize MemReserveSize;
    char *CallerFuntion;
    void *MemReserve;
};

void * Malloc_(usize Size, char *CallerName) {
#if GUARD_PAGE && GUARD_PAGE_BEFORE
    // @UNIMLEMENTED.
#elif GUARD_PAGE
    auto MemReserveSize = __builtin_align_up(Size + sizeof(memory_header), 4096) + 4096;

    // int Alignment = 0;
    // Alignment = (Size%64==0) ? 64 :
    //             (Size%32==0) ? 32 :
    //             (Size%16==0) ? 16 :
    //             (Size% 8==0) ?  8 :
    //             (Size% 4==0) ?  4 :
    //             (Size% 2==0) ?  2 :
    //                             1;

    auto ReserveAddr = (char*)VirtualAlloc(0, MemReserveSize, MEM_RESERVE, PAGE_READWRITE);
    assert0(ReserveAddr != NULL);
    auto GuardPage   = (char*)VirtualAlloc(ReserveAddr + 4096, Size, MEM_COMMIT, PAGE_NOACCESS);
    assert0(GuardPage == ReserveAddr + 4096);
    auto CommitAddr  = (char*)VirtualAlloc(ReserveAddr, Size, MEM_COMMIT, PAGE_READWRITE);
    assert0(CommitAddr == ReserveAddr);

    auto UserMemory = GuardPage - Size;
    auto Allocation = (memory_header *)(UserMemory - sizeof(memory_header));
    Allocation->UserMemorySize = Size;
    Allocation->MemReserveSize = MemReserveSize;
    Allocation->CallerFuntion  = CallerName;
    Allocation->MemReserve     = ReserveAddr;

    fprintf(stderr, "\033[0m ---------- \r\n");
    fflush(stderr);
    printf("MEMORY: 0x%llx .. 0x%llx (%lld)\r\n", (off)UserMemory, (off)GuardPage, Size);
    fflush(stderr);

    return UserMemory;
#else
    auto UserMemory = LocalAlloc(LPTR, Size);
    return UserMemory;
#endif
}

void Free(void * Memory) {
#if GUARD_PAGE && GUARD_PAGE_BEFORE
    auto Allocation = (memory_header*)((char*)Memory - sizeof(memory_header));
    usize Size = Allocation->MemReserveSize;
    usize TotalSize = Size + 4096;
    // int ok = VirtualFree(Allocation, Size, MEM_DECOMMIT);
    // ok = VirtualFree(Allocation, TotalSize, MEM_RELEASE);

    auto ok = VirtualAlloc(Allocation, Size, MEM_COMMIT, PAGE_NOACCESS);
    if (0 == ok) {
        auto Error = LastError();
        printf("%ls\n", Error);
    }

    assert0(ok);
#elif GUARD_PAGE
    auto Allocation = (memory_header *)((char*)Memory - sizeof(memory_header));
    auto ok = VirtualFree(Allocation->MemReserve, Allocation->MemReserveSize, MEM_DECOMMIT);
#else
    LocalFree(Memory);
#endif
}

struct terminal_state {
    HANDLE std_out_handle;
    CONSOLE_FONT_INFOEX font;
    DWORD codepage;
    DWORD mode;
};
static terminal_state TerminalState;

void TerminalInit() {
    TerminalState.std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    TerminalState.codepage       = GetConsoleOutputCP();
    GetConsoleMode(TerminalState.std_out_handle, &TerminalState.mode);

    OSVERSIONINFOEXA ver = { .dwBuildNumber = 8000 };
    bool is_win7 = VerifyVersionInfo(&ver, VER_BUILDNUMBER, VerSetConditionMask(0, VER_BUILDNUMBER, VER_LESS));
    if (is_win7) {
        TerminalState.font.cbSize = sizeof(TerminalState.font);
        GetCurrentConsoleFontEx(TerminalState.std_out_handle, FALSE, &TerminalState.font);

        CONSOLE_FONT_INFOEX new_font = TerminalState.font;
        static const wchar_t consolas[] = L"Consolas";
        memcpy(new_font.FaceName, (void*)consolas, sizeof(consolas));
        SetCurrentConsoleFontEx(TerminalState.std_out_handle, FALSE, &new_font);
    } else {
        TerminalState.font.cbSize = 0;
    }

    auto mode = TerminalState.mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(TerminalState.std_out_handle, mode);
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
}

void TerminalCleanup() {
    SetConsoleMode(TerminalState.std_out_handle, TerminalState.mode);
    if (TerminalState.font.cbSize != 0)
        SetCurrentConsoleFontEx(TerminalState.std_out_handle, FALSE, &TerminalState.font);
    SetConsoleCP(TerminalState.codepage);
    SetConsoleOutputCP(TerminalState.codepage);
}

HANDLE
NextFile(WIN32_FIND_DATAW *FileInfo, HANDLE FindHandle, wchar_t *FindPattern, bool SkipDots) {
    assert(FileInfo != NULL);

    bool Found = false;

    if (FindHandle == NULL) {
        FindHandle = FindFirstFileW(FindPattern, FileInfo);
        if (FindHandle == INVALID_HANDLE_VALUE) {
            return NULL; // Cannot start a find thingy.
        }

        if (SkipDots) {
            FindNextFileW(FindHandle, FileInfo);         // skip .
            Found = FindNextFileW(FindHandle, FileInfo); // skip ..
        }
    } else {
        Found = FindNextFileW(FindHandle, FileInfo);
    }

    if (Found) {
        return FindHandle;
    } else {
        FindClose(FindHandle);
        return NULL;
    }
}

s64 DWORDToInt(DWORD Hi, DWORD Lo) {
    LARGE_INTEGER Number;
    Number.HighPart = Hi;
    Number.LowPart  = Lo;
    return Number.QuadPart;
}

array<file> ReadDirectory(str *Directory, bool GetFileSizes) {
    array<file> Files;

    auto FindPattern  = Directory->Cat('*');
    auto FindPatternW = UTF8ToWide(&FindPattern);

    HANDLE FindHandle = NULL;
    WIN32_FIND_DATAW FileInfo;
    while ((FindHandle = NextFile(&FileInfo, FindHandle, FindPatternW.Wchars, true))) {
        file New;
        New.Name = WideToUTF8(FileInfo.cFileName);
        New.Size = DWORDToInt(FileInfo.nFileSizeHigh, FileInfo.nFileSizeLow);
        if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            New.Type = file_type::Directory;
        else
            New.Type = file_type::File;
        Files.Push(New);
    }
    CloseHandle(FindHandle);

    Free(FindPattern.Chars);
    Free(FindPatternW.Wchars);

    return Files;
}

strw StringAppend(strw *String, wchar_t Character) {
    strw Result;

    Result.Size = String->Size + sizeof(wchar_t);
    Result.Wchars = (wchar_t*)LocalAlloc(LPTR, Result.Size + sizeof(wchar_t));

    memcpy(Result.Wchars, String->Wchars, String->Size);
    Result.Wchars[String->Size/sizeof(wchar_t)] = L'*';
    Result.Wchars[String->Size/sizeof(wchar_t)+1] = L'\0';

    return Result;
}

strw NewString(wchar_t *String) {
    strw Result = {};
    Result.Size = wcslen(String) * sizeof(wchar_t);
    Result.Wchars = String;
    return Result;
}

int RunCommandLineProgram(str *Command) {
    STARTUPINFOW        StartupInfo = {};
    PROCESS_INFORMATION ProcessInfo = {};
    StartupInfo.cb = sizeof(StartupInfo);

    auto CommandW = UTF8ToWide(Command);

    LPCWSTR               lpApplicationName    = NULL;
    LPWSTR                lpCommandLine        = CommandW.Wchars;
    LPSECURITY_ATTRIBUTES lpProcessAttributes  = NULL;
    LPSECURITY_ATTRIBUTES lpThreadAttributes   = NULL;
    BOOL                  bInheritHandles      = FALSE;
    DWORD                 dwCreationFlags      = 0;
    LPVOID                lpEnvironment        = NULL;
    LPCWSTR               lpCurrentDirectory   = NULL;
    LPSTARTUPINFOW        lpStartupInfo        = &StartupInfo;
    LPPROCESS_INFORMATION lpProcessInformation = &ProcessInfo;

    BOOL ProcessCreated = CreateProcessW(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation
    );

    DWORD ExitCode = -1;

    if (!ProcessCreated) {
        auto Error = LastError();
        Printf(c_dim_red "[E]" c_grey " Failed to create a process: " c_dim_red FSTR c_default "\n", (int)Error.Size, Error.Chars);
        return -1;
    }

    DWORD WaitResult = WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    switch (WaitResult) {
        case WAIT_ABANDONED: break;
        case WAIT_OBJECT_0:  ExitCode = 0; break;
        case WAIT_TIMEOUT:   break;
        case WAIT_FAILED:    break;
        default:             break;
    }

    if (ExitCode == 0) {
        BOOL Ok = GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
        if (!Ok) {
            Printf("[W] Unexpectedly failed to get an exit code for the command line tool " FSTR "\n", (int)Command->Size, Command->Chars);
            return -1;
        }
    }

    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
    Free(CommandW.Wchars);

    return (int)ExitCode;
}

wchar_t *StrWCopy(wchar_t *Dst, str *Src) {
    auto SrcW = UTF8ToWide(Src);
    Copy(Dst, SrcW.Wchars, SrcW.Size);
    wchar_t *End = &Dst[SrcW.Size/sizeof(wchar_t)];
    *End = L'\0';
    Free(SrcW.Wchars);
    return End;
}

wchar_t *StrWCopy(wchar_t *Dst, wchar_t *Src) {
    while (*Src != L'\0') {
        *Dst++ = *Src++;
    }
    *Dst = L'\0';
    return Dst;
}

file_type::file_type FileType(str *Path) {
    auto PathW = UTF8ToWide(Path);
    DWORD Type = GetFileAttributesW(PathW.Wchars);
    Free(PathW.Wchars);

    if (Type == INVALID_FILE_ATTRIBUTES) {
        return file_type::Invalid;
    }

    if (Type & FILE_ATTRIBUTE_DIRECTORY)
        return file_type::Directory;
    else
        return file_type::File;
}

wchar_t * StrWDup(wchar_t *String) {
    usize Size = 0;
    for (auto C = String; *C != L'\0'; ++C) {
        Size += 1;
    }
    auto Result = MallocCount<wchar_t>(Size+1);
    StrWCopy(Result, String);
    return Result;
}

void DeleteDirectory_(wchar_t *Path, wchar_t *PathEnd) {
    PathEnd[0] = L'*'; PathEnd[1] = L'\0';

    HANDLE FindHandle = NULL;
    WIN32_FIND_DATAW FileInfo = {};
    while ((FindHandle = NextFile(&FileInfo, FindHandle, Path, true))) {
        if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            auto End = StrWCopy(PathEnd, FileInfo.cFileName);
            End[0] = L'\\'; End[1] = L'\0';
            DeleteDirectory_(Path, &End[1]);
            PathEnd[0] = L'*'; PathEnd[1] = L'\0';
        } else {
            auto End = StrWCopy(PathEnd, FileInfo.cFileName);
            if (!DeleteFileW(Path)) {
                SetFileAttributesW(Path, FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFileW(Path)) {
                    auto TMP = WideToUTF8(Path);
                    auto Error = LastError();
                    Printf(c_red "[E]" c_grey "Failed to remove file: " c_yellow FSTR c_default FSTR "\n", (int)TMP.Size, TMP.Chars, (int)Error.Size, Error.Chars);
                    Free(TMP.Chars);
                    Exit(0);
                }
            }

            PathEnd[0] = L'*'; PathEnd[1] = L'\0';
        }
    }
    FindClose(FindHandle);
    PathEnd[0] = '\0';

    PathEnd[0] = L'\0';
    if (!RemoveDirectoryW(Path)) {
        SetFileAttributesW(Path, FILE_ATTRIBUTE_NORMAL);
        if (!RemoveDirectoryW(Path)) {
            auto TMP = WideToUTF8(Path);
            auto Error = LastError();
            Printf("Failed to remove directory [" FSTR "]: (" FSTR ")\n", (int)TMP.Size, TMP.Chars, (int)Error.Size, Error.Chars);
            // PrintString(TMP, false);
            // Printf("]: (%ls)\n", LastError());
            Free(TMP.Chars);
            Exit(0);
        }
    }
}

void Delete(str *Path) {
    auto PathBuffer = MallocCount<wchar_t>(32767 + 1);
    auto PathEnd    = StrWCopy(PathBuffer, Path);

    switch (FileType(Path)) {
        case file_type::Directory: {
            if (PathEnd[-1] != L'\\') *PathEnd++ = L'\\';
            DeleteDirectory_(PathBuffer, PathEnd);
        } break;
        case file_type::File: {
            DeleteFileW(PathBuffer);
        } break;
        case file_type::Invalid: {
            auto Error = LastError();
            Printf(c_red "[E]" c_grey " Cannot remove \"" c_yellow FSTR c_grey "\"" c_default " (" FSTR ")\n", (int)Path->Size, Path->Chars, (int)Error.Size, Error.Chars);
            // PrintString(Path, false);
            // Printf(c_grey "\"" c_default "\n");
        } break;
      break;
    }
}

