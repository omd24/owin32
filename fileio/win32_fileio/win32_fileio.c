/* ===========================================================
   #File: win32_fileio.c #
   #Date: 16 March 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description:  copying file with win32 fileio vs c stdio #
   #Notice: (C) Copyright 2021 by Omid. All Rights Reserved. #
   =========================================================== */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#if 0       // c copy file
int main(int argc, char * args []) {
    char buf[100];
    FILE * in;
    FILE * out;
    size_t nread = 0;
    size_t nwritten = 0;

    fopen_s(&in, "f1.txt", "rb");
    fopen_s(&out, "f2.txt", "wb");

    if (in && out) {
        while ((nread = fread(buf, 1, _countof(buf), in)) > 0) {
            nwritten = fwrite(buf, 1, nread, out);
            if (nread != nwritten) {
                perror("Fatal error");
                return 4;
            }
        }
        fclose(out);
        fclose(in);
    }

    return(0);
}
#else
int main (int argc, char * argv []) {
    char buf[100];
    HANDLE in;
    HANDLE out;
    DWORD nread = 0;
    DWORD nwritten = 0;

    in = CreateFile(
        _T("f1.txt"), GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
    );
    out = CreateFile(
        _T("f2.txt"), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
    );
    if (INVALID_HANDLE_VALUE != in &&
        INVALID_HANDLE_VALUE != out) {
        while (ReadFile(in, buf, _countof(buf), &nread, NULL) && nread > 0) {
            WriteFile(out, buf, nread, &nwritten, NULL);
            if (nread != nwritten) {
                printf("Fatal Error: %x\n", GetLastError());
                return 1;
            }
        }
    }

    CloseHandle(out);
    CloseHandle(in);
    return(0);
}
#endif
