#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
//#undef UNICODE    // Don't! instead use Properties > Advanced > Character Set
//#define UNICODE
#include <windows.h>
#include <stdio.h>

#include <tchar.h>  // _T macro
#include <stdlib.h> // _countof

int main (void) {
    // =======================================================================================================
    // An 8-bit ANSI character
    char c1 = 'A';
    // An array of 99 8-bit ANSI characters and an 8-bit terminating zero (NUL).
    char sz_buffer1[100] = "A String";
    // =======================================================================================================

    // =======================================================================================================
    // NOTE(omid): The compiler defines wchar_t data type only when the /Zc:wchar_t compiler switch is specified.
    // A 16-bit Unicode (UTF-16) character
    wchar_t c2 = L'A';
    // An array up to 99 16-bit Unicode (UTF-16) characters and a 16-bit terminating zero (16-bit NUL).
    wchar_t sz_buffer2[100] = L"A String";
    // =======================================================================================================

    // =======================================================================================================
    // NOTE(omid): The _T macro is identical to the _TEXT macro.
    // If UNICODE defined, a 16-bit character; else an 8-bit character
    TCHAR c3 = _T('A');
    // If UNICODE defined, an array of 16-bit characters; else 8-bit characters
    TCHAR sz_buffer3 [100] = TEXT("A String");
    // =======================================================================================================

    //
    // Unsecure string copy
    //
#if 0   // The followings put 4 characters in 3-character buffer, resulting in memory corruption
    // -- using UTF-16 string
    WCHAR sz_buffer_test [3] = L"";
    wcscpy(sz_buffer_test, L"abc"); // The terminating 0 is a character too!
    // -- using ANSI string
    CHAR sz_buffer_test [3] = "";
    strcpy(sz_buffer_test, "abc"); // The terminating 0 is a character too!
#endif
    //
    // Secure string copy
    //
    // -- using UTF-16 string
    WCHAR sz_buffer_test_w [4] = L"";
    wcscpy_s(sz_buffer_test_w, _countof(sz_buffer_test_w), L"abc");
    // -- using ANSI string
    CHAR sz_buffer_test_a [4] = "";
    strcpy_s(sz_buffer_test_a, _countof(sz_buffer_test_a), "abc");


    //
    // Display Results
    //
    printf("size of c1 = %llu\n", sizeof(c1));
    printf("size of c2 = %llu\n", sizeof(c2));
    printf("size of c3 = %llu\n", sizeof(c3));
    printf("length of sz_buffer1 = %llu\n", strlen(sz_buffer1));
    printf("length of sz_buffer2 = %llu\n", wcslen(sz_buffer2));
#ifdef UNICODE
    printf("length of sz_buffer3 = %llu\n", wcslen(sz_buffer3));
#else   // ANSI
    printf("length of sz_buffer3 = %llu\n", strlen(sz_buffer3));
#endif


    return(0);
}

