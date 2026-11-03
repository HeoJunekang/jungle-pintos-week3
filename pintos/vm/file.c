/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include <string.h>
#include "threads/mmu.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;
	struct file_page *file_page = &page->file;

	file_page->file = NULL;
	file_page->offset = 0;
	file_page->read_bytes = 0;
	
	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
} 

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	if(page->frame != NULL){
		if(pml4_is_dirty(thread_current()->pml4, page->va) && file_page->file!= NULL){
			file_write_at(file_page->file, page->frame->kva, file_page->read_bytes, file_page->offset);
		}
	}
	pml4_clear_page(thread_current()->pml4, page->va);
	
}

/* Do the mmap */
struct mmap_load_info{
	off_t offset;
	size_t zero_bytes;
	size_t read_bytes;
	struct mmap_info *mmap_info;
};

/*
현재 file_backed_initailziaer()에서 VM_FILE로 타입 변경해주면서 aux를 만들어 넘겨줌
파일에서 ram으로 데이터 읽기
page->file에 파일 정보 보존
임시 aux 해제
*/
static bool lazy_load_mmap_page(struct page *page, void *aux){

	struct mmap_load_info *load_aux = (struct mmap_load_info*) aux;
	off_t read_byte = file_read_at(load_aux->mmap_info->con_file, page->frame->kva, load_aux->read_bytes, load_aux->offset);
		
	if(read_byte != (off_t)load_aux->read_bytes){
		free(load_aux);
		return false;
	}

	memset (page->frame->kva + load_aux->read_bytes, 0, load_aux->zero_bytes);

	page->file.file = load_aux->mmap_info->con_file;
	page->file.offset = load_aux->offset;
	page->file.read_bytes = load_aux->read_bytes;

	free(load_aux);
	
	return true;
}

static struct mmap_load_info*
make_load_info(off_t offset, size_t read_bytes, size_t zero_bytes, void* mmap_info)
{
	struct mmap_load_info * info = malloc(sizeof(struct mmap_load_info));
	if(info == NULL) return NULL;

	info->offset = offset;
	info->read_bytes = read_bytes;
	info->zero_bytes = zero_bytes;
	info->mmap_info = mmap_info;

	return info;
}

void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {

	size_t file_size = file_length(file);
	size_t remain_len = (size_t) offset < file_size ? file_size - (size_t) offset : 0;
	void *cur_addr = addr;
	off_t cur_offset = offset;

	struct file *file_addr = file_reopen(file);
	if(file_addr == NULL) return NULL;

	struct mmap_info* mmap_info = malloc(sizeof(struct mmap_info));
	if(mmap_info == NULL){
		file_close(file_addr);
		return NULL;
	}
	
	mmap_info->addr = addr;
	mmap_info->con_file = file_addr;
	int page_count = 0;

	while(length > 0){

		size_t page_len = length < PGSIZE ? length : PGSIZE;
		size_t read_bytes = remain_len < page_len ? remain_len : page_len;
		size_t zero_bytes = PGSIZE - read_bytes;

		struct mmap_load_info *info = make_load_info(cur_offset, read_bytes, zero_bytes, mmap_info);
		if(info == NULL) return false;

		void *aux = info; 
		if(!vm_alloc_page_with_initializer(VM_FILE, cur_addr, writable, lazy_load_mmap_page, aux)){
			return NULL;
		}

		page_count++;
		length -= page_len;
		remain_len -= read_bytes;
		cur_addr = (uint8_t *)cur_addr + PGSIZE;
		cur_offset += PGSIZE;
		
	}
	mmap_info->page_count = page_count;

	return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
}
