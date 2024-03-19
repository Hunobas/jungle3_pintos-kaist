#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "intrinsic.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* Projects 2 and later. */
static void halt (void) NO_RETURN;
static void exit (int status) NO_RETURN;
static pid_t fork (const char *thread_name, struct intr_frame *f);
static int exec (const char *file);
static int wait (pid_t);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);
static int open (const char *file);
static int filesize (int fd);
static int read (int fd, void *buffer, unsigned length);
static int write (int fd, const void *buffer, unsigned length);
static void seek (int fd, unsigned position);
static unsigned tell (int fd);
static void close (int fd);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

struct lock filesys_lock;

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	lock_init (&filesys_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
	// TODO: Your implementation goes here.
	uint64_t handler_num = f->R.rax;

	switch (handler_num)
	{
	case SYS_HALT:
		halt ();
		break;
	case SYS_EXIT:
		exit (f->R.rdi);
		break;
	case SYS_FORK:
		f->R.rax = fork (f->R.rdi, f);
		break;
	case SYS_EXEC:
		f->R.rax = exec (f->R.rdi);
		break;
	case SYS_WAIT:
		f->R.rax = wait (f->R.rdi);
		break;
	case SYS_CREATE:
		f->R.rax = create (f->R.rdi, f->R.rsi);
		break;
	case SYS_REMOVE:
		f->R.rax = remove (f->R.rdi);
		break;
	case SYS_OPEN:
		f->R.rax = open (f->R.rdi);
		break;
	case SYS_FILESIZE:
		f->R.rax = filesize (f->R.rdi);
		break;
	case SYS_READ:
		f->R.rax = read (f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_WRITE:
		if ((f->R.rax = write (f->R.rdi, f->R.rsi, f->R.rdx)) == -1)
			exit (-1);
		break;
	case SYS_SEEK:
		seek (f->R.rdi, f->R.rsi);
		break;
	case SYS_TELL:
		f->R.rax = tell (f->R.rdi);
		break;
	case SYS_CLOSE:
		close (f->R.rdi);
		break;
	default:
		printf("unknown syscall num!\n");
		break;
	}
}

static void
check_address (void *addr) {
	struct thread *t = thread_current ();
	/* --- Project 2: User memory access --- */
	if (addr == NULL || !is_user_vaddr (addr) || 
	pml4_get_page (t->pml4, addr) == NULL) {
		// printf("wrong address!: %p\n", addr);
		exit(-1);
	}
}


static void halt (void) {
	power_off ();
}


static void exit (int status) {
	thread_current ()->exit_status = status;
	sema_up (&thread_current ()->wait_sema);
	thread_exit ();
}


static pid_t fork (const char *thread_name, struct intr_frame *f) {
	return process_fork (thread_name, f);
}


static int exec (const char *file) {
	check_address(file);

	char *fn_copy;
	int size = strlen (file) + 1;

	if ((fn_copy = palloc_get_page (PAL_ZERO)) == NULL)
		exit (-1);

	strlcpy (fn_copy, file, size);

	return process_exec (fn_copy);
}


static int wait (pid_t pid) {
	return process_wait (pid);
}


static bool create (const char *file, unsigned initial_size) {
	check_address (file);
	return filesys_create (file, initial_size);
}


static bool remove (const char *file) {
	check_address (file);
	return filesys_remove (file);
}


static int open (const char *file) {
	struct thread *t = thread_current();
	int fd = t->fdidx;

	check_address (file);
	struct file *file_obj = filesys_open (file); // 열려고 하는 파일 객체 정보를 filesys_open()으로 받기
	
	// 제대로 파일 생성됐는지 체크
	if (file_obj == NULL)
		return -1;

	while (fd < FDT_COUNT_LIMIT && t->fd_table[fd] != NULL)
		fd++;

	// fd가 FDT_COUNT_LIMIT를 넘으면 예외처리
	if (fd >= FDT_COUNT_LIMIT)
		file_close (file_obj);

	t->fdidx = fd;
	t->fd_table[fd] = file_obj;

	return fd;
}


static int filesize (int fd) {
	if (fd < 0 || fd >= FDT_COUNT_LIMIT)
        return -1;

	struct thread *t = thread_current ();
	struct file *file_obj = t->fd_table[fd];

	if (file_obj == NULL)
		return -1;

	return file_length (file_obj);
}


static int read (int fd, void *buffer, unsigned length) {
	check_address (buffer);
	check_address (buffer + length - 1);

	int read_count;
	unsigned char *save_ptr = buffer;

	if (fd < 0 || fd >= FDT_COUNT_LIMIT || fd == STDOUT_FILENO)
        return -1;

	struct thread *t = thread_current ();
	struct file *file_obj = t->fd_table[fd];

	if (file_obj == NULL)
		return -1;

	if (fd == STDIN_FILENO) {
		char key;
		for (read_count = 0; read_count < length; read_count++) {
			key = input_getc ();
			*save_ptr++ = key;
			if (key == '\0') { // same as '\n' by user
				break;
			}
		}
	}
	else {
		lock_acquire (&filesys_lock);
		read_count = file_read (file_obj, buffer, length); // 파일 읽어들일 동안만 lock 걸어준다.
		lock_release (&filesys_lock);
	}
	return read_count;
}


static int write (int fd, const void *buffer, unsigned length) {
	// 파일 정보를 사용하여 유저 모드의 버퍼 유효성 검사
	check_address (buffer);
	check_address (buffer + length - 1);

	int write_count;

    if (length > (1 << 14))
        return 0;
	
    if (fd == STDOUT_FILENO) {
        // 표준 출력에 쓰기
        putbuf(buffer, length);
		write_count = length;

    } else {
        if (fd < 0 || fd >= FDT_COUNT_LIMIT || fd == STDIN_FILENO)
            return -1;

		struct thread *t = thread_current();
		struct file *file_obj = t->fd_table[fd];

		if (file_obj == NULL)
			return -1;

		lock_acquire (&filesys_lock);
		write_count = file_write (file_obj, buffer, length);
		lock_release (&filesys_lock);
	}

    return write_count;
}


static void seek (int fd, unsigned position) {
	if (fd < 2 || fd >= FDT_COUNT_LIMIT)
		return;

	struct thread *t = thread_current();
	struct file *file_obj = t->fd_table[fd];

	if (file_obj == NULL)
		return;

	file_seek(file_obj, position);
} 


static unsigned tell (int fd) {
	if (fd < 2 || fd >= FDT_COUNT_LIMIT)
		return 0;

	struct thread *t = thread_current();
	struct file *file_obj = t->fd_table[fd];

	if (file_obj == NULL)
		return 0;

	return file_tell(file_obj);
}


static void close (int fd) {
	if (fd < 2 || fd >= FDT_COUNT_LIMIT)
		return;

	struct thread *t = thread_current ();
	struct file *file_obj = t->fd_table[fd];

	if (file_obj == NULL)
		return;

	file_close (file_obj);
	t->fd_table[fd] = NULL;
}
