#include "platform.h"
#include "common.h"

// ---------------------------------------------------------------------------------------

template <typename T>
void CopyCount(T * Dst, const T & RESTRICT Src, usize Count) {
    Copy(Dst, &Src, sizeof(T) * Count);
}

template <typename T>
void CopyCount(T * Dst, const T * RESTRICT Src, usize Count) {
    CopyCount(Dst, *Src, Count);
}

template <typename T>
T * MallocCount(usize Count) {
    auto Result = Malloc(Count * sizeof(T));
    return (T *)Result;
}

// ---------------------------------------------------------------------------------------

template <typename T>
array<T>::array(usize Capacity) {
    this->Count = 0;
    this->Capacity = Capacity;
    this->Data = MallocCount<T>(Capacity);
}

template <typename T>
array<T>::array() {
    this->Count = 0;
    this->Capacity = 10;
    this->Data = MallocCount<T>(this->Capacity);
}

template <typename T>
void array<T>::Reserve(usize NewCapacity) {
    if (NewCapacity <= this->Capacity) return;

    T *New = MallocCount<T>(NewCapacity);
    CopyCount(New, this->Data, this->Count);
    Free(this->Data);

    this->Data     = New;
    this->Capacity = NewCapacity;
}

template <typename T>
T * array<T>::Push() {
    if (this->Count == this->Capacity) {
        Reserve(this->Capacity * 2);
    }
    return &this->Data[this->Count++];
}

template <typename T>
T * array<T>::Push(const T &Item) {
    auto New = Push();
    CopyCount(New, Item, 1);
    return New;
}

template <typename T>
T * array<T>::Push(T *Item) {
    auto Result = Push(*Item);
    return Result;
}

// ---------------------------------------------------------------------------------------

void
assert_(bool expr, char *expr_as_string, char *filename, char *line_number, char *function) {
    IFUNLIKELY(!expr) {
        Printf(
            "\n"
            "    " c_default "%s() " c_grey "{\n"
            "    " "    " c_underline c_red "assert" c_grey c_nounderline "(" c_yellow "%s" c_grey ") // %s[%s]\n"
            "    " "}" c_default
            "\n",
            function, expr_as_string, filename, line_number
        );
        asm(
            "int3 \t\n"
            "nop  \t\n"
        );
    }
}

// ---------------------------------------------------------------------------------------

string NewStringCopy(char *String, usize Size) {
    string Result;
    Result.Size = Size;
    Result.String = (char*)Malloc(Size * sizeof(char));
    Copy(Result.String, String, Size);
    return Result;
}

bool StringEndsWith(string *String, char Char) {
    bool Result = String->Size && String->String[String->Size] == Char;
    return Result;
}

string StringAppend(string *String, char Char) {
    string Result;
    Result.Size   = String->Size + 1;
    Result.String = (char*)Malloc(Result.Size + 1);
    Copy(Result.String, String->String, String->Size);
    Result.String[Result.Size-1] = Char; // Safe to "-1" because "Size" will be at least "1".
    Result.String[Result.Size]   = '\0';
    return Result;
}

void PrintString(string &String, bool NewLine) {
    if (NewLine) Printf(FSTR "\n", String.Size, String.String);
    else Printf(FSTR, String.Size, String.String);
}

void PrintString(string *String, bool NewLine) {
    PrintString(*String, NewLine);
}

bool Equal(void *A, void *B, usize Size) {
    auto Ac = (u8*)A;
    auto Bc = (u8*)B;
    for (usize I = 0; I < Size; ++I) {
        if (Ac[I] != Bc[I]) return false;
    }
    return true;
}

// str ----------------------------------------------------------------------------------

str str::Substring(usize Size) {
    assert0(Size <= this->Size);
    str Result;
    Result.Chars = this->Chars;
    Result.Size  = Size;
    return Result;
}

usize str::StrSize(char *Str) {
    usize Result = 0;
    while (*Str++) Result += 1;
    return Result;
}

str::str(char *String, usize Size) {
    this->Chars = String;
    this->Size  = Size;
}

str::str(void) {
    this->Chars = NULL;
    this->Size  = 0;
}

str::str(char *String) {
    this->Chars = String;
    this->Size  = str::StrSize(String);
}

bool str::Equal(str Str) {
    if (this->Size != Str.Size) return false;
    return ::Equal(this->Chars, Str.Chars, Str.Size);
}

bool str::StartsWith(str B) {
    if (this->Size < B.Size) return false;
    return ::Equal(this->Chars, B.Chars, B.Size);
}

str str::Until(str Str, char Char) {
    str Result;
    Result.Chars = Str.Chars;
    Result.Size  = 0;

    while (Result.Size < Str.Size) {
        if (Str.Chars[Result.Size] == Char) {
            break;
        }
        Result.Size += 1;
    }

    return Result;
}

void str::Append(char *Chars, usize Size) {
    for (usize I = 0; I < Size; ++I) {
        this->Chars[this->Size + I] = Chars[I];
    }
    this->Size += Size;
}

void str::Append(str Str) {
    this->Append(Str.Chars, Str.Size);
}

void str::Append(char Char) {
    this->Chars[this->Size] = Char;
    this->Size += 1;
}

bool str::EndsWith(char Char) {
    if (this->Size == 0) return false;
    return this->Chars[this->Size-1] == Char;
}

str str::Cat(char Char) {
    str Result;
    Result.Chars = MallocCount<char>(this->Size + 1 + 1);
    Result.Size = this->Size;
    for (usize I = 0; I < this->Size; ++I) {
        Result.Chars[I] = this->Chars[I];
    }
    Result.Chars[Result.Size++] = Char;
    Result.Chars[Result.Size] = '\0';
    return Result;
}

str str::Copy(char *Chars, usize Size) {
    str Result;
    Result.Size = Size;
    Result.Chars = MallocCount<char>(Size + 1);
    ::Copy(Result.Chars, Chars, Size);
    Result.Chars[Size] = '\0';
    return Result;
}

bool str::Contains(char Char) {
    for (auto C = this->Chars, End = &this->Chars[this->Size]; C < End; ++C) {
        if (*C == Char) return true;
    }
    return false;
}

// ??? ----------------------------------------------------------------------------------




