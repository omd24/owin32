/* ===========================================================
   #File: handshake.c #
   #Date: 10 July 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description: Studying usage of auto-reset events #
   #The main idea is to reverse a string with two threads
   #Client thread (primary thread itself) submits requests
   #Server thread reverses the submitted string
   #Shutting down is done with a special string value
   #Reference: "Windows via C/C++" 09-Handshake example
   #Notice: (C) Copyright 2021 by Omid. All Rights Reserved. #
   =========================================================== */

#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <tchar.h>
#include <windowsx.h>

#include "resource.h"   /* ui controls IDs (from editor) */

// =========================================================================================

/* X could be null */
#pragma warning(disable:28183)
/* variable must be accessed via an Interlocked function */
#pragma warning(disable:28112)
/* X could be 0 */
#pragma warning(disable:6387)

// =========================================================================================

//
// Event to signal when client submits a request to server
HANDLE g_event_request_submitted;

//
// Event to signal when server returns the result to client
HANDLE g_event_result_returned;

//
// Buffer shared between clinet and server
TCHAR g_str_shared[1024];

//
// Special value sent from client to mark shutdown
// NOTE(omid): a mechanism for server thread to terminate more cleanly
TCHAR g_str_shutdown [] = TEXT("Server Shutdown");

//
// Server will check main dialog is no longer active when shutdown str is received
HWND g_dialog_main;

// =========================================================================================

unsigned WINAPI
ServerThread_Func (void * param_ptr) {
   // -- assume server is to run forever
    BOOL shutdown = FALSE;
    while (!shutdown) {
         // -- wait for client to submit request
        WaitForSingleObject(g_event_request_submitted, INFINITE);

        // -- check to see if client wants the server to shut down
        shutdown =
            (NULL == g_dialog_main) &&
            (0 == _tcscmp(g_str_shared, g_str_shutdown));

        if (FALSE == shutdown) // process the request
            _tcsrev(g_str_shared);  // reverse the string

        // -- let the client know the result is ready
        SetEvent(g_event_result_returned);
    }
     // -- always return from exiting thread
    return(0);
}

// =========================================================================================
//
// from "Windows via C/C++" source code:
// The normal HANDLE_MSG macro in WindowsX.h does not work properly for dialog boxes
// because DialogProc callback returns a BOOL instead of an LRESULT (like WndProcs)
// This HANDLE_DIALOGBOX_MSG macro corrects the problem:
//
#define HANDLE_DIALOGBOX_MSG(hwnd, msg, fn)                 \
   case (msg): return (SetDlgMsgResult(hwnd, umsg,          \
      HANDLE_##msg((hwnd), (wparam), (lparam), (fn))))
// =========================================================================================

BOOL
DialogBox_OnInit (HWND hwnd, HWND hwnd_focus, LPARAM lparam) {
    // -- initialize edit control with some test data request
    Edit_SetText(GetDlgItem(hwnd, ID_TXT_REQUEST), TEXT("TEST DATA..."));

    // -- store main dialog handle
    g_dialog_main = hwnd;

    return TRUE;
}
void
DialogBox_OnCommand (HWND hwnd, int id, HWND hwnd_control, UINT code_notify) {
    switch (id) {
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    case ID_BTN_SUBMIT:     // submit a request to server thread
        // -- copy request string to shared buffer
        Edit_GetText(
            GetDlgItem(hwnd, ID_TXT_REQUEST),
            g_str_shared,
            _countof(g_str_shared)
        );

        // -- let server know a request is ready,
        // -- and wait for the server to process it
        SignalObjectAndWait(
            g_event_request_submitted,
            g_event_result_returned,
            INFINITE, FALSE
        );

        // -- after receiving the result, let the user know it
        Edit_SetText(GetDlgItem(hwnd, ID_TXT_RESULT), g_str_shared);
        break;
    }
}
INT_PTR WINAPI
DialogBox_Func (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    switch (umsg) {
        HANDLE_DIALOGBOX_MSG(hwnd, WM_INITDIALOG, DialogBox_OnInit);
        HANDLE_DIALOGBOX_MSG(hwnd, WM_COMMAND, DialogBox_OnCommand);
    }
    return FALSE;
}

// =========================================================================================

int WINAPI
_tWinMain(
    _In_ HINSTANCE instance,
    _In_opt_ HINSTANCE prev,
    _In_ LPWSTR cmdline,
    _In_ int showcmd
) {
    UNREFERENCED_PARAMETER(prev);
    UNREFERENCED_PARAMETER(showcmd);

    // -- create two nonsignaled, auto-reset events
    g_event_request_submitted = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_event_result_returned = CreateEvent(NULL, FALSE, FALSE, NULL);

    // -- spawn server thread
    unsigned int thread_id;
    HANDLE thread_server =  (HANDLE)_beginthreadex(
        NULL, 0, ServerThread_Func, NULL, 0, &thread_id
    );

    // -- execute client/main/primary thread UI
    DialogBox(instance, MAKEINTRESOURCE(ID_DIALOG_MAIN), NULL, &DialogBox_Func);
    g_dialog_main = NULL;

    //
    // Dialog box is closed
    // -- tell server thread to shutdown
    _tcscpy_s(g_str_shared, _countof(g_str_shared), g_str_shutdown);
    SetEvent(g_event_request_submitted);

    // -- wait for server to acknowledge the shutdown and fully terminate
    HANDLE h[] = {g_event_result_returned, thread_server};
    WaitForMultipleObjects(_countof(h), h, TRUE, INFINITE);

    // -- cleanup
    CloseHandle(thread_server);
    CloseHandle(g_event_request_submitted);
    CloseHandle(g_event_result_returned);

    // Client thread terminates with whole process
    return(0);
}
