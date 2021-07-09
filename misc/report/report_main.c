#include <windows.h>
#include "Reprt_Err.h"

#ifdef _UNICODE
#define _memtchr wmemchr
#else
#define _memtchr memchr
#endif


#pragma region options Impl
#include <stdarg.h>
/*  argv is the command line.
 The options, if any,
 start with a '-' in argv[1], argv[2], ...
 opt_str is a string containing all possible options,
 in one-to-one correspondence with
 the addresses of Boolean variables
 in the variable argument list (...).
 These flags are set if and only if
 the corresponding option character occurs in
 argv [1], argv [2], ...
 The return value is the argv index of
 the first argument beyond the options. */

DWORD options (
    int argc, LPCTSTR argv [], LPCTSTR opt_str, ...
) {
    va_list flag_list;
    LPBOOL flag;
    int i_flag = 0, i_arg;

    va_start (flag_list, opt_str);

    while (
        (flag = va_arg (flag_list, LPBOOL)) != NULL &&
        i_flag < (int)_tcslen (opt_str)
        ) {
        *flag = FALSE;
        for (
            i_arg = 1;
            !(*flag) && i_arg < argc &&
            argv[i_arg][0] == _T('-');
            i_arg++
            ) {
            *flag = _memtchr(
                argv[i_arg], opt_str[i_flag],
                _tcslen (argv[i_arg])
            ) != NULL;
        }
        i_flag++;
    }

    va_end (flag_list);

    for (
        i_arg = 1;
        i_arg < argc && argv[i_arg][0] == _T('-');
        i_arg++
        );

    return i_arg;
}
#pragma endregion

#define BUF_SIZE 0x200

static VOID
cat_file (HANDLE infile, HANDLE outfile) {
    DWORD n_in, n_out;
    BYTE buf[BUF_SIZE];

    while (
        ReadFile(infile, buf, BUF_SIZE, &n_in, NULL) &&
        (n_in != 0) &&
        WriteFile(outfile, buf, n_in, &n_out, NULL)
        );

    return;
}
int _tmain (int argc, LPTSTR argv []) {
    HANDLE infile, hstdin;
    hstdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL dash_s;
    int i_arg, i_first;

    /* dash_s will be set only if "-s" is on cmd. */
    /* i_first is the argv [] index of
    the first input file. */
    i_first = options(
        argc, argv, _T("s"), &dash_s, NULL
    );
    int x = 10;
    if (i_first == argc) { /* No files in arg list. */
        cat_file(hstdin, hstdout);
        return 0;
    }

    /* Process the input files. */
    for (i_arg = i_first; i_arg < argc; i_arg++) {
        infile = CreateFile(
            argv[i_arg], GENERIC_READ,
            0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL
        );
        if (infile == INVALID_HANDLE_VALUE) {
            if (!dash_s)
                ReportError(
                    _T("Cat Error: File not exist."),
                    0, TRUE
                );
        } else {
            cat_file (infile, hstdout);
            if (GetLastError() != 0 && !dash_s) {
                ReportError(
                    _T("Cat Error: Cant process file"),
                    0, TRUE
                );
            }
            CloseHandle(infile);
        }
    }
    return 0;
}



