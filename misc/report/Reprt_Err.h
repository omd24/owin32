#pragma once

#include <windows.h>
#include <tchar.h>
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
);


