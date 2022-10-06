#ifndef COMMON_H
#define COMMON_H

//------------------------------------------------------------------------------

typedef __UINT8_TYPE__ u8;
typedef __INT8_TYPE__  s8;
typedef __UINT16_TYPE__ u16;
typedef __INT16_TYPE__  s16;
typedef __INT32_TYPE__  s32;
typedef __UINT32_TYPE__ u32;
typedef __INT64_TYPE__  s64;
typedef __UINT64_TYPE__ u64;
typedef u32 uint;
typedef u64 off;
typedef u64 usize;

//------------------------------------------------------------------------------

#ifndef NULL
    #define NULL 0
#endif
#define INLINE __attribute__((__always_inline__))
#define NOINLINE __attribute__((__noinline__))
#define naked __attribute__((__naked__))
#define PRINTFLIKE(Format_Idx, Args_Idx) __attribute__((__format__(__printf__, Format_Idx, Args_Idx))) // Both param indexes are 1–based, for example: `PRINTFLIKE(1,2) int foo(FormatString, ...)`
#define ASM __asm__ __volatile__
#define CAT_(a,b) a##b
#define CAT(a,b) CAT_(a,b)
#define STR_(s) #s
#define STR(s) STR_(s)
#define IFLIKELY(x)   if(__builtin_expect(x,1))
#define IFUNLIKELY(x) if(__builtin_expect(x,0))
#define KILOBYTES(x) (x*1024llu)
#define MEGABYTES(x) KILOBYTES(x*1024llu)
#define GIGABYTES(x) MEGABYTES(x*1024llu)
#define TERABYTES(x) GIGABYTES(x*1024llu)
#define PETABYTES(x) TERABYTES(x*1024llu)
#define MAX(a,b) ({ auto a_ = a; auto b_ = b; a_ > b_ ? a_ : b_ ;})
#define MIN(a,b) ({ auto a_ = a; auto b_ = b; a_ > b_ ? b_ : a_ ;})
#define SYMBOL CAT(symbol,CAT(__LINE__,__COUNTER__)) // Semi-unique
#define ALIGN(alignment) __attribute__((__aligned__(alignment)))
// #define alignof(type) __alignof(type)
// #define overload __attribute__((__overloadable__))
#define TYPEOF __typeof__
#define RESTRICT __restrict__

//------------------------------------------------------------------------------

#if TERMCOLOR
#define c_default     "\33[0m"
#define c_dim_grey    "\33[30m"
#define c_dim_red     "\33[31m"
#define c_dim_green   "\33[32m"
#define c_dim_yellow  "\33[33m"
#define c_dim_blue    "\33[34m"
#define c_dim_magenta "\33[35m"
#define c_dim_cyan    "\33[36m"
#define c_grey        "\33[90m"
#define c_red         "\33[91m"
#define c_green       "\33[92m"
#define c_yellow      "\33[93m"
#define c_blue        "\33[94m"
#define c_magenta     "\33[95m"
#define c_cyan        "\33[96m"
#define c_underline   "\33[4m"
#define c_nounderline "\33[24m"
#define c_italic      "\33[3m"
#define c_noitalic    "\33[23m"
#else
#define c_default
#define c_dim_grey
#define c_dim_red
#define c_dim_green
#define c_dim_yellow
#define c_dim_blue
#define c_dim_magenta
#define c_dim_cyan
#define c_grey
#define c_red
#define c_green
#define c_yellow
#define c_blue
#define c_magenta
#define c_cyan
#define c_underline
#define c_nounderline
#define c_italic
#define c_noitalic
#endif

//------------------------------------------------------------------------------

// void Copy(void *Dest, void *Src, usize Size);
// __attribute__((__format__(__printf__, 3, 4))) int snprintf(char * __RESTRICT out, __SIZE_TYPE__ size, const char * __RESTRICT format, ...);
// __attribute__((__format__(__printf__, 1, 2))) int printf(const char *format, ...);
// void * memcpy(void *dest, const void *src, __SIZE_TYPE__ size);
// void free(void *dest);
// void * malloc(__SIZE_TYPE__ size);
// void * realloc(void *p, __SIZE_TYPE__ size);
// int strncmp(const char * a, const char * b, unsigned long size);

//------------------------------------------------------------------------------

#if DEBUG_PRINT
#define  Debug_Info(format, ...) printf(c_grey   "[I] "        format c_default "\n", ##__VA_ARGS__)
#define Debug_Error(format, ...) printf(c_red    "[E] " c_grey format c_default "\n", ##__VA_ARGS__)
#define  Debug_Warn(format, ...) printf(c_yellow "[W] " c_grey format c_default "\n", ##__VA_ARGS__)
#else
#define  Debug_Info(format, ...) (void)0
#define Debug_Error(format, ...) (void)0
#define  Debug_Warn(format, ...) (void)0
#endif

//------------------------------------------------------------------------------

#define assert0(expr) assert_(expr, STR(expr), (char*)__FILE__, (char*)STR(__LINE__), (char*)__FUNCTION__)

#if(ASSERT_LEVEL > 1)
    #define assert2(expr) assert_(expr, STR(expr), (char*)__FILE__, (char*)STR(__LINE__), (char*)__FUNCTION__)
#else
    #define assert2(expr) expr
#endif
void assert_(bool expr, char *expr_as_string, char *filename, char *line_number, char *function);

//------------------------------------------------------------------------------

