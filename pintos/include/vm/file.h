#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page {
};

struct 
mmap_info
{
	void *addr;
	struct mmap_file *mmap_file;
	int page_count;
};

struct
mmap_load_info
{
	//lazy_load_mmap_page에 들어갈 aux값 세팅
	size_t page_read_bytes;
	size_t page_zero_bytes;
	off_t ofs;
	struct mmap_info *mmap_info;
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
#endif
