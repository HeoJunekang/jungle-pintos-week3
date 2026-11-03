#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page {
	struct file *file;
	off_t offset;
	size_t read_bytes;
	struct mmap_info* mmap_info;
};

/* 
mmap_info <-file close의 주체
mmap---------
filepage1
filepage2
filepage3
-------------
addr는 uva 즉 file page의 시작 주소
page_count는 4kb이상의 file page일 경우 4kb 단위로 연속되게 저장됨
*/
struct mmap_info{
	void *addr;
	size_t page_count;
	struct file *con_file;
};


void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
#endif