#if DEBUG_ALLOCATIONS
void * reallocate_debug(void *original, usize new_data_size, char *function, char *file, int line);
void deallocate_debug(void *allocation);
void * allocate_debug(usize data_size, char *function, char *file, int line);
#define         allocate(size)               allocate_debug(size, (char*)__FUNCTION__, __FILE__, __LINE__)
#define       reallocate(original, new_size) reallocate_debug(original, new_size, (char*)__FUNCTION__, __FILE__, __LINE__)
#define       deallocate(allocation)         deallocate_debug(allocation)
bool allocation_validate(void *allocation);
#else
#define allocate(size)                 malloc(size)
#define reallocate(original, new_size) realloc(original, new_size)
#define alloc_debug_print(allocation)  (void)(0)
#define deallocate(allocation)         free(allocation)
#endif

//------------------------------------------------------------------------------
#if 0
struct array {
    #define ARRAY_MAGIC 0x79727261
    u32 magic;
    u16 element_size;
    u16 element_align;
    void *start;
    off cursor;
    off end;
};

#define             array_start(type, initial_capacity) (TYPEOF(type)*)array_start_(initial_capacity, sizeof(type), alignof(type))
#define              array_push(array_data)             (TYPEOF(array_data))array_push_((void**)&array_data)
#define              array_free(array_data)             array_free_((void**)&array_data)
#define       array_foreach_idx(array_data, index)      for (usize index = 0, end = array_count(array_data); index < end; ++index)
#define           array_foreach(array_data, it)         array_foreach_(array_data, it, SYMBOL)
usize               array_count(void *array_data);
usize            array_capacity(void *array_data);
float array_utilization_percent(void *array_data);

#define array_foreach_(array_data, it, array_count_) usize array_count_ = array_count(array_data); for (auto it = array_data; it < &array_data[array_count_]; ++it)
void * array_start_(usize initial_capacity, u16 element_size, u16 element_align);
usize array_indexof(void *array_data, void *element);
void * array_push_(void **array_data);
void  array_free_(void **array_data);
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#else

// template <typename T>
// struct slice {
//     T *Data;
//     usize Count;
//     slice() {
//         this->Count = 0;
//         this->Data  = nullptr;
//     }
// };

template <typename T>
struct slice {
    T *Data;
    usize Count;

    slice () {
        this->Data = nullptr;
        this->Count = 0;
    }
    slice (T *Data, usize Count) {
        this->Data = Data;
        this->Count = Count;
    }
};

template <typename T>
struct array {
    T *Data;
    usize Capacity;
    usize Count;

    array();
    array(usize Capacity);
    void Reserve(usize);
    T * Push();
    T * Push(T *);
    T * Push(const T &);
    void Reset() {
        this->Count = 0;
    }
    T * PushCount(usize Count) {
        auto NewCount = this->Count + Count;
        this->Reserve(NewCount);

        auto Result = &this->Data[this->Count];
        this->Count = NewCount;
        return Result;
    }

    slice<T> SliceStartingWith(T *First) {
        // DEBUG:
        usize Index = -1;
        for (usize I = 0; I < this->Count; ++I) {
            if (&this->Data[I] == First) {
                Index = I;
                break;
            }
        }
        bool IsPartOfThisArray = Index != -1;
        assert0(IsPartOfThisArray);

        slice<T> Result = slice<T>(First, this->Count - Index);

        return Result;
    }
};

#define foreach(Array_) for (auto It = (Array_).Data, End_ = &(Array_).Data[(Array_).Count]; It < End_; ++It)

#endif

//------------------------------------------------------------------------------

char * format_size(usize size);
char * FormatNanoseconds(u64 time);
template <typename T> T * MallocCount(usize Count);

//------------------------------------------------------------------------------

struct string {
    char *String;
    uint Size;

    bool StartsWith(char Char) {
        return (this->Size > 0 && this->String[0] == Char);
    }

    bool StartsWith(char *String, uint StringSize) {
        if (this->Size < StringSize) return false;
        for (uint I = 0; I < StringSize; ++I) {
            if (this->String[I] != String[I]) return false;
        }
        return true;
    }

    bool StartsWith(char *String) {
        uint StringSize = 0;
        for (char *C = String; *C; ++C) StringSize += 1;
        return this->StartsWith(String, StringSize);
    }

    bool Equal(string &String) {
        if (String.Size != this->Size) return false;
        for (uint I = 0; I < String.Size; ++I) {
            if (this->String[I] != String.String[I]) return false;
        }
        return true;
    }

    bool Equal(string *String) {
        return this->Equal(*String);
    }
};



string NewStringCopy(char *String, usize Size);
bool StringEndsWith(string *String, char Char);
string StringAppend(string *String, char Char);

void PrintString(string &String, bool NewLine = true);
void PrintString(string *String, bool NewLine = true);

bool Equal(void *A, void *B, usize Size);

struct str {
    char *Chars;
    usize Size;

    str();
    str(char *);
    str(char *Str, usize Size);

    static str Until(str Str, char Char);
    static usize StrSize(char *Str);
    bool StartsWith(char Char);
    bool StartsWith(str Str);
    bool Equal(str Str);
    str Substring(usize Size);
    void Append(str Str);
    void Append(char Char);
    void Append(char *Chars, usize Size);
    bool EndsWith(char Char);
    str Cat(char Char);
    bool Contains(char Char);

    static str Copy(char *Chars, usize Size);
};


//------------------------------------------------------------------------------

#endif