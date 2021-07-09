#include "Reprt_Err.h"
#include <stdio.h>
/*
General-purpose function for reporting system errors.
Obtain the error number,
and convert it to the system error message.
Display this information
and the user-specified msg to standard error device.
umsg:       message to be displayed to standard error device.
excode:     0 - Return.
            > 0 - ExitProcess with this code.
print_err:  Display the last system error message ?
*/
void
ReportError (
    LPCTSTR umsg,        // user-message
    DWORD excode,        // exit-code
    BOOL print_err
) {
    DWORD emsg_len, errnum;
    LPTSTR sysmsg;
    errnum = GetLastError();
    _ftprintf(stderr, _T("%s\n"), umsg);
    if(print_err) {
        emsg_len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, errnum,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &sysmsg, 0, NULL
        );
        if (emsg_len > 0) {
            _ftprintf(stderr, _T("%s\n"), sysmsg);
        } else {
            _ftprintf(
                stderr,
                _T("Last Error Number: %d.\n"), errnum
            );
        }
        if (sysmsg != NULL)
            LocalFree(sysmsg);
    }
    if (excode > 0)
        ExitProcess(excode);
    return;
}