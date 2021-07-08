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
#include <tchar.h>

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

int g_nthreads = 0;     // number of reader/writer threads

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

}
