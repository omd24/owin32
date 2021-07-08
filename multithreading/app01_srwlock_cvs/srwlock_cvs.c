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
// UI Controls IDs
//
#define ID_DIALOGBOX_PARENT                 1
#define ID_DIALOGBOX_ITEM_CLIENTS           1000
#define ID_DIALOGBOX_ITEM_SERVERS           1001
#define ID_BTN_STOP                         1002

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
        add_text(GetDlgItem(g_hwnd, ID_DIALOGBOX_ITEM_SERVERS), TEXT("-----------------"));
        add_text(GetDlgItem(g_hwnd, ID_DIALOGBOX_ITEM_CLIENTS), TEXT("-----------------"));
    }
}
unsigned WINAPI
stopping_thread_func (void * param_ptr) {

    stop_processing();
    return(0);
}
unsigned WINAPI
writer_thread_func (void * param_ptr) {

    //...
    return(0);
}
unsigned WINAPI
reader_thread_func (void * param_ptr) {

    //...
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
            (HANDLE)_beginthreadex(NULL, 0, writer_thread_func, (void *)i, 0, &thread_id);

    //
    // Create two reader threads
    for (int i = 0; i < 2; ++i)
        g_threads[g_threads_count++] =
            (HANDLE)_beginthreadex(NULL, 0, reader_thread_func, (void *)i, 0, &thread_id);

    return TRUE;
}
void
DialogBox_OnCommand (HWND hwnd, int id, HWND hwnd_control, UINT code_notify) {
    switch (id) {
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    case ID_BTN_STOP: {
        // -- stop_processing cannot be called from UI thread (a deadlock occurs)
        // -- another thread is required
        unsigned int thread_id;
        CloseHandle(
            (HANDLE)_beginthreadex(NULL, 0, stopping_thread_func, NULL, 0, &thread_id)
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
    // NOTE(omid): The resource identifier of dialog box is created by MAKEINTRESOURCE macro
    DialogBox(instance, MAKEINTRESOURCE(ID_DIALOGBOX_PARENT), NULL, &DialogBox_Func);
    stop_processing();
    return(0);
}
