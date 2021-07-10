/* ===========================================================
   #File: queue.c #
   #Date: 10 July 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description: Thread synchronization with kernel objects
    Reworking the queue example using mutex and semaphore
    Semaphore keeps track of the # of elements in the Queue
    Mutex guards the global shared resource (Queue)
   #
   #Reference: "Windows via C/C++" 09-Handshake example #
   #Notice: (C) Copyright 2021 by Omid. All Rights Reserved. #
   =========================================================== */

#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <tchar.h>
#include <strsafe.h>
#include <windowsx.h>

#include "resource.h"   /* ui controls IDs (from editor) */

// =========================================================================================

/* X could be null */
#pragma warning(disable:28183)
/* X must be accessed via an Interlocked function */
#pragma warning(disable:28112)
/* X could be 0 */
#pragma warning(disable:6387)

// =========================================================================================

struct Queue {
    struct Element {
        int thread_number;
        int request_number;
        /* additional element data */

    };
    struct Element *    elements;           // array of elements
    int                 max_elements;       // max # of elements

    enum HANDLE_ID {
        MTX_HID,         // mutex for guarding queue
        SEM_HID,         // semaphore for counting elements

        _COUNT_HIDS
    } HANDLE_IDS;

    HANDLE  handles[_COUNT_HIDS];     // mutex and semaphore handles
};

typedef struct Queue Queue;
typedef struct Element Element;

static void
Queue_Init (Queue * q, int max_e) {
    q->elements = (Element *)HeapAlloc(
        GetProcessHeap(), 0, sizeof(Element) * max_e
    );
    if (q->elements)
        ZeroMemory(q->elements, sizeof(Element) * max_e);

    q->max_elements = max_e;

    q->handles[MTX_HID] = CreateMutex(NULL, FALSE, NULL);
    q->handles[SEM_HID] = CreateSemaphore(NULL, 0, max_e, NULL);
}
static void
Queue_Deinit (Queue * q) {
    CloseHandle(q->handles[MTX_HID]);
    CloseHandle(q->handles[SEM_HID]);
    HeapFree(GetProcessHeap(), 0, q->elements);
}
static BOOL
Queue_Append (Queue * q, Element * e, DWORD timeout) {
    BOOL ret = FALSE;
    DWORD dw = WaitForSingleObject(q->handles[MTX_HID], timeout);

    if (WAIT_OBJECT_0 == dw) {
        // Thread has exclusive access to queue

        // -- increment # of elements
        LONG prev_count;
        ret = ReleaseSemaphore(q->handles[SEM_HID], 1, &prev_count);
        if (ret)    // q is not full, append element
            q->elements[prev_count] = *e;
        else        // q is full, set error code
            SetLastError(ERROR_DATABASE_FULL);

        // -- allow other threads to access q
        ReleaseMutex(q->handles[MTX_HID]);
    } else {    // timeout!
        SetLastError(ERROR_TIMEOUT);
    }
    return ret;     // call GetLastError for more info
}
static BOOL
Queue_Remove (Queue * q, Element * e_out, DWORD timeout) {
    BOOL ret =
        (WAIT_OBJECT_0 ==
        WaitForMultipleObjects(_countof(q->handles), q->handles, TRUE, timeout));

    if (ret) {
        // Queue has an element, pull it from queue
        *e_out = q->elements[0];

        // -- shift remaining elements down
        MoveMemory(
            &q->elements[0], &q->elements[1],
            sizeof(Element) * (q->max_elements - 1)
        );

        // -- allow other threads to access the queue
        ReleaseMutex(q->handles[MTX_HID]);
    } else {    // timeout!
        SetLastError(ERROR_TIMEOUT);
    }
    return ret;     // call GetLastError for more info
}
// =========================================================================================

Queue               g_q;                    // shared resource b/w threads
volatile LONG       g_shutdown;             // signal client/server threads to die
HWND                g_hwnd;                 // to give status b/w client/server

// -- cache handles to all threads [thread kernel objs]
HANDLE g_threads[MAXIMUM_WAIT_OBJECTS];

