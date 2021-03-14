/* ===========================================================
   #File: source.c #
   #Date: 14 Mar 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description: An overivew of [c]strings foundations such as
                    ANSI vs UTF-16, Copy, concat and compare strings
   #Notice: (C) Copyright 2021 by Omid. All Rights Reserved. #
   =========================================================== */

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
//#undef UNICODE    // Don't! instead use Properties > Advanced > Character Set

#ifndef UNICODE
#define UNICODE
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#include <windows.h>
#include <stdio.h>

#include <tchar.h>  // _T macro, _tcslen
#include <stdlib.h> // _countof

#include <strsafe.h> // StringCchCat, StringCbCat, StringCchCopy, StringCbCopy

int main (void) {
#pragma region ANSI vs UTF-16
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
    TCHAR sz_buffer3[100] = TEXT("A String");
    // =======================================================================================================

    //
    // Display Results
    //
    printf("=============================\n");
    printf("ANSI vs UNICODE results\n");
    printf("=============================\n");
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
    // NOTE(omid): _tcslen expands to 
    printf("length of sz_buffer3 = %llu\n", _tcslen(sz_buffer3));
#pragma endregion ANSI vs UTF-16
    printf("=============================\n");
#pragma region Secure vs Unsecure string manipulation
    //
    // Unsecure string copy
    //
#if 0   // The followings put 4 characters in 3-character buffer, resulting in memory corruption
    // -- using UTF-16 string
    WCHAR str[3] = L"";
    wcscpy(str, L"abc"); // The terminating 0 is a character too!
    // -- using ANSI string
    CHAR str[3] = "";
    strcpy(str, "abc"); // The terminating 0 is a character too!
#endif
    //
    // Secure string copy
    //
    // -- using UTF-16 string
    WCHAR str_w[4] = L"";
    wcscpy_s(str_w, _countof(str_w), L"abc");
    // -- using ANSI string
    CHAR str_a[4] = "";
    strcpy_s(str_a, _countof(str_a), "abc");

    // NOTE(omid): When a writable buffer is passed as a parameter, its size must also be provided.

#ifndef _UNICODE
    char str_dst[100];
#else
    wchar_t str_dst[100];
#endif
    // Unsecure copy with Generic-Text Routine Mappings
    _tcscpy(str_dst, _T("Hello World! (unsecure)"));

    // Secure copy with Generic-Text Routine Mappings
    _tcscpy_s(str_dst, _countof(str_dst), _T("Hello World! (secure)"));

    //
    // StringCbCopy, StringCchCopy, StringCbCat, StringCchCat
    //
#ifndef _UNICODE
    char str_cpy_cat[100];
#else
    wchar_t str_cpy_cat[100];
#endif
    // NOTE(omid): Cch functions use _countof (count of characters) and Cb functions use sizeof (byte-size)
    StringCbCopy(str_cpy_cat, sizeof(str_cpy_cat), str_dst);
    StringCchCat(str_cpy_cat, _countof(str_cpy_cat), _T(" additional text"));
    /// NOTE(omid): Unlike the secure (_s suffixed) functions, when a buffer is too small, these functions do perform truncation 

    //
    // Display Results
    //
    printf("Secure vs Unsecure string manipulation\n");
    printf("=============================\n");
    // NOTE(omid): _tprintf expands to wprintf if _UNICODE defined otherwise printf
    _tprintf(_T("str_dst = %s\n"), str_dst);
    _tprintf(_T("size of str_dst = %llu\n"), sizeof(str_dst));
    _tprintf(_T("str_cpy_cat = %s\n"), str_cpy_cat);
#pragma endregion Secure vs Unsecure string manipulation
    printf("=============================\n");
#pragma region String Compare
    printf("String Compare\n");
    printf("=============================\n");
    _tprintf(_T("1st string = %s\n"), str_cpy_cat);
    _tprintf(_T("2st string = %s\n"), str_dst);
    printf("\n");
//
// win32 approach
//
    int res = CompareStringEx(
        LOCALE_NAME_INVARIANT,
        LINGUISTIC_IGNORECASE,
        str_cpy_cat, -1,
        str_dst, -1,
        NULL, NULL, 0
    );
    res -= 2;
    if (0 == res)
        printf("strings equal (CompareStringEx)\n");
    else if (0 < res)
        printf("1st string greater (CompareStringEx)\n");
    else if (0 > res)
        printf("1st string less (CompareStringEx)\n");
//
// C approach
//
    res = _tcscmp(str_cpy_cat, str_dst);
    /*
    res = strcmp(str_cpy_cat, str_dst);
    res = wcscmp(str_cpy_cat, str_dst);
    */
    if (0 == res)
        printf("strings equal (_tcscmp)\n");
    else if (0 < res)
        printf("1st string greater (_tcscmp)\n");
    else if (0 > res)
        printf("1st string less (_tcscmp)\n");
#pragma endregion String Compare
    printf("=============================\n");
    return(0);
}

