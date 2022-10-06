#include "common.h"
#include "platform.h"
#include "common.cpp"

#if (MACINTOSH_X64)
    #include "platform_macos_x64.cpp"
#elif (WIN_X64)
    #include "platform_windows_x64.cpp"
#endif

enum token_type {
    token_NAME,
    token_COLON,
    token_TEXT,
    token_NEW_COMMAND
};
struct command_token {
    token_type Type;
    str Str;
};
array<command_token> TokenizeCommands(slice<str> Commands) {
    array<command_token> Tokens = array<command_token>(Commands.Count);

    for (auto It = Commands.Data, End = &Commands.Data[Commands.Count]; It < End; ++It) {
        auto CommandString = It->Chars;
        if (It->Contains('"')) {
            // @TODO: Handling quotes. Nested quotes?
            Printf(c_dim_red "[E]" c_grey " Cannot use argument \"" c_dim_yellow FSTR c_grey "\" (we do not support command arguments that have \" in them (yet). Sorry)" c_default "\n", (int)It->Size, It->Chars);
            Exit(0);
        }

        auto NewCommandToken = Tokens.Push();
        NewCommandToken->Type = token_NEW_COMMAND;

        auto C = str(It->Chars, It->Size);
        while (C.Size > 0) {
            command_token Token;

            str Text = str::Until(C, ':');
            if (Text.Chars) {
                if (Text.Size > 0) {
                    Token.Type = token_TEXT;
                    Token.Str  = Text;
                } else {
                    if (C.StartsWith("::")) {
                        Token.Type = token_COLON;
                        Token.Str.Chars = NULL;
                        Token.Str.Size  = 2;
                    } else if (C.StartsWith(":name")) {
                        Token.Type = token_NAME;
                        Token.Str  = C.Substring(5);
                    } else {
                        if (C.Size == 1) break;

                        C.Size  -= 1;
                        C.Chars += 1;
                        Token.Type = token_TEXT;
                        Token.Str  = str::Until(C, ':');

                        Printf(c_dim_red "[E]" c_grey " Unexpected token: " c_dim_yellow ":" FSTR c_grey c_default "\n", (int)Token.Str.Size, Token.Str.Chars);
                        Exit(0);
                    }
                }
            }

            C.Chars += Token.Str.Size;
            C.Size  -= Token.Str.Size;

            Tokens.Push(Token);
        }
    }

    return Tokens;
}