int g_threads_count = 0;     // number of reader/writer threads

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

unsigned WINAPI
WriterThread_Func (void * param_ptr) {
    int thread_number = (intptr_t)param_ptr;
    HWND hwnd_listbox = GetDlgItem(g_hwnd, ID_LBOX_CLIENTS);

    int request_number = 0;
    while (1 != InterlockedCompareExchange(&g_shutdown, 0, 0)) {
        ++request_number;   // keep track of current preocessed element

        TCHAR str[1024];
        Element e = {thread_number, request_number};

        // -- try to append an element onto the q
        if (Queue_Append(&g_q, &e, 200)) {
            // -- indicate which thread sent which request
            StringCchPrintf(
                str, _countof(str),
                TEXT("Sending %d:%d"), thread_number, request_number
            );
        } else {
            // -- couldn't put an element onto q
            StringCchPrintf(
                str, _countof(str),
                TEXT("Sending %d:%d (%s)"), thread_number, request_number,
                (GetLastError() == ERROR_TIMEOUT) ? TEXT("Timeout") : TEXT("Full")
            );
        }
        // -- update client list box
        ListBox_SetCurSel(hwnd_listbox, ListBox_AddString(hwnd_listbox, str));
        Sleep(2500);    // wait before appending another element
    }
    // -- always return from exiting thread
    return(0);
}
unsigned WINAPI
ReaderThread_Func (void * param_ptr) {
    int thread_number = (intptr_t)param_ptr;
    HWND hwnd_listbox = GetDlgItem(g_hwnd, ID_LBOX_SERVERS);

    while (1 != InterlockedCompareExchange(&g_shutdown, 0, 0)) {

        TCHAR str[1024];
        Element e;

        // -- try to get an element from the q
        if (Queue_Remove(&g_q, &e, 5000)) {
            // -- indicate which thread processed which request
            StringCchPrintf(
                str, _countof(str),
                TEXT("%d: Processing %d:%d"), thread_number, e.thread_number, e.request_number
            );

            // -- server takes some time to process request
            Sleep(2000 * e.thread_number);
        } else {
            // -- couldn't get an element from q
            StringCchPrintf(
                str, _countof(str),
                TEXT("%d: (timeout)"), thread_number
            );
        }
        // -- update server list box
        ListBox_SetCurSel(hwnd_listbox, ListBox_AddString(hwnd_listbox, str));
    }
    // -- always return from exiting thread
    return(0);
}
BOOL
DialogBox_OnInit (HWND hwnd, HWND hwnd_focus, LPARAM lparam) {
    g_hwnd = hwnd;  // used by client/server threads to show status
    unsigned int thread_id;

    // Create four writer threads (client)
    for (int i = 0; i < 4; ++i)
        g_threads[g_threads_count++] =
        (HANDLE)_beginthreadex(NULL, 0, WriterThread_Func, (void *)(intptr_t)i, 0, &thread_id);

//
// Create two reader threads (server)
    for (int i = 0; i < 2; ++i)
        g_threads[g_threads_count++] =
        (HANDLE)_beginthreadex(NULL, 0, ReaderThread_Func, (void *)(intptr_t)i, 0, &thread_id);

    return TRUE;
}
void
DialogBox_OnCommand (HWND hwnd, int id, HWND hwnd_control, UINT code_notify) {
    switch (id) {
    case IDCANCEL:
        EndDialog(hwnd, id);
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
    Queue_Init(&g_q, 10);

    DialogBox(instance, MAKEINTRESOURCE(ID_DIALOG_MAIN), NULL, &DialogBox_Func);

    // -- mark the closing of dialog box
    InterlockedExchange(&g_shutdown, TRUE);

    // -- wait for all threads to terminate
    WaitForMultipleObjects(g_threads_count, g_threads, TRUE, INFINITE);

    // -- cleanup
    while (g_threads_count--)
        CloseHandle(g_threads[g_threads_count]);
    Queue_Deinit(&g_q);

    return(0);
}

