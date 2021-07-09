/* ===========================================================
   #File: srwlock_cvs #
   #Date: 08 July 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description: Experimenting with SRWLock and condition variables #
   #following "Windows via C/C++" 08-Queue example
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

// =========================================================================================

struct Queue {
    struct Element {
        int thread_number;
        int request_number;
        /* additional element data */

    };
    struct InnerElement {
        int             stamp;      // element counter: 0 means read
        struct Element  e;
    };
    int                     max_elements;   // max # of elements
    int                     curr_stamp;     // keep track of the # of added elements
    struct InnerElement *   elements;       // array of elements
};

typedef struct Queue Queue;
typedef struct Element Element;
typedef struct InnerElement InnerElement;

static void
Queue_Init (Queue * q, int max_e) {
    q->elements = (InnerElement *)HeapAlloc(
        GetProcessHeap(), 0, sizeof(InnerElement) * max_e
    );
    if (q->elements)
        ZeroMemory(q->elements, sizeof(InnerElement) * max_e);

    q->curr_stamp = 0;  // initialize the element counter
    q->max_elements = max_e;
}
static void
Queue_Deinit (Queue * q) {
    HeapFree(GetProcessHeap(), 0, q->elements);
}
static int
Queue_GetFreeSlot (Queue * q) {
    int ret = -1;   // -1 means no free slot
    //
    // Look for the first element with a stamp of 0
    for (int i = 0; i < q->max_elements; ++i) {
        if (0 == q->elements[i].stamp) {
            ret = i;
            break;
        }
    }
    return ret;
}
static BOOL
Queue_IsFull (Queue * q) {
    return (-1 == Queue_GetFreeSlot(q));
}
static int
Queue_GetNextSlot (Queue * q, int thread_number) {
    int ret = -1;   // by default there is not slot for this thread

    // -- element can't hav a stamp higher than last added
    int first_stamp = q->curr_stamp + 1;

    // -- look for element that is not free
    // -- even means thread-0, odd means thread-1
    for (int i = 0; i < q->max_elements; ++i) {
        // keep track of the lowest stamp to ensure FIFO behavior
        if (
            (q->elements[i].stamp != 0) &&  // free element
            ((q->elements[i].e.request_number % 2) == thread_number) &&
            (q->elements[i].stamp < first_stamp)
        ) {
            first_stamp = q->elements[i].stamp;
            ret = i;    // first slot
        }
    }
    return ret;
}
static BOOL
Queue_IsEmpty (Queue * q, int thread_number) {
    return (-1 == Queue_GetNextSlot(q, thread_number));
}
static void
Queue_AddElement (Queue * q, Element e) {
    // -- do nothing if q is full
    int free_slot = Queue_GetFreeSlot(q);
    if (free_slot != -1) {
        // -- cpy element
        q->elements[free_slot].e = e;
        // -- mark e with new stamp
        q->elements[free_slot].stamp = ++q->curr_stamp;
        // NOTE(omid): C 101: postfix operators like -> have higher precedence than unary (prefix) operators
    }
}
static BOOL
Queue_GetNewElement (Queue * q, int thread_number, Element * e_out) {
    BOOL ret = FALSE;
    int new_slot = Queue_GetNextSlot(q, thread_number);
    if (new_slot != -1) {
        *e_out = q->elements[new_slot].e;
        q->elements[new_slot].stamp = 0;    // mark as read
        ret = TRUE;
    }
    return ret;
}

// =========================================================================================

