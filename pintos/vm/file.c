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
	/*프레임에 적제되어 있을 경우 파일이 수정되어있으면 파일에 정보 저장*/
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
ram에 file을 적재하는 함수 aux는 vm/uninit.c의 uninit_initialize를 통해 uninit_page -> aux가 전달된다.
*/
static bool lazy_load_mmap_page(struct page *page, void *aux){
	/*frame page크기를 file에서 read_bytes 만큼 체우고 나머지를 0으로 패딩*/
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
	page->file.mmap_info = load_aux->mmap_info;
	/*임시 aux 해제*/
	free(load_aux);
	
	return true;
}

/*file_backup_page 물리메모리 매핑에 필요한 정보 적재*/
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

/* 사용자 syscall mmap을 통한 file 페이지 초기 설정 */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	if(!is_user_vaddr(addr)) return NULL;
	/*주소 정렬 부정확*/
	if((uintptr_t)addr % PGSIZE != 0) return NULL;
	/*length == 0 페이지 할당 x*/
	if(length == 0) return NULL;



	/*file에 적혀있는 크기*/
	size_t file_size = file_length(file);
	/*offset 부터 남아있는 데이터 크기*/
	size_t remain_len = (size_t) offset < file_size ? file_size - (size_t) offset : 0;
	/*page 주소 uva*/
	void *cur_addr = addr;
	/*4kb 이상의 파일을 읽을 때 다음 페이지의 오프셋을 설정해 주기 위한 변수*/
	off_t cur_offset = offset;

	/*fd에 파일을 열어 독립된 파일주소와 연결*/
	struct file *file_addr = file_reopen(file);
	if(file_addr == NULL) return NULL;
	
	/*하나의 mmap이 할당받고 있는 전체 page 관리를 위한 구조체 선언*/
	struct mmap_info* mmap_info = malloc(sizeof(struct mmap_info));
	if(mmap_info == NULL){
		file_close(file_addr);
		return NULL;
	}
	
	mmap_info->addr = addr;
	mmap_info->con_file = file_addr;
	int page_count = 0;
	/*
	length 와 remain_len 차이
	length : 파일타입의 페이지의 공간을 할당
	remain_len : 디스크에 저장되어있는 file에서 읽어올 남은 데이터의크기
	*/
	while(length > 0){

		size_t page_len = length < PGSIZE ? length : PGSIZE;
		size_t read_bytes = remain_len < page_len ? remain_len : page_len;
		size_t zero_bytes = PGSIZE - read_bytes;
		/*aux값으로 file로 페이지가 전환되었을 때 구성에 필요한 값 저장*/
		struct mmap_load_info *info = make_load_info(cur_offset, read_bytes, zero_bytes, mmap_info);
		if(info == NULL) return NULL;
		
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
	
	struct thread *munmap_thread = thread_current ();
	struct page *munmap_page = spt_find_page (&munmap_thread -> spt, addr);

	switch (munmap_page->operations->type)
	{
	case VM_UNINIT:
		for (int i = 0; i < ((struct mmap_load_info*)munmap_page -> uninit.aux)->mmap_info->page_count ; i++)
		{
			spt_remove_page (&munmap_thread -> spt, spt_find_page (&munmap_thread -> spt, addr + PGSIZE * i));
		}

		free (munmap_page -> file.mmap_info);
		break;
	
	case VM_FILE:
		for (int i = 0; i < munmap_page -> file.mmap_info->page_count; i++)
		{
			spt_remove_page (&munmap_thread -> spt, spt_find_page (&munmap_thread -> spt, addr + PGSIZE * i));
		}

		free (munmap_page -> file.mmap_info);
		break;

	default:
		break;
	}
}
