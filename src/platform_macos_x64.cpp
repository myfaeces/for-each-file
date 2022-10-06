#include "common.h"
#include "platform.h"

#include <errno.h>
#include "stdlib.h"
#include <cstring>
#include <dirent.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void * Malloc_(usize Size, char *Function) {
    return malloc(Size);
}

void Free(void * Memory) {
    free(Memory);
}

array<file> ReadDirectory(str &Directory, bool GetFileSizes) {
    struct array<file> Result;

    auto Handle = opendir(Directory.Chars);
    if (!Handle) return Result;

    dirent *Entry;
    Entry = readdir(Handle); // Skip "."
    Entry = readdir(Handle); // Skip ".."

    while ((Entry = readdir(Handle))) {
        file File;
        File.Size = 0;
        File.Name = str::Copy(Entry->d_name, Entry->d_namlen);

        auto Type = DTTOIF(Entry->d_type);

        if (S_ISDIR(Type)) {
            File.Type = file_type::Directory;
        }

        if (S_ISREG(Type)) {
            File.Type = file_type::File;
        }

        Result.Push(File);
    }

    return Result;
}

array<file> ReadDirectory(str *Directory, bool GetFileSizes) {
    return ReadDirectory(*Directory, GetFileSizes);
}

file_type::file_type FileType(str *Path) {
    struct stat Stat = {};
    lstat(Path->Chars, &Stat);
    if (S_ISDIR(Stat.st_mode)) return file_type::Directory;
    else if (S_ISREG(Stat.st_mode)) return file_type::File;
    else return file_type::Invalid;
}

void DeleteDirectory(char *Path, char* PathEnd) {
    auto Path_ = str(Path);
    auto Children = ReadDirectory(Path_);

    foreach(Children) {
        switch (It->Type) {
            case file_type::Directory: {
                Copy(PathEnd, It->Name.Chars, It->Name.Size);
                PathEnd[It->Name.Size+0] = '/';
                PathEnd[It->Name.Size+1] = '\0';

                DeleteDirectory(Path, PathEnd + It->Name.Size + 1);
                int DelResult = rmdir(Path);
                if (DelResult) {
                    auto Error = strerror(errno);
                    Printf(c_dim_red "[E] " c_grey "Failed to remove directory \"" c_yellow "%s" c_grey "\" (%s)\n", Path, Error);
                }

                PathEnd[0] = '\0';
            } break;

            case file_type::File: {
                Copy(PathEnd, It->Name.Chars, It->Name.Size);
                PathEnd[It->Name.Size] = '\0';

                int DelResult = unlink(Path);
                if (DelResult) {
                    auto Error = strerror(errno);
                    Printf(c_dim_red "[E] " c_grey "Failed to remove file \"" c_yellow "%s" c_grey "\" (%s)\n", Path, Error);
                }

                PathEnd[0] = '\0';
            } break;

            case file_type::Invalid:
                // @Unhandled:
            break;
        }
    }

    Free(Children.Data);
}

void Delete(str *Path) {
    switch (FileType(Path)) {
        case file_type::Directory: {
            usize PathCount = Path->Size;
            assert0(PathCount > 0);

            auto PathBuffer = MallocCount<char>(99999);
            Copy(PathBuffer, Path->Chars, PathCount);
            if (PathBuffer[PathCount-1] != '/') PathBuffer[PathCount++] = '/';
            PathBuffer[PathCount] = '\0';

            DeleteDirectory(PathBuffer, &PathBuffer[PathCount]);
            int DelResult = rmdir(PathBuffer);
            if (DelResult) {
                auto Error = strerror(errno);
                Printf(c_dim_red "[E] " c_grey "Failed to remove directory \"" c_yellow "%s" c_grey "\" (%s)\n", PathBuffer, Error);
            }

            Free(PathBuffer);
        } break;

        case file_type::File: {
            unlink(Path->Chars); // @NoErrorHandling:
        } break;

        case file_type::Invalid:
            // @NoInvalidArgumentHandling:
        break;
    }
}

str GetCwd() {
    str Result;
    Result.Chars = getcwd(NULL, 0);
    Result.Size   = strlen(Result.Chars);

    // Unless cwd is "/", getcwd() will return path without terminating "/", so we append it manually.
    if (false == Result.EndsWith('/')) {
        auto ResultNormalized = Result.Cat('/');
        free(Result.Chars);
        Result = ResultNormalized;
    }

    return Result;
}

int RunCommandLineProgram(char **Command) {
    int ExitCode = -1;
    pid_t Pid;
    if ((Pid = fork()) < 0) {
        perror("[E] Fork failed");
        ExitCode = -1;
    } else if (Pid > 0) {
        auto res = waitpid(Pid, &ExitCode, 0);
        return ExitCode;
    } else {
        execvp(Command[0], Command);
        // We should not be here!
        exit(-1);
    }
    // We should not be here!
    return ExitCode;
}

int RunCommandLineProgram(array<str> Command) {
    array<char *> Commands;
    foreach(Command) {
        It->Chars[It->Size] = '\0';
        Commands.Push(It->Chars);
    }

    auto ExitCode = RunCommandLineProgram(Commands.Data);

    return ExitCode;
}

void Exit(int ExitCode) {
    _Exit(ExitCode);
}

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
