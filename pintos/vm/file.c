/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "vm/file.h"


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






bool 
lazy_load_mmap_page (struct page *page, void *aux){
	//여기서 메모리 매핑

	struct mmap_load_info *mmap_load_info = (struct mmap_load_info*) aux;
	struct mmap_info *mmap_info = mmap_load_info -> mmap_info;
	
	//페이지 할당해서 파일내용을 로딩하기
	//(읽을파일, 데이터 저장할 주소, 읽으려는 바이트수, 파일에서의 위치)
	off_t read_byte = file_read_at(mmap_info->mmap_file, page->frame->kva, mmap_load_info->page_read_bytes,
		mmap_load_info->ofs);

	if (read_byte != (off_t)mmap_load_info->page_read_bytes) {
		free(mmap_load_info);
		return false;
	}

	//남은 부분은 0바이트로 패딩하기
	memset (page->frame->kva + mmap_load_info-> page_read_bytes, 0, mmap_load_info-> page_zero_bytes);


	//페이지의 file 타입 정보 채우기
	page->file.file = mmap_load_info->mmap_info->mmap_file;
	page->file.offset = mmap_load_info->ofs;
	page->file.read_bytes = mmap_load_info->page_read_bytes;
	
	free(mmap_load_info);
	return true;

}


/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {

	//(접근주소, 읽는 길이, 쓰기여부, 파일 구조체, 파일 오프셋)

	//파일 위치
	struct file *mmap_file = file_reopen(file);
	if(mmap_file == NULL){
		return NULL;
	}
	//접근 주소
	void *cur_addr = addr;
	//남은 읽을 길이
	size_t remain_length = length;
	//읽기 시작하는 오프셋
	off_t cur_offset = offset;

	//mmap_info 구조체 준비 => 파일 정보 넣음
	struct mmap_info *mmap_info = malloc(sizeof(struct mmap_info));
	if(mmap_info = NULL){
		file_close(mmap_file);
		return NULL;
	}

	mmap_info -> addr = addr;
	mmap_info ->mmap_file = mmap_file;
	int page_count = 0;

	//파일 다 읽을때까지 파일 읽기
	while(remain_length > 0){
		//읽을 바이트 설정 -> PGSIZE보다 많이 남으면 page 만큼 읽고 아니면 남은 만큼 읽기
		size_t read_bytes = remain_length > PGSIZE ? PGSIZE : remain_length;
		//페이지 남은 부분 0으로 채울 바이트
		size_t zero_bytes = PGSIZE - read_bytes;

		//aux로 보낼 구조체 준비
		struct mmap_load_info *info = malloc(sizeof(struct mmap_load_info));
		if(info = NULL) return false;

		//initializer aux로 사용할 struct 준비
		info->page_read_bytes = read_bytes;
		info->page_zero_bytes = zero_bytes;
		info->ofs = cur_offset;
		info->mmap_info = mmap_info;


		//initializer 호출
		void *aux = info;
		if(!vm_alloc_page_with_initializer(VM_FILE, cur_addr, writable, lazy_load_mmap_page, aux)){
			return NULL;
		}
		
		page_count++;
		//읽기 위한 정보 update
		cur_addr = (uint8_t *)cur_addr + PGSIZE;
		remain_length -= PGSIZE;
		cur_offset += PGSIZE;

	}
	mmap_info -> page_count = page_count;
	return;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	//uninit도 죽여야함.
}
