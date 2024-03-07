#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. : 쓰레드의 상태*/
enum thread_status {
	THREAD_RUNNING,     /* Running thread. => thread_current()가 실행중인 스레드 리턴 */
	THREAD_READY,       /* Not running but ready to run.=> 스케줄러가 호출될때 실행가능, ready_list저장 */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. : 잠금이나 인터럽트 호출 기다린다. => thread_unblock()전까지 다시 스케줄 안됨 */
	THREAD_DYING        /* About to be destroyed.*/
};

/* Thread identifier type. : 식별자 1부터 시작
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities.: 우선순위 */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 * 
 * 총 4KB : thread structured은 0KB부터, kernel stack은 4KB부터 아래로 성장
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *       
 *       struct thread은 1KB미만으로 유지되야 => 커널 스택 공간 필요
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *       
 *       커널 스택 너무 커져서 오버플로되면 struct thread 손상됨
 *       큰 구조체나 배열을 non-static local variables하지말고 동적 할당해라?
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. */
	int init_priority;                  /* 원래 Priority 저장용*/
	int64_t awake;                      /* 본인 잠들 시간 저장용 */

	struct lock *wait_on_lock;           /* 기다리는 lock */
	struct list donations;              /* Priority 기부 해줄 리스트*/
	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */
	struct list_elem d_elem;            /* donations list_element */

#ifdef USERPROG //만약 USERPROG매크로가 정의되있다면
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching : 재개를 위해? */
	unsigned magic;                     /* Detects stack overflow. : thread_current()가 현재 스레드내 magic멤버가 THREAD_MAGIC인지 확인한다.*/
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;
void remove_donation(struct lock *);
bool cmp_priority (const struct list_elem *,const struct list_elem *,void *);
bool cmp_donation_priority (const struct list_elem *,const struct list_elem *,void *);
void thread_compare_priority(void);
void priority_donation(void);
void re_priority(void);

void thread_sleep(int64_t);
void thread_awake(int64_t);

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

#endif /* threads/thread.h */