Queue               g_q;                    // shared resource b/w threads
volatile LONG       g_shutdown;             // signal client/server threads to die
HWND                g_hwnd;                 // to give status b/w client/server
SRWLOCK             g_srwlock;              // slim reader-writer lock to protect q
CONDITION_VARIABLE  g_cv_ready_to_read;     // signaled by writers
CONDITION_VARIABLE  g_cv_ready_to_write;    // signaled by readers

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
static void
add_text (HWND hwnd_list_box, PCTSTR fmt, ...) {
    va_list arglist;
    va_start(arglist, fmt);
    TCHAR str[2 * 1024];
    _vstprintf_s(str, _countof(str), fmt, arglist);
    ListBox_SetCurSel(hwnd_list_box, ListBox_AddString(hwnd_list_box, str));
    va_end(arglist);
}
static void
stop_processing() {
    if (!g_shutdown) {
        // -- ask all threads to end
        InterlockedExchange(&g_shutdown, TRUE);

        // -- free all threads waiting on condition variables
        WakeAllConditionVariable(&g_cv_ready_to_read);
        WakeAllConditionVariable(&g_cv_ready_to_write);

        // -- wait for all the threads to terminate & then cleanup
        WaitForMultipleObjects(g_threads_count, g_threads, TRUE, INFINITE);

        // -- cleanup kernel objects
        while (g_threads_count--)
            CloseHandle(g_threads[g_threads_count]);

        // -- close each list box
        add_text(GetDlgItem(g_hwnd, IDC_LIST_SERVERS), TEXT("-----------------"));
        add_text(GetDlgItem(g_hwnd, IDC_LIST_CLIENTS), TEXT("-----------------"));
    }
}
unsigned WINAPI
StoppingThread_Func (void * param_ptr) {
    stop_processing();
    return(0);
}
unsigned WINAPI
WriterThread_Func (void * param_ptr) {
    int thread_number = (intptr_t)param_ptr;
    HWND hwnd_listbox = GetDlgItem(g_hwnd, IDC_LIST_CLIENTS);

    for (int request_number = 1; !g_shutdown; ++request_number) {
        Element e = {thread_number, request_number};

        // -- require acess for writing
        AcquireSRWLockExclusive(&g_srwlock);

        // -- if q is full, fall sleep as long as condition variable is not signaled
        // NOTE(omid): during wait for lock, a shutdown might have been instructed
        if (Queue_IsFull(&g_q) && !g_shutdown) {
            add_text(
                hwnd_listbox,
                TEXT("[%d] Queue is full: Cannot add %d"), thread_number, request_number
            );
            // -- wait for a reader to empty a slot before acquiring lock again
            SleepConditionVariableSRW(&g_cv_ready_to_write, &g_srwlock, INFINITE, 0);
        }
        if (g_shutdown) {   // -- shutting down

            // NOTE(omid): Other writer threads might still be blocked on the lock
            // -- release the lock. No need to keep the lock any longer
            ReleaseSRWLockExclusive(&g_srwlock);
            // -- signal other blocked writer threads it's time to exit
            WakeAllConditionVariable(&g_cv_ready_to_write);

            add_text(hwnd_listbox, TEXT("[%d] exiting; Bye Bye"), thread_number);
            // -- always return from exiting thread
            return(0);
        } else {
            // -- add new element
            Queue_AddElement(&g_q, e);

            add_text(hwnd_listbox, TEXT("[%d] adding %d"), thread_number, request_number);

            // -- no need to keep the lock after writing
            ReleaseSRWLockExclusive(&g_srwlock);

            // -- signal reader threads there is new element to read
            WakeAllConditionVariable(&g_cv_ready_to_read);

            // -- wait before adding another element
            Sleep(1500);
        }
    }
    add_text(hwnd_listbox, TEXT("[%d] exiting; Bye Bye"), thread_number);
    // -- always return from exiting thread
    return(0);
}
static BOOL
consume_element (int thread_num, int request_num, HWND lbox) {
    // get shared access to queue to read an element
    AcquireSRWLockShared(&g_srwlock);

    // fall asleep until there is s.th. to read
    // check if, while asleep, it was not decided to stop the thread
    while (Queue_IsEmpty(&g_q, thread_num) && !g_shutdown) {
        // no readable element
        add_text(lbox, TEXT("[%d] Nothing to process"), thread_num);

        // since q is empty wait until writers produce anything
        SleepConditionVariableSRW(
            &g_cv_ready_to_read,
            &g_srwlock,
            INFINITE,
            CONDITION_VARIABLE_LOCKMODE_SHARED
        );
    }
    // on the other hand, when thread is exiting, lock should be released,
    // and other reader(s) should be signaled through condition variables
    if (g_shutdown) {
        add_text(lbox, TEXT("[%d] exiting; Bye Bye"), thread_num);

        ReleaseSRWLockShared(&g_srwlock);
        WakeConditionVariable(&g_cv_ready_to_read);

        return FALSE;
    }
    //
    // Consuming new element. Here we know q is not empty 
    Element e;
    Queue_GetNewElement(&g_q, thread_num, &e);

    // -- no need to keep the lock any longer
    ReleaseSRWLockShared(&g_srwlock);

    add_text(
        lbox, TEXT("[%d] Processing %d: %d"), thread_num,
        e.thread_number, e.request_number
    );

    // -- notify writers a free slot became available to produce new element
    WakeConditionVariable(&g_cv_ready_to_write);

    return TRUE;
}
unsigned WINAPI
ReaderThread_Func (void * param_ptr) {
    int thread_number = (intptr_t)param_ptr;
    HWND hwnd_listbox = GetDlgItem(g_hwnd, IDC_LIST_SERVERS);

    for (int request_number = 1; !g_shutdown; ++request_number) {
        if (FALSE == consume_element(thread_number, request_number, hwnd_listbox))
            return (0);
        Sleep(2500);    // wait before reading another element
    }
    // g_shutdown has been set during sleep
    add_text(hwnd_listbox, TEXT("[%d] exiting; Bye Bye"), thread_number);
    // -- always return from exiting thread
    return(0);
}
BOOL
DialogBox_OnInit (HWND hwnd, HWND hwnd_focus, LPARAM lparam) {
    g_hwnd = hwnd;  // used by client/server threads to show status

    //
    // Init SRWLock to be used later
    InitializeSRWLock(&g_srwlock);

    //
    // Init condition variables to be used later
    InitializeConditionVariable(&g_cv_ready_to_read);
    InitializeConditionVariable(&g_cv_ready_to_write);

    g_shutdown = FALSE;

    //
    // Create four writer threads
    unsigned int thread_id;
    for (int i = 0; i < 4; ++i)
        g_threads[g_threads_count++] =
        (HANDLE)_beginthreadex(NULL, 0, WriterThread_Func, (void *)(intptr_t)i, 0, &thread_id);

//
// Create two reader threads
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
    case IDC_BTN_STOP: {
        // -- stop_processing cannot be called from UI thread (a deadlock occurs)
        // -- another thread is required
        unsigned int thread_id;
        CloseHandle(
            (HANDLE)_beginthreadex(NULL, 0, StoppingThread_Func, NULL, 0, &thread_id)
        );
        // -- the button cannot be pushed twice
        Button_Enable(hwnd_control, FALSE);
    }break;
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
    // NOTE(omid): The resource identifier of dialog box is created by MAKEINTRESOURCE macro
    DialogBox(instance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, &DialogBox_Func);
    stop_processing();
    Queue_Deinit(&g_q);
    return(0);
}