void Main(array<str> *Args, str *Exe, str *Cwd) {
    if (Args->Count < 2) {
        Printf(
            "\nUsage example:\n"
            "  fef --files cp :name :name_copy\n"
            "  (Will duplicate all files in the working directory adding '_copy' at the end.)\n"
            "\n"
            "Options:\n"
            "  --files - Perform operation on files.\n"
            "  --dirs  - Perform operation on directories.\n"
            "            (If neither is present will do both files and directories.)\n"
            "  --dry   - Do not perform an operation, just echo it to the console.\n"
            "  --del   - Delete file or directory afterwards (only if program was run successfully.\n"
            "\n"
            "Patterns:\n"
            "  :name      - Filename or directory name.\n"
            "  :noextname - Filename (without extention if any) or directory name.\n"
            "  :allfiles  - Will output every file name, separated by space.\n"
            "  :alldir    - Will output every directory name, separated by space.\n"
            "\n"
        );
        Exit(0);
    }

    struct {
        bool DoFiles;
        bool DoDirs;
        bool DryRun;
        bool DeleteAfterwards;
        str *ProgramToRun;
    } Options = {};

    //
    // Parse command line arguments.
    //

    slice<str> Commands; {
        auto ArgFilesOnly = str("--files");
        auto ArgDirsOnly  = str("--dirs");
        auto ArgDryRun    = str("--dry");
        auto ArgDel       = str("--del");

        foreach(*Args) {
            auto Arg = It;
            if (Arg->Equal(ArgFilesOnly)) {
                Options.DoFiles = true;
            } else if (Arg->Equal(ArgDirsOnly)) {
                Options.DoDirs = true;
            } else if (Arg->Equal(ArgDryRun)) {
                Options.DryRun = true;
            } else if (Arg->Equal(ArgDel)) {
                Options.DeleteAfterwards = true;
            } else if (Arg->StartsWith("--")) {
                Printf("[E] Unknown command line argument: " FSTR "\n", (int)Arg->Size, Arg->Chars);
                Exit(0);
            } else {
                Options.ProgramToRun = Arg;
                Commands = Args->SliceStartingWith(Arg+1);
                break;
            }
        }
    }

    //
    // Check executable.
    //

    if (false && file_type::Invalid == FileType(Options.ProgramToRun)) {
        Printf("[E] Cannot run \"" FSTR "\" (file does not exist).\n", (int)Options.ProgramToRun->Size, Options.ProgramToRun->Chars);
        Exit(-1);
    }

    //
    // Tokenize command patterns.
    //

    array<command_token> Tokens = TokenizeCommands(Commands);
    array<str> TargetArgs;
    TargetArgs.Push(str(Options.ProgramToRun->Chars, Options.ProgramToRun->Size));

    // Generate commands passed to the target program.

    auto Files = ReadDirectory(Cwd);

    array<char> CommandString;

    foreach(Files) {
        auto File = It;
        switch (File->Type) {
            default: break;

            case file_type::File: {
                // Printf("[F] "); PrintString(File->Name);
            } break;

            case file_type::Directory: {
                // Printf("[D] "); PrintString(File->Name);

                str *ArgCursor = NULL;

                foreach(Tokens) {
                    switch (It->Type) {
                        case token_NEW_COMMAND: {
                            str NewStr;
                            NewStr.Chars = MallocCount<char>(9999);
                            NewStr.Size  = 0;
                            ArgCursor = TargetArgs.Push(NewStr);
                        } break;

                        case token_NAME: {
                            ArgCursor->Append(str(File->Name.Chars, File->Name.Size));
                        } break;

                        case token_COLON: {
                            ArgCursor->Append(':');
                        } break;

                        case token_TEXT: {
                            ArgCursor->Append(It->Str);
                        } break;
                    }
                }

                foreach(TargetArgs) {
                    bool NeedsQuotes = It->Contains(' ');
                    if (NeedsQuotes) CommandString.Push('"');

                    for (char *C = It->Chars, *End = &It->Chars[It->Size]; C < End; ++C) {
                        CommandString.Push(C);
                    }

                    if (NeedsQuotes) CommandString.Push('"');
                    CommandString.Push(' ');
                }
                assert0(CommandString.Count > 0);
                CommandString.Count -= 1;
                CommandString.Push('\0');

                Printf(c_grey "running " c_cyan FSTR c_grey "..." c_default "\n",
                    (int)CommandString.Count, CommandString.Data);

                if (!Options.DryRun) {
                    auto CommandStr = str(CommandString.Data);
#if MACINTOSH_X64
                    int ExitCode = RunCommandLineProgram(TargetArgs);
#elif WIN_X64
                    int ExitCode = RunCommandLineProgram(&CommandStr);
#endif
                    if (ExitCode != 0) {
                        Printf(c_dim_red "[E]" c_grey " Failed to run " c_cyan FSTR c_grey "\n",
                            (int)CommandStr.Size, CommandStr.Chars);
                        Exit(0);
                    }
                }
                CommandString.Reset();

                if (Options.DeleteAfterwards) {
                    Printf(c_grey "Removing \"" c_dim_yellow FSTR c_grey "\"..." c_default "\n",
                        (int)File->Name.Size, File->Name.Chars);
                    if (!Options.DryRun)
                        Delete(&File->Name);
                }

                TargetArgs.Count = 1;
            } break;
        }
    }
}

#if (MACINTOSH_X64) // ------------------------------------------------------------------
int main(int ArgsCount, char **Args) {
    array<str> Arguments = array<str>(ArgsCount - 1);
    for (auto It = Args+1, End = &Args[ArgsCount]; It < End; ++It) {
        Arguments.Push(str(*It));
    }
    auto Exe = str(*Args);
    auto Cwd = GetCwd();

    Main(&Arguments, &Exe, &Cwd);

    return 0;
}
#elif (WIN_X64) // ----------------------------------------------------------------------
int wmain(int ArgsCount, wchar_t *Args[]) {
    TerminalInit();

    array<str> Arguments = array<str>(ArgsCount - 1);
    for (auto It = Args+1, End = &Args[ArgsCount]; It < End; ++It) {
        auto ArgumentUTF8 = WideToUTF8(*It);
        Arguments.Push(ArgumentUTF8);
    }
    auto Exe = WideToUTF8(*Args);
    auto Cwd = GetCwd();

    Main(&Arguments, &Exe, &Cwd);

    TerminalCleanup();
    return 0;
}
#endif // -------------------------------------------------------------------------------

